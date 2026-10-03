#include "isr.h"           /* reuse struct interrupt_frame */
#include "../../drivers/pic.h"
#include "../../drivers/timer.h"

void irq_handler(struct interrupt_frame *frame)
{
    uint8_t irq = (uint8_t)(frame->vector_num - 32);

    if (irq == 0) {
        timer_tick();
    }

    pic_send_eoi(irq);
}