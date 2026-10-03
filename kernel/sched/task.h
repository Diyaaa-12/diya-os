#ifndef TASK_H
#define TASK_H

#include <stdint.h>

struct task {
    uint64_t rsp;
};

typedef void (*task_entry_fn)(void);

struct task *task_create(task_entry_fn entry);
void task_set_current(struct task *t);
void task_switch_to(struct task *next);

#endif