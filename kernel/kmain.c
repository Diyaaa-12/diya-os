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

static struct task main_task;
static struct task *task_a;
static struct task *task_b;
static volatile int switch_count = 0;
static volatile uint64_t start_tick;

static void task_a_entry(void)
{
    for (;;) {
        if (switch_count % 500 == 0) {
            serial_write("TaskA: running, tick=0x");
            serial_write_hex(timer_get_ticks());
            serial_write("\n");
        }

        switch_count++;
                if (switch_count >= 2000) {
            uint64_t end_tick = timer_get_ticks();
            serial_write("TaskA: done, halting. start_tick=0x");
            serial_write_hex(start_tick);
            serial_write(" end_tick=0x");
            serial_write_hex(end_tick);
            serial_write(" elapsed=0x");
            serial_write_hex(end_tick - start_tick);
            serial_write("\n");
            for (;;) { __asm__ ("hlt"); }
        }

        task_switch_to(task_b);
    }
}

static void task_b_entry(void)
{
    for (;;) {
        if (switch_count % 500 == 0) {
            serial_write("TaskB: running, tick=0x");
            serial_write_hex(timer_get_ticks());
            serial_write("\n");
        }

        switch_count++;
                if (switch_count >= 2000) {
            uint64_t end_tick = timer_get_ticks();
            serial_write("TaskA: done, halting. start_tick=0x");
            serial_write_hex(start_tick);
            serial_write(" end_tick=0x");
            serial_write_hex(end_tick);
            serial_write(" elapsed=0x");
            serial_write_hex(end_tick - start_tick);
            serial_write("\n");
            for (;;) { __asm__ ("hlt"); }
        }
            start_tick = timer_get_ticks();
        task_switch_to(task_a);
    }
}

void kmain(void)
{
    serial_init();
    serial_write("DiyaOS: Milestone 1 boot OK\n");

    gdt_init();
    serial_write("DiyaOS: GDT loaded\n");

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

    vmm_switch_to_kernel_pml4();
    serial_write("DiyaOS: CR3 switched successfully, still alive\n");

    heap_init();
    serial_write("DiyaOS: heap initialized\n");

    /* --- Context switching test --- */
    task_set_current(&main_task);
    task_a = task_create(task_a_entry);
    task_b = task_create(task_b_entry);

    serial_write("DiyaOS: switching to Task A -- testing context switching\n");
    task_switch_to(task_a);

    for (;;) {
        __asm__ ("hlt");
    }
}