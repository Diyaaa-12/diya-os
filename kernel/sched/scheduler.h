#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdint.h>
#include "task.h"

void scheduler_add_task(struct task *t);
void scheduler_tick(void);
uint64_t scheduler_get_switch_count(void);

#endif