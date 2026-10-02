#include <stdint.h>
#include "../third_party/limine/limine.h"
#include "drivers/serial.h"

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

    for (;;) {
        __asm__ ("hlt");
    }
}