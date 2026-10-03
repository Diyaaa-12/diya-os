#include "vmm.h"
#include "pmm.h"
#include "../drivers/serial.h"

#define PAGE_ADDR_MASK 0x000FFFFFFFFFF000ULL
#define ENTRIES_PER_TABLE 512

static uint64_t hhdm_off;
static uint64_t kernel_pml4_phys;

static inline uint64_t *phys_to_virt(uint64_t phys)
{
    return (uint64_t *)(phys + hhdm_off);
}

static void zero_table(uint64_t *table_virt)
{
    for (int i = 0; i < ENTRIES_PER_TABLE; i++) {
        table_virt[i] = 0;
    }
}

/* Walks one level: if the entry at `index` isn't present, allocates a fresh
 * physical frame for the next-level table, zeroes it, and wires it in.
 * Returns a pointer (via HHDM) to the next-level table, ready to index into. */
static uint64_t *get_or_create_table(uint64_t *table_virt, uint64_t index)
{
    if (!(table_virt[index] & PAGE_PRESENT)) {
        void *new_table_phys = pmm_alloc_frame();
        uint64_t *new_table_virt = phys_to_virt((uint64_t)new_table_phys);
        zero_table(new_table_virt);
        table_virt[index] = (uint64_t)new_table_phys | PAGE_PRESENT | PAGE_WRITABLE;
    }
    return phys_to_virt(table_virt[index] & PAGE_ADDR_MASK);
}

void vmm_init(uint64_t hhdm_offset)
{
    hhdm_off = hhdm_offset;

    void *pml4_phys = pmm_alloc_frame();
    kernel_pml4_phys = (uint64_t)pml4_phys;

    uint64_t *pml4_virt = phys_to_virt(kernel_pml4_phys);
    zero_table(pml4_virt);

    serial_write("DiyaOS: VMM init -- new PML4 at phys=0x");
    serial_write_hex(kernel_pml4_phys);
    serial_write("\n");
}

void vmm_map_page(uint64_t virt_addr, uint64_t phys_addr, uint64_t flags)
{
    uint64_t pml4_idx = (virt_addr >> 39) & 0x1FF;
    uint64_t pdpt_idx = (virt_addr >> 30) & 0x1FF;
    uint64_t pd_idx   = (virt_addr >> 21) & 0x1FF;
    uint64_t pt_idx   = (virt_addr >> 12) & 0x1FF;

    uint64_t *pml4 = phys_to_virt(kernel_pml4_phys);
    uint64_t *pdpt = get_or_create_table(pml4, pml4_idx);
    uint64_t *pd   = get_or_create_table(pdpt, pdpt_idx);
    uint64_t *pt   = get_or_create_table(pd, pd_idx);

    pt[pt_idx] = (phys_addr & PAGE_ADDR_MASK) | flags | PAGE_PRESENT;
}

void vmm_switch_to_kernel_pml4(void)
{
    __asm__ volatile ("mov %0, %%cr3" : : "r"(kernel_pml4_phys) : "memory");
}