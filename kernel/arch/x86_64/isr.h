#ifndef ISR_H
#define ISR_H

#include <stdint.h>

/* Layout must exactly match the push order in isr_common_stub (isr_stubs.asm).
 * Pushes happen in reverse, so the LAST thing pushed (r15) is at the LOWEST
 * address, matching this struct's FIRST field. */
struct interrupt_frame {
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
    uint64_t vector_num;
    uint64_t error_code;
    uint64_t rip, cs, rflags, rsp, ss;
} __attribute__((packed));

void isr_handler(struct interrupt_frame *frame);

#endif