#include "timer.h"

#define PIT_CHANNEL0 0x40
#define PIT_COMMAND  0x43
#define PIT_BASE_FREQ 1193182

static inline void outb(uint16_t port, uint8_t val)
{
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static volatile uint64_t ticks = 0;

void pit_init(uint32_t frequency_hz)
{
    uint32_t divisor = PIT_BASE_FREQ / frequency_hz;

    /* 0x36 = channel 0, lobyte/hibyte access, mode 3 (square wave) */
    outb(PIT_COMMAND, 0x36);
    outb(PIT_CHANNEL0, (uint8_t)(divisor & 0xFF));
    outb(PIT_CHANNEL0, (uint8_t)((divisor >> 8) & 0xFF));
}

void timer_tick(void)
{
    ticks++;
}

uint64_t timer_get_ticks(void)
{
    return ticks;
}