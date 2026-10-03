#ifndef VMM_H
#define VMM_H

#include <stdint.h>

#define PAGE_PRESENT  0x1
#define PAGE_WRITABLE 0x2
#define PAGE_USER     0x4

void vmm_init(uint64_t hhdm_offset);
void vmm_map_page(uint64_t virt_addr, uint64_t phys_addr, uint64_t flags);
void vmm_switch_to_kernel_pml4(void);

#endif