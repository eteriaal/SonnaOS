#include "gdt.h"
#include <stdint.h>

#define GDT_ENTRIES 6

static struct gdt_entry gdt[GDT_ENTRIES] __attribute__((aligned(16))) = {0};
static struct gdt_ptr gp;

static void load_gdt(uint64_t gdt_ptr_addr) {
    asm volatile (
        "lgdt (%0)\n"
        "mov $0x10, %%ax\n"
        "mov %%ax, %%ds\n"
        "mov %%ax, %%es\n"
        "mov %%ax, %%fs\n"
        "mov %%ax, %%gs\n"
        "mov %%ax, %%ss\n"
        "pushq $0x08\n"
        "lea 1f(%%rip), %%rax\n"
        "pushq %%rax\n"
        "lretq\n"
        "1:\n"
        :
        : "r"(gdt_ptr_addr)
        : "rax", "memory", "cc"
    );
}

void init_gdt(void) {
    // 0: Null descriptor
    // 1: Kernel code (0x08)
    // 2: Kernel data (0x10)
    // 3: User code (0x1B = 0x18 | 3)
    // 4: User data (0x23 = 0x20 | 3)

    gdt[1].limit_low = 0;
    gdt[1].base_low = 0;
    gdt[1].base_mid = 0;
    gdt[1].access = 0x9A;
    gdt[1].granularity = 0x20;
    gdt[1].base_high = 0;

    gdt[2].limit_low = 0;
    gdt[2].base_low = 0;
    gdt[2].base_mid = 0;
    gdt[2].access = 0x92;
    gdt[2].granularity = 0x00;
    gdt[2].base_high = 0;

    gdt[3].limit_low = 0;
    gdt[3].base_low = 0;
    gdt[3].base_mid = 0;
    gdt[3].access = 0xFA;
    gdt[3].granularity = 0x20;
    gdt[3].base_high = 0;

    gdt[4].limit_low = 0;
    gdt[4].base_low = 0;
    gdt[4].base_mid = 0;
    gdt[4].access = 0xF2;
    gdt[4].granularity = 0x00;
    gdt[4].base_high = 0;

    // GDTR
    gp.limit = sizeof(gdt) - 1;
    gp.base  = (uint64_t)&gdt;

    load_gdt((uint64_t)&gp);
}