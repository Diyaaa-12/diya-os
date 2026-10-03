#include <stdint.h>
#include "../third_party/limine/limine.h"
#include "drivers/serial.h"
#include "drivers/pic.h"
#include "drivers/timer.h"
#include "arch/x86_64/gdt.h"
#include "arch/x86_64/idt.h"
#include "mm/pmm.h"

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

__attribute__((used, section(".limine_requests_start")))
static volatile uint64_t limine_requests_start_marker[] = LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests_end")))
static volatile uint64_t limine_requests_end_marker[] = LIMINE_REQUESTS_END_MARKER;

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
    serial_write("DiyaOS: memory map entries=0x");
    serial_write_hex(memmap->entry_count);
    serial_write("\n");

    uint64_t usable_total = 0;
    for (uint64_t i = 0; i < memmap->entry_count; i++) {
        struct limine_memmap_entry *entry = memmap->entries[i];
        serial_write("  base=0x");
        serial_write_hex(entry->base);
        serial_write(" len=0x");
        serial_write_hex(entry->length);
        serial_write(" type=0x");
        serial_write_hex(entry->type);
        serial_write("\n");

        if (entry->type == LIMINE_MEMMAP_USABLE) {
            usable_total += entry->length;
        }
    }
    serial_write("DiyaOS: total usable bytes=0x");
    serial_write_hex(usable_total);
    serial_write("\n");

    pmm_init(memmap, hhdm_request.response->offset);

    void *frame1 = pmm_alloc_frame();
    void *frame2 = pmm_alloc_frame();
    serial_write("DiyaOS: alloc1=0x");
    serial_write_hex((uint64_t)frame1);
    serial_write(" alloc2=0x");
    serial_write_hex((uint64_t)frame2);
    serial_write("\n");

    pmm_free_frame(frame1);
    void *frame3 = pmm_alloc_frame();
    serial_write("DiyaOS: after free+realloc, frame3=0x");
    serial_write_hex((uint64_t)frame3);
    serial_write(" (should equal alloc1 if reuse worked)\n");

    serial_write("DiyaOS: free frames remaining=0x");
    serial_write_hex(pmm_get_free_frame_count());
    serial_write("\n");

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