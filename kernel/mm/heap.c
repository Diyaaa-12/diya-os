#include "heap.h"
#include "pmm.h"
#include "vmm.h"

static uint64_t heap_next = KHEAP_START;

void heap_init(void)
{
    heap_next = KHEAP_START;
}

void *kmalloc(uint64_t size)
{
    /* Simple 16-byte alignment -- a reasonable default for general-purpose
     * allocations, avoids misaligned access issues for most struct types. */
    uint64_t aligned_size = (size + 15) & ~((uint64_t)15);

    void *result = (void *)heap_next;
    heap_next += aligned_size;

    /* We do NOT map anything here. The memory is accessed lazily -- the
     * first read/write to this region triggers a page fault, which
     * heap_handle_page_fault() resolves on demand. */
    return result;
}

int heap_handle_page_fault(uint64_t faulting_addr)
{
    if (faulting_addr < KHEAP_START || faulting_addr >= KHEAP_START + KHEAP_SIZE) {
        return 0; /* not ours -- let the caller treat it as a real fault */
    }

    uint64_t page_addr = faulting_addr & ~((uint64_t)0xFFF); /* round down to page boundary */

    void *frame = pmm_alloc_frame();
    if (!frame) {
        return 0; /* out of physical memory -- nothing we can do here */
    }

    vmm_map_page(page_addr, (uint64_t)frame, PAGE_WRITABLE);
    return 1;
}