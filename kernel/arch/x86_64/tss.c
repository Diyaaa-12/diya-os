#include "tss.h"

/* x86_64 TSS -- much smaller than the legacy 32-bit TSS. We only care
 * about RSP0: the kernel stack the CPU switches to automatically when
 * an interrupt/exception occurs while running in ring 3. */
struct tss {
    uint32_t reserved0;
    uint64_t rsp0;
    uint64_t rsp1;
    uint64_t rsp2;
    uint64_t reserved1;
    uint64_t ist1;
    uint64_t ist2;
    uint64_t ist3;
    uint64_t ist4;
    uint64_t ist5;
    uint64_t ist6;
    uint64_t ist7;
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t iopb_offset;
} __attribute__((packed));

static struct tss tss;

void tss_init(uint64_t kernel_stack_top)
{
    for (uint64_t i = 0; i < sizeof(struct tss); i++) {
        ((uint8_t *)&tss)[i] = 0;
    }

    tss.rsp0 = kernel_stack_top;
    tss.iopb_offset = sizeof(struct tss); /* no I/O permission bitmap -- set offset past the end */
}

/* Exposed so gdt.c can build the TSS descriptor pointing at this struct. */
uint64_t tss_get_address(void)
{
    return (uint64_t)&tss;
}

uint32_t tss_get_size(void)
{
    return sizeof(struct tss);
}