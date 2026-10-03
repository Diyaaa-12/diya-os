#include <stdint.h>
#include "../third_party/limine/limine.h"
#include "drivers/serial.h"
#include "drivers/pic.h"
#include "drivers/timer.h"
#include "arch/x86_64/gdt.h"
#include "arch/x86_64/idt.h"

__attribute__((used, section(".limine_requests")))
static volatile uint64_t limine_base_revision[] = LIMINE_BASE_REVISION(3);

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
    pit_init(100);          /* 100 Hz = tick every 10ms */
    pic_unmask_irq(0);      /* enable timer IRQ only */
    __asm__ ("sti");        /* enable interrupts globally */
    serial_write("DiyaOS: timer enabled, interrupts on\n");

    uint64_t last_printed = 0;
    for (;;) {
        uint64_t t = timer_get_ticks();
        if (t - last_printed >= 100) {   /* print roughly once per second */
            serial_write("tick=0x");
            serial_write_hex(t);
            serial_write("\n");
            last_printed = t;
        }
        __asm__ ("hlt");
    }
}