#ifndef PMM_H
#define PMM_H

#include <stdint.h>
#include "../../third_party/limine/limine.h"

void pmm_init(struct limine_memmap_response *memmap, uint64_t hhdm_offset);
void *pmm_alloc_frame(void);
void pmm_free_frame(void *frame_phys_addr);
uint64_t pmm_get_free_frame_count(void);

#endif