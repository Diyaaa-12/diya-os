#include "scheduler.h"

#define MAX_TASKS 8

static struct task *ready_queue[MAX_TASKS];
static int task_count = 0;
static int current_index = 0;
static volatile uint64_t switch_count = 0;

void scheduler_add_task(struct task *t)
{
    if (task_count < MAX_TASKS) {
        ready_queue[task_count] = t;
        task_count++;
    }
}

/* Called from the timer IRQ handler. Picks the next task in round-robin
 * order and switches to it -- the currently running task has no idea
 * this is happening; from its perspective, time just "skips forward"
 * on its next instruction after it resumes. */
void scheduler_tick(void)
{
    if (task_count == 0) {
        return;
    }

    current_index = (current_index + 1) % task_count;
    switch_count++;
    task_switch_to(ready_queue[current_index]);
}

uint64_t scheduler_get_switch_count(void)
{
    return switch_count;
}