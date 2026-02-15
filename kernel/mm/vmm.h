#ifndef ESTELLA_MM_VMM_H
#define ESTELLA_MM_VMM_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <mm/pmm.h>

#define PAGE_SIZE 4096ULL
#define HUGE_2MB  (2ULL*1024*1024)

#define PTE_PRESENT (1ULL << 0)
#define PTE_WRITE (1ULL << 1)
#define PTE_USER (1ULL << 2)
#define PTE_PWT (1ULL << 3)
#define PTE_PCD (1ULL << 4)
#define PTE_ACCESSED (1ULL << 5)
#define PTE_DIRTY (1ULL << 6)
#define PTE_HUGE (1ULL << 7)
#define PTE_GLOBAL (1ULL << 8)
#define PTE_NX (1ULL << 63)

#define PTE_KERNEL_RO PTE_PRESENT
#define PTE_KERNEL_RW (PTE_PRESENT | PTE_WRITE)
#define PTE_KERNEL_EXEC (PTE_PRESENT | PTE_WRITE)
#define PTE_KERNEL_RO_NX PTE_PRESENT
#define PTE_KERNEL_RW_NX (PTE_PRESENT | PTE_WRITE | PTE_NX)

extern uint64_t hhdm_offset;

static inline uint64_t phys_to_virt(uint64_t phys) {
    return phys + hhdm_offset;
}

static inline uint64_t virt_to_phys(uint64_t virt) {
    return virt - hhdm_offset;
}

void vmm_init(void);
bool vmm_map(uint64_t virt, uint64_t phys, uint64_t flags);
bool vmm_map_huge_2mb(uint64_t virt, uint64_t phys, uint64_t flags);
bool vmm_map_range(uint64_t virt, uint64_t phys, size_t page_count, uint64_t flags);
bool vmm_unmap(uint64_t virt);
bool vmm_unmap_huge_2mb(uint64_t virt);
bool vmm_unmap_range(uint64_t virt, size_t page_count);
uint64_t vmm_get_physical(uint64_t virt);
uint64_t vmm_get_flags(uint64_t virt);
void vmm_dump_pte(uint64_t virt);

#endif