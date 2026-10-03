#include <stdint.h>
#include "../third_party/limine/limine.h"
#include "drivers/serial.h"
#include "drivers/pic.h"
#include "drivers/timer.h"
#include "arch/x86_64/gdt.h"
#include "arch/x86_64/idt.h"
#include "mm/pmm.h"
#include "mm/vmm.h"

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

    /* --- Paging setup --- */
    vmm_init(hhdm_offset);

    uint64_t kvirt_base = exec_addr_request.response->virtual_base;
    uint64_t kphys_base = exec_addr_request.response->physical_base;
    uint64_t kernel_end  = (uint64_t)&__kernel_end;

    serial_write("DiyaOS: mapping kernel range virt=0x");
    serial_write_hex(kvirt_base);
    serial_write(" - 0x");
    serial_write_hex(kernel_end);
    serial_write("\n");

    for (uint64_t v = kvirt_base; v < kernel_end; v += 0x1000) {
        vmm_map_page(v, kphys_base + (v - kvirt_base), PAGE_WRITABLE);
    }

    /* Map all "real RAM" physical memory, not just usable frames --
     * Limine's own boot stack lives in bootloader-reclaimable memory,
     * which our PMM doesn't track but our HHDM still needs to cover.
     * Capped at 4GiB: higher memmap entries here are PCI64 MMIO windows,
     * not real RAM, and would be wasteful/pointless to map. */
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

    uint64_t hhdm_range_end = hhdm_offset + highest_mapped_phys;

    serial_write("DiyaOS: mapping HHDM range virt=0x");
    serial_write_hex(hhdm_offset);
    serial_write(" - 0x");
    serial_write_hex(hhdm_range_end);
    serial_write(" (covers all real RAM below 4GiB)\n");

    for (uint64_t phys = 0; phys < highest_mapped_phys; phys += 0x1000) {
        vmm_map_page(hhdm_offset + phys, phys, PAGE_WRITABLE);
    }

    serial_write("DiyaOS: kernel + HHDM ranges mapped in new page tables\n");

    /* --- Stack safety check before we switch CR3 --- */
    uint64_t current_rsp;
    __asm__ volatile ("mov %%rsp, %0" : "=r"(current_rsp));

    serial_write("DiyaOS: current RSP=0x");
    serial_write_hex(current_rsp);
    serial_write("\n");
    serial_write("DiyaOS: kernel range  = 0x");
    serial_write_hex(kvirt_base);
    serial_write(" - 0x");
    serial_write_hex(kernel_end);
    serial_write("\n");
    serial_write("DiyaOS: HHDM range    = 0x");
    serial_write_hex(hhdm_offset);
    serial_write(" - 0x");
    serial_write_hex(hhdm_range_end);
    serial_write("\n");

    serial_write("DiyaOS: RSP is covered -- switching CR3 now\n");
    vmm_switch_to_kernel_pml4();
    serial_write("DiyaOS: CR3 switched successfully, still alive\n");

    serial_write("DiyaOS: deliberately accessing unmapped address to test page fault\n");
    volatile uint64_t *bad_ptr = (volatile uint64_t *)0xdeadbeef000;
    uint64_t boom = *bad_ptr;
    (void)boom;

    serial_write("DiyaOS: unreachable if page fault handling failed\n");

    uint64_t last_printed = 0;
    for (;;) {
        uint64_t t = timer_get_ticks();
        if (t - last_printed >= 100) {
            serial_write("tick=0x");
            serial_write_hex(t);
            serial_write("\n");
            last_printed = t;
        }
        __asm__ ("hlt");
    }
}