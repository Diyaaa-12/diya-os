#include <stdint.h>
#include "../third_party/limine/limine.h"
#include "drivers/serial.h"
#include "drivers/pic.h"
#include "drivers/timer.h"
#include "arch/x86_64/gdt.h"
#include "arch/x86_64/idt.h"
#include "mm/pmm.h"
#include "mm/vmm.h"
#include "mm/heap.h"
#include "sched/task.h"
#include "sched/scheduler.h"
#include "arch/x86_64/tss.h"

__attribute__((used, section(".limine_requests")))
static volatile uint64_t limine_base_revision[] = LIMINE_BASE_REVISION(3);

__attribute__((used, section(".limine_requests")))
static volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST_ID,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST_ID,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_executable_address_request exec_addr_request = {
    .id = LIMINE_EXECUTABLE_ADDRESS_REQUEST_ID,
    .revision = 0
};

__attribute__((used, section(".limine_requests_start")))
static volatile uint64_t limine_requests_start_marker[] = LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests_end")))
static volatile uint64_t limine_requests_end_marker[] = LIMINE_REQUESTS_END_MARKER;

extern uint64_t __kernel_end;
extern uint64_t __user_text_start;
extern uint64_t __user_text_end;
extern void enter_usermode(uint64_t entry, uint64_t user_stack_top);

static uint8_t user_stack[0x4000];
static uint8_t tss_stack[0x4000];
static struct task main_task;
static struct task *task_a;
static struct task *task_b;
static volatile uint64_t a_iterations = 0;
static volatile uint64_t b_iterations = 0;

static void task_a_entry(void)
{
    for (;;) {
        a_iterations++;
        if (a_iterations % 300000 == 0) {
            serial_write("TaskA: iter=0x");
            serial_write_hex(a_iterations);
            serial_write(" tick=0x");
            serial_write_hex(timer_get_ticks());
            serial_write(" preemptions_so_far=0x");
            serial_write_hex(scheduler_get_switch_count());
            serial_write("\n");
        }
    }
}

static void task_b_entry(void)
{
    for (;;) {
        b_iterations++;
        if (b_iterations % 300000 == 0) {
            serial_write("TaskB: iter=0x");
            serial_write_hex(b_iterations);
            serial_write(" tick=0x");
            serial_write_hex(timer_get_ticks());
            serial_write(" preemptions_so_far=0x");
            serial_write_hex(scheduler_get_switch_count());
            serial_write("\n");
        }
    }
}

__attribute__((section(".user_text")))
static void usermode_syscall_test(void)
{
    char msg[] = "Hello from ring 3 via syscall!\n";

    __asm__ volatile (
        "mov $0, %%rax\n"
        "mov %0, %%rdi\n"
        "int $0x80\n"
        :
        : "r"(msg)
        : "rax", "rdi", "memory"
    );

    __asm__ volatile (
        "mov $1, %%rax\n"
        "int $0x80\n"
        :
        :
        : "rax"
    );

    for (;;) { }
}

/* Deliberately privileged instruction -- cli is ring-0-only. If ring 3
 * isolation is genuinely working, this MUST fault (General Protection
 * Fault, vector 13) rather than silently succeed or do nothing. */
__attribute__((section(".user_text")))
static void usermode_test_entry(void)
{
    __asm__ volatile ("cli");

    /* Unreachable if the fault occurred as expected. */
    for (;;) { }
}

void kmain(void)
{
    serial_init();
    serial_write("DiyaOS: Milestone 1 boot OK\n");

    tss_init((uint64_t)(tss_stack + sizeof(tss_stack)));
    gdt_init();
    serial_write("DiyaOS: GDT + TSS loaded\n");

    idt_init();
    serial_write("DiyaOS: IDT loaded\n");

    pic_remap();
    pit_init(100);
    pic_unmask_irq(0);
    __asm__ ("sti");
    serial_write("DiyaOS: timer enabled, interrupts on\n");

    struct limine_memmap_response *memmap = memmap_request.response;
    uint64_t hhdm_offset = hhdm_request.response->offset;

    pmm_init(memmap, hhdm_offset);
    vmm_init(hhdm_offset);

    uint64_t kvirt_base = exec_addr_request.response->virtual_base;
    uint64_t kphys_base = exec_addr_request.response->physical_base;
    uint64_t kernel_end  = (uint64_t)&__kernel_end;

    for (uint64_t v = kvirt_base; v < kernel_end; v += 0x1000) {
        vmm_map_page(v, kphys_base + (v - kvirt_base), PAGE_WRITABLE);
    }

    #define PHYS_MAP_CAP 0x100000000ULL
    uint64_t highest_mapped_phys = 0;
    for (uint64_t i = 0; i < memmap->entry_count; i++) {
        struct limine_memmap_entry *entry = memmap->entries[i];
        if (entry->type == LIMINE_MEMMAP_BAD_MEMORY) continue;
        if (entry->base >= PHYS_MAP_CAP) continue;
        uint64_t end = entry->base + entry->length;
        if (end > PHYS_MAP_CAP) end = PHYS_MAP_CAP;
        if (end > highest_mapped_phys) highest_mapped_phys = end;
    }
    for (uint64_t phys = 0; phys < highest_mapped_phys; phys += 0x1000) {
        vmm_map_page(hhdm_offset + phys, phys, PAGE_WRITABLE);
    }

    serial_write("DiyaOS: kernel + HHDM ranges mapped in new page tables\n");

    /* Re-map just the .user_text section (and the ring-3 test stack) as
     * USER-accessible. Everything else in the kernel stays protected --
     * this is a deliberately narrow, scoped exception for this one test
     * function, not a general relaxation. Real user programs (Milestone
     * 11, ELF loading) will get their own separate address space instead
     * of sharing the kernel's. */
    uint64_t ut_start = (uint64_t)&__user_text_start;
    uint64_t ut_end   = (uint64_t)&__user_text_end;
    for (uint64_t v = ut_start; v < ut_end; v += 0x1000) {
        vmm_map_page(v, kphys_base + (v - kvirt_base), PAGE_WRITABLE | PAGE_USER);
    }

    uint64_t us_start = (uint64_t)user_stack & ~0xFFFULL;
    uint64_t us_end = ((uint64_t)user_stack + sizeof(user_stack) + 0xFFF) & ~0xFFFULL;
    for (uint64_t v = us_start; v < us_end; v += 0x1000) {
        vmm_map_page(v, kphys_base + (v - kvirt_base), PAGE_WRITABLE | PAGE_USER);
    }

    serial_write("DiyaOS: user_text + user_stack re-mapped as USER-accessible\n");

    vmm_switch_to_kernel_pml4();
    serial_write("DiyaOS: CR3 switched successfully, still alive\n");

    heap_init();
    serial_write("DiyaOS: heap initialized\n");

    serial_write("DiyaOS: jumping to ring 3 -- testing syscall\n");
    enter_usermode((uint64_t)usermode_syscall_test, (uint64_t)(user_stack + sizeof(user_stack)));

    serial_write("DiyaOS: unreachable if ring 3 isolation failed\n");

    task_set_current(&main_task);
    task_a = task_create(task_a_entry);
    task_b = task_create(task_b_entry);

    scheduler_add_task(task_a);
    scheduler_add_task(task_b);

    serial_write("DiyaOS: starting preemptive scheduling -- neither task yields voluntarily\n");
    task_switch_to(task_a);

    for (;;) {
        __asm__ ("hlt");
    }
}