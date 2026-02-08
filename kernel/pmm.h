#ifndef KERNEL_PMM_H
#define KERNEL_PMM_H

#include <stdint.h>
#include <stddef.h>
#include "limine.h"

#define PAGE_SIZE 4096ULL

void pmm_init(const struct limine_memmap_response *memmap, uint64_t hhdm_offset);
void* pmm_alloc(void); // one page
void* pmm_alloc_frames(size_t count); // frames
void pmm_free(void* phys_addr); // one page
void pmm_free_frames(void* phys_addr, size_t count); // frames
size_t pmm_get_total_frames(void);
size_t pmm_get_free_frames(void);
size_t pmm_get_usable_frames(void);
size_t pmm_get_used_frames(void);

#endif