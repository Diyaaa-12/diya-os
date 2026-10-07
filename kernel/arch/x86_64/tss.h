#ifndef TSS_H
#define TSS_H

#include <stdint.h>

void tss_init(uint64_t kernel_stack_top);
uint64_t tss_get_address(void);
uint32_t tss_get_size(void);

#endif