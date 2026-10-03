#include "pmm.h"
#include "../drivers/serial.h"

#define FRAME_SIZE 4096

static uint8_t *bitmap;
static uint64_t total_frames;
static uint64_t free_frames;
static uint64_t hhdm_off;
static uint64_t search_hint = 0; /* speeds up repeated allocation -- see note below */

static inline void bitmap_set(uint64_t frame_idx)
{
    bitmap[frame_idx / 8] |= (1 << (frame_idx % 8));
}

static inline void bitmap_clear(uint64_t frame_idx)
{
    bitmap[frame_idx / 8] &= ~(1 << (frame_idx % 8));
}

static inline int bitmap_test(uint64_t frame_idx)
{
    return (bitmap[frame_idx / 8] >> (frame_idx % 8)) & 1;
}

void pmm_init(struct limine_memmap_response *memmap, uint64_t hhdm_offset)
{
    hhdm_off = hhdm_offset;

    /* Step 1: find the highest usable physical address, to size the bitmap. */
    uint64_t highest_addr = 0;
    for (uint64_t i = 0; i < memmap->entry_count; i++) {
        struct limine_memmap_entry *entry = memmap->entries[i];
        if (entry->type == LIMINE_MEMMAP_USABLE) {
            uint64_t end = entry->base + entry->length;
            if (end > highest_addr) {
                highest_addr = end;
            }
        }
    }

    total_frames = highest_addr / FRAME_SIZE;
    uint64_t bitmap_size_bytes = (total_frames + 7) / 8;

    /* Step 2: find a usable region big enough to hold the bitmap itself. */
    uint64_t bitmap_phys_addr = 0;
    for (uint64_t i = 0; i < memmap->entry_count; i++) {
        struct limine_memmap_entry *entry = memmap->entries[i];
        if (entry->type == LIMINE_MEMMAP_USABLE && entry->length >= bitmap_size_bytes) {
            bitmap_phys_addr = entry->base;
            break;
        }
    }

    bitmap = (uint8_t *)(bitmap_phys_addr + hhdm_off);

    /* Step 3: start by marking EVERYTHING used. We'll explicitly free only
     * the usable regions next -- safer default than the reverse, since any
     * memmap type we don't recognise correctly stays marked used. */
    for (uint64_t i = 0; i < bitmap_size_bytes; i++) {
        bitmap[i] = 0xFF;
    }
    free_frames = 0;

    /* Step 4: free the usable regions. */
    for (uint64_t i = 0; i < memmap->entry_count; i++) {
        struct limine_memmap_entry *entry = memmap->entries[i];
        if (entry->type != LIMINE_MEMMAP_USABLE) {
            continue;
        }
        uint64_t start_frame = entry->base / FRAME_SIZE;
        uint64_t frame_count = entry->length / FRAME_SIZE;
        for (uint64_t f = 0; f < frame_count; f++) {
            bitmap_clear(start_frame + f);
            free_frames++;
        }
    }

    /* Step 5: re-reserve the frames the bitmap itself occupies -- it's
     * sitting inside a region we just marked free in step 4. */
    uint64_t bitmap_start_frame = bitmap_phys_addr / FRAME_SIZE;
    uint64_t bitmap_frame_count = (bitmap_size_bytes + FRAME_SIZE - 1) / FRAME_SIZE;
    for (uint64_t f = 0; f < bitmap_frame_count; f++) {
        if (!bitmap_test(bitmap_start_frame + f)) {
            bitmap_set(bitmap_start_frame + f);
            free_frames--;
        }
    }

    serial_write("DiyaOS: PMM init -- total_frames=0x");
    serial_write_hex(total_frames);
    serial_write(" free_frames=0x");
    serial_write_hex(free_frames);
    serial_write("\n");
}

void *pmm_alloc_frame(void)
{
    for (uint64_t i = search_hint; i < total_frames; i++) {
        if (!bitmap_test(i)) {
            bitmap_set(i);
            free_frames--;
            search_hint = i + 1;
            return (void *)(i * FRAME_SIZE);
        }
    }
    return 0; /* out of memory -- caller must check for NULL */
}

void pmm_free_frame(void *frame_phys_addr)
{
    uint64_t idx = (uint64_t)frame_phys_addr / FRAME_SIZE;
    if (bitmap_test(idx)) {
        bitmap_clear(idx);
        free_frames++;
        if (idx < search_hint) {
            search_hint = idx; /* a freed-earlier frame might be found faster next time */
        }
    }
}

uint64_t pmm_get_free_frame_count(void)
{
    return free_frames;
}