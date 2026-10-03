#include "task.h"
#include "../mm/heap.h"

#define TASK_STACK_SIZE 0x4000  /* 16 KiB per kernel thread stack */

extern void context_switch(uint64_t *old_rsp_ptr, uint64_t new_rsp);

static struct task *current_task;

extern void task_trampoline(void);

struct task *task_create(task_entry_fn entry)
{
    struct task *t = (struct task *)kmalloc(sizeof(struct task));

    uint8_t *stack_mem = (uint8_t *)kmalloc(TASK_STACK_SIZE);
    uint64_t *stack_top = (uint64_t *)(stack_mem + TASK_STACK_SIZE);

    *(--stack_top) = (uint64_t)task_trampoline; /* "return address" for the ret */
    *(--stack_top) = 0;               /* rbp */
    *(--stack_top) = (uint64_t)entry; /* rbx -- trampoline reads entry from here */
    *(--stack_top) = 0;               /* r12 */
    *(--stack_top) = 0;               /* r13 */
    *(--stack_top) = 0;               /* r14 */
    *(--stack_top) = 0;               /* r15 */

    t->rsp = (uint64_t)stack_top;
    return t;
}

void task_set_current(struct task *t)
{
    current_task = t;
}

void task_switch_to(struct task *next)
{
    struct task *prev = current_task;
    current_task = next;
    context_switch(&prev->rsp, next->rsp);
}