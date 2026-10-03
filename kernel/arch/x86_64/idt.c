#include <stdint.h>
#include "idt.h"

struct idt_entry {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t  ist;         /* Interrupt Stack Table index; 0 = not used yet */
    uint8_t  type_attr;
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t zero;        /* reserved, must be 0 */
} __attribute__((packed));

struct idt_ptr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

#define IDT_ENTRIES 256

static struct idt_entry idt[IDT_ENTRIES];
static struct idt_ptr   idtp;

/* Defined in idt_flush.asm */
extern void idt_flush(uint64_t idtp_addr);

/* Array of the 32 exception-stub entry addresses, defined in isr_stubs.asm.
 * We declare it here as "extern" and fill the IDT from it in a loop. */
extern uint64_t isr_stub_table[32];
extern uint64_t irq_stub_table[16];

static void idt_set_entry(int vector, uint64_t handler_addr)
{
    idt[vector].offset_low  = handler_addr & 0xFFFF;
    idt[vector].selector    = 0x08;             /* kernel code segment */
    idt[vector].ist         = 0;
    idt[vector].type_attr   = 0x8E;              /* present | ring0 | 64-bit interrupt gate */
    idt[vector].offset_mid  = (handler_addr >> 16) & 0xFFFF;
    idt[vector].offset_high = (handler_addr >> 32) & 0xFFFFFFFF;
    idt[vector].zero        = 0;
}

extern uint64_t isr_stub_table[32];
extern uint64_t irq_stub_table[16];

void idt_init(void)
{
    for (int i = 0; i < 32; i++) {
        idt_set_entry(i, isr_stub_table[i]);
    }

    for (int i = 0; i < 16; i++) {
        idt_set_entry(32 + i, irq_stub_table[i]);
    }

    idtp.limit = sizeof(idt) - 1;
    idtp.base  = (uint64_t)&idt;

    idt_flush((uint64_t)&idtp);
}