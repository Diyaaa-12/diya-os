#include "isr.h"
#include "../../drivers/pic.h"
#include "../../drivers/timer.h"
#include "../../sched/scheduler.h"

void irq_handler(struct interrupt_frame *frame)
{
    uint8_t irq = (uint8_t)(frame->vector_num - 32);

    if (irq == 0) {
        timer_tick();

        /* CRITICAL ORDER: EOI must happen BEFORE we switch away. If we
         * switched first, the PIC would never be told "timer IRQ handled",
         * and it would withhold ALL further timer interrupts -- for every
         * task -- until this exact task happened to run again (which,
         * under preemption, might be never). Send EOI first, every time. */
        pic_send_eoi(irq);

        scheduler_tick(); /* may not return here for a while -- that's fine */
        return;
    }

    pic_send_eoi(irq);
}