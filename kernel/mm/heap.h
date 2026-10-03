#ifndef HEAP_H
#define HEAP_H

#include <stdint.h>

#define KHEAP_START 0xffffffffc0000000ULL
#define KHEAP_SIZE  0x10000000ULL  /* 256 MiB reserved virtual range */

void heap_init(void);
void *kmalloc(uint64_t size);

/* Called by the page fault handler when a fault lands inside the heap
 * range -- allocates and maps the single faulting page. Returns 1 if
 * handled, 0 if the address wasn't actually in the heap range. */
int heap_handle_page_fault(uint64_t faulting_addr);

#endif