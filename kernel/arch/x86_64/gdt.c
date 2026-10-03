#include <stdint.h>
#include "gdt.h"

/* One 8-byte GDT entry, packed so the compiler doesn't insert padding. */
struct gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_mid;
    uint8_t  access;
    uint8_t  granularity;
    uint8_t  base_high;
} __attribute__((packed));

struct gdt_ptr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

/* Entry 0: null (required). Entry 1: kernel code. Entry 2: kernel data. */
static struct gdt_entry gdt[3];
static struct gdt_ptr   gdtp;

/* Defined in gdt_flush.asm — loads GDTR and reloads CS/DS/etc. */
extern void gdt_flush(uint64_t gdtp_addr);

static void gdt_set_entry(int index, uint8_t access, uint8_t granularity)
{
    /* Base and limit are unused in 64-bit flat mode; zero them explicitly. */
    gdt[index].limit_low   = 0;
    gdt[index].base_low    = 0;
    gdt[index].base_mid    = 0;
    gdt[index].base_high   = 0;
    gdt[index].access      = access;
    gdt[index].granularity = granularity;
}

void gdt_init(void)
{
    gdt_set_entry(0, 0x00, 0x00); /* null descriptor */

    /* Kernel code: present(0x80) | ring0(0x00) | descriptor(0x10) |
     * executable(0x08) | readable(0x02) = 0x9A.
     * Granularity: long-mode bit (0x20) set, rest 0. */
    gdt_set_entry(1, 0x9A, 0x20);

    /* Kernel data: present | ring0 | descriptor | writable(0x02) = 0x92. */
    gdt_set_entry(2, 0x92, 0x00);

    gdtp.limit = sizeof(gdt) - 1;
    gdtp.base  = (uint64_t)&gdt;

    gdt_flush((uint64_t)&gdtp);
}