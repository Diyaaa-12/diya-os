#include <stdint.h>
#include "gdt.h"
#include "tss.h"

struct gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_mid;
    uint8_t  access;
    uint8_t  granularity;
    uint8_t  base_high;
} __attribute__((packed));

struct tss_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_mid;
    uint8_t  access;
    uint8_t  granularity;
    uint8_t  base_high;
    uint32_t base_upper;
    uint32_t reserved;
} __attribute__((packed));

/* Combined into one packed struct -- guarantees these land in memory in
 * exactly this order with no padding, which the GDT table format requires.
 * Two separate global variables would NOT give us this guarantee. */
struct gdt_table {
    struct gdt_entry entries[5]; /* 0:null 1:kcode 2:kdata 3:ucode 4:udata */
    struct tss_entry tss;
} __attribute__((packed));

static struct gdt_table gdt;

struct gdt_ptr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

static struct gdt_ptr gdtp;

extern void gdt_flush(uint64_t gdtp_addr);
extern void tss_flush(void);

static void gdt_set_entry(int index, uint8_t access, uint8_t granularity)
{
    gdt.entries[index].limit_low   = 0;
    gdt.entries[index].base_low    = 0;
    gdt.entries[index].base_mid    = 0;
    gdt.entries[index].base_high   = 0;
    gdt.entries[index].access      = access;
    gdt.entries[index].granularity = granularity;
}

static void gdt_set_tss(uint64_t base, uint32_t limit)
{
    gdt.tss.limit_low   = limit & 0xFFFF;
    gdt.tss.base_low    = base & 0xFFFF;
    gdt.tss.base_mid    = (base >> 16) & 0xFF;
    gdt.tss.access      = 0x89; /* present | ring0 | type=0x9 (64-bit TSS) */
    gdt.tss.granularity = (limit >> 16) & 0x0F;
    gdt.tss.base_high   = (base >> 24) & 0xFF;
    gdt.tss.base_upper  = (base >> 32) & 0xFFFFFFFF;
    gdt.tss.reserved    = 0;
}

void gdt_init(void)
{
    gdt_set_entry(0, 0x00, 0x00);
    gdt_set_entry(1, 0x9A, 0x20); /* kernel code */
    gdt_set_entry(2, 0x92, 0x00); /* kernel data */
    gdt_set_entry(3, 0xFA, 0x20); /* user code */
    gdt_set_entry(4, 0xF2, 0x00); /* user data */

    gdt_set_tss(tss_get_address(), tss_get_size() - 1);

    gdtp.limit = sizeof(struct gdt_table) - 1;
    gdtp.base  = (uint64_t)&gdt;

    gdt_flush((uint64_t)&gdtp);
    tss_flush();
}