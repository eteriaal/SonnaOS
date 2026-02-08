// Physical memory manager using a simple bitmap
#include "pmm.h"
#include <stdbool.h>

static uint8_t *pmm_bitmap; 
static size_t pmm_bitmap_bytes;
static size_t pmm_bitmap_frames;
static size_t pmm_total_frames_count;
static size_t pmm_usable_frames_count;
static size_t pmm_free_frames_count;
static size_t pmm_used_frames_count;

static uint64_t align_up(uint64_t value, uint64_t align) {
    return (value + align - 1) & ~(align - 1);
}

static uint64_t align_down(uint64_t value, uint64_t align) {
    return value & ~(align - 1);
}

static bool is_ram_type(uint64_t type) {
    return type == LIMINE_MEMMAP_USABLE
        || type == LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE
        || type == LIMINE_MEMMAP_ACPI_RECLAIMABLE;
}

// fills bitmap using rep stosb - safe for now. Using memset is currently unsafe, because IDT is not set
static void pmm_fill_bitmap(uint8_t value) {
    uint8_t *ptr = pmm_bitmap;
    size_t bytes = pmm_bitmap_bytes;

    __asm__ volatile (
        "mov %0, %%rdi\n"
        "mov %1, %%rcx\n"
        "test %%rcx, %%rcx\n"
        "jz 1f\n"
        "mov %2, %%al\n"
        "rep stosb\n"
        "1:\n"
        :
        : "r"(ptr), "r"(bytes), "r"(value)
        : "rdi", "rcx", "al", "memory"
    );
}

static void pmm_set_frame(size_t frame) {
    pmm_bitmap[frame / 8] |= (uint8_t)(1u << (frame % 8));
}

static void pmm_clear_frame(size_t frame) {
    pmm_bitmap[frame / 8] &= (uint8_t)~(1u << (frame % 8));
}

static bool pmm_test_frame(size_t frame) {
    return (pmm_bitmap[frame / 8] & (uint8_t)(1u << (frame % 8))) != 0;
}

void pmm_init(const struct limine_memmap_response *memmap, uint64_t hhdm_offset) {
    uint64_t max_addr = 0;
    size_t usable_frames = 0;
    size_t total_ram_frames = 0;

    // calculate bitmap size, total memory, usable memory.
    for (size_t i = 0; i < memmap->entry_count; i++) {
        struct limine_memmap_entry *entry = memmap->entries[i];

        if (is_ram_type(entry->type)) {
            uint64_t start = align_up(entry->base, PAGE_SIZE);
            uint64_t end   = align_down(entry->base + entry->length, PAGE_SIZE);
            if (end > start) {
                total_ram_frames += (size_t)((end - start) / PAGE_SIZE);
            }
            if (end > max_addr) {
                max_addr = end;
            }
        }

        if (entry->type == LIMINE_MEMMAP_USABLE) {
            uint64_t start = align_up(entry->base, PAGE_SIZE);
            uint64_t end   = align_down(entry->base + entry->length, PAGE_SIZE);
            if (end > start) {
                usable_frames += (size_t)((end - start) / PAGE_SIZE);
            }
        }
    }

    pmm_bitmap_frames = align_up(max_addr, PAGE_SIZE) / PAGE_SIZE;
    pmm_total_frames_count = total_ram_frames;
    pmm_usable_frames_count = usable_frames;

    pmm_bitmap_bytes = (pmm_bitmap_frames + 7) / 8;
    size_t bitmap_size = align_up(pmm_bitmap_bytes, PAGE_SIZE);


    // find place for bitmap
    uint64_t bitmap_phys = 0;
    bool bitmap_found = false;

    for (size_t i = 0; i < memmap->entry_count; i++) {
        struct limine_memmap_entry *entry = memmap->entries[i];
        if (entry->type != LIMINE_MEMMAP_USABLE) continue;

        uint64_t start = align_up(entry->base, PAGE_SIZE);
        uint64_t end   = entry->base + entry->length;
        if (end <= start) continue;

        if (end - start >= bitmap_size) {
            bitmap_phys = start;
            bitmap_found = true;
            break;
        }
    }

    if (!bitmap_found) {
        pmm_bitmap = NULL;
        pmm_free_frames_count = 0;
        pmm_used_frames_count = pmm_usable_frames_count;
        return;
    }

    pmm_bitmap = (uint8_t *)(bitmap_phys + hhdm_offset);

    // initially everything marked as used
    pmm_fill_bitmap(0xFF);

    pmm_free_frames_count = 0;
    pmm_used_frames_count = pmm_usable_frames_count;

    // mark usable regions as free
    for (size_t i = 0; i < memmap->entry_count; i++) {
        struct limine_memmap_entry *entry = memmap->entries[i];
        if (entry->type != LIMINE_MEMMAP_USABLE) continue;

        uint64_t base = align_up(entry->base, PAGE_SIZE);
        uint64_t end  = align_down(entry->base + entry->length, PAGE_SIZE);
        for (uint64_t addr = base; addr < end; addr += PAGE_SIZE) {
            size_t frame = (size_t)(addr / PAGE_SIZE);
            if (frame < pmm_bitmap_frames && pmm_test_frame(frame)) {
                pmm_clear_frame(frame);
                pmm_free_frames_count++;
                pmm_used_frames_count--;
            }
        }
    }

    // protect frame 0
    pmm_set_frame(0);
    if (pmm_free_frames_count > 0) {
        pmm_free_frames_count--;
        pmm_used_frames_count++;
    }

    // protect bitmap
    uint64_t bitmap_end = bitmap_phys + bitmap_size;
    for (uint64_t addr = bitmap_phys; addr < bitmap_end; addr += PAGE_SIZE) {
        size_t frame = (size_t)(addr / PAGE_SIZE);
        if (frame < pmm_bitmap_frames && !pmm_test_frame(frame)) {
            pmm_set_frame(frame);
            pmm_free_frames_count--;
            pmm_used_frames_count++;
        }
    }
}

void *pmm_alloc(void) {
    return pmm_alloc_frames(1);
}

// "first-fit" allocator - simple but slow.
void *pmm_alloc_frames(size_t count) {
    if (count == 0 || pmm_free_frames_count < count) return NULL;

    size_t run_start = 0;
    size_t run_length = 0;

    for (size_t frame = 0; frame < pmm_bitmap_frames; frame++) {
        if (!pmm_test_frame(frame)) {
            if (run_length == 0) run_start = frame;
            run_length++;
            if (run_length == count) {
                for (size_t i = 0; i < count; i++) pmm_set_frame(run_start + i);
                pmm_free_frames_count -= count;
                pmm_used_frames_count += count;
                return (void *)(run_start * PAGE_SIZE);
            }
        } else {
            run_length = 0;
        }
    }

    return NULL;
}

void pmm_free(void *phys_addr) {
    pmm_free_frames(phys_addr, 1);
}

void pmm_free_frames(void *phys_addr, size_t count) {
    if (count == 0) return;

    uint64_t addr = (uint64_t)phys_addr;
    size_t frame = (size_t)(addr / PAGE_SIZE);

    for (size_t i = 0; i < count; i++) {
        size_t cur = frame + i;
        if (cur >= pmm_bitmap_frames) break;
        if (pmm_test_frame(cur)) {
            pmm_clear_frame(cur);
            pmm_free_frames_count++;
            pmm_used_frames_count--;
        }
    }
}

size_t pmm_get_total_frames(void) {
    return pmm_total_frames_count;
}

size_t pmm_get_usable_frames(void) {
    return pmm_usable_frames_count;
}

size_t pmm_get_free_frames(void) {
    return pmm_free_frames_count;
}

size_t pmm_get_used_frames(void) {
    return pmm_used_frames_count;
}