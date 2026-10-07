#include <stdint.h>
#include "idt.h"

struct idt_entry {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t  ist;
    uint8_t  type_attr;
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t zero;
} __attribute__((packed));

struct idt_ptr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

#define IDT_ENTRIES 256

static struct idt_entry idt[IDT_ENTRIES];
static struct idt_ptr   idtp;

extern void idt_flush(uint64_t idtp_addr);
extern uint64_t isr_stub_table[32];
extern uint64_t irq_stub_table[16];
extern void syscall_entry(void);

static void idt_set_entry(int vector, uint64_t handler_addr, uint8_t dpl)
{
    idt[vector].offset_low  = handler_addr & 0xFFFF;
    idt[vector].selector    = 0x08;
    idt[vector].ist         = 0;
    /* present(0x80) | DPL(bits 5-6) | type=0xE (64-bit interrupt gate) */
    idt[vector].type_attr   = 0x80 | ((dpl & 0x3) << 5) | 0x0E;
    idt[vector].offset_mid  = (handler_addr >> 16) & 0xFFFF;
    idt[vector].offset_high = (handler_addr >> 32) & 0xFFFFFFFF;
    idt[vector].zero        = 0;
}

void idt_init(void)
{
    for (int i = 0; i < 32; i++) {
        idt_set_entry(i, isr_stub_table[i], 0);
    }

    for (int i = 0; i < 16; i++) {
        idt_set_entry(32 + i, irq_stub_table[i], 0);
    }

    idt_set_entry(0x80, (uint64_t)syscall_entry, 3);

    idtp.limit = sizeof(idt) - 1;
    idtp.base  = (uint64_t)&idt;

    idt_flush((uint64_t)&idtp);
}