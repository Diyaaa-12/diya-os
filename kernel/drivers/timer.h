#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

void pit_init(uint32_t frequency_hz);
void timer_tick(void);
uint64_t timer_get_ticks(void);

#endif