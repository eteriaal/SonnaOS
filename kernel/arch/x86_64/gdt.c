#include <arch/x86_64/gdt.h>

#define GDT_ENTRIES 7

static struct gdt_entry gdt[GDT_ENTRIES] __attribute__((aligned(16))) = {0};
static struct gdt_ptr gp;

#define IST_STACK_SIZE 8192
static uint8_t ist1_stack[IST_STACK_SIZE] __attribute__((aligned(16)));
static uint8_t ist2_stack[IST_STACK_SIZE] __attribute__((aligned(16)));
static uint8_t ist3_stack[IST_STACK_SIZE] __attribute__((aligned(16)));

#define KERNEL_STACK_SIZE  (16 * 4096)
static uint8_t kernel_main_stack[KERNEL_STACK_SIZE] __attribute__((aligned(16)));

static void load_gdt(uint64_t gdt_ptr_addr) {
    asm volatile (
        "lgdt (%0)\n"
        "mov $0x10, %%ax\n"
        "mov %%ax, %%ds\n"
        "mov %%ax, %%es\n"
        "pushq $0x08\n"
        "lea 1f(%%rip), %%rax\n"
        "pushq %%rax\n"
        "lretq\n"
        "1:\n"
        "mov $0x10, %%ax\n"
        "mov %%ax, %%ss\n"
        :
        : "r"(gdt_ptr_addr)
        : "rax", "memory", "cc"
    );
}

void gdt_init(void) {
    // 0: Null descriptor
    // 1: Kernel code
    // 2: Kernel data
    // 3: User code
    // 4: User data
    // 5-6: TSS descriptor

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

    uint64_t tss_addr = (uint64_t)&tss;
    struct gdt_system_entry *tss_desc = (struct gdt_system_entry*)&gdt[5];

    tss_desc->limit_low    = sizeof(tss) - 1;
    tss_desc->base_low     = tss_addr & 0xFFFF;
    tss_desc->base_mid     = (tss_addr >> 16) & 0xFF;
    tss_desc->access       = 0x89;
    tss_desc->granularity  = 0x00;
    tss_desc->base_high    = (tss_addr >> 24) & 0xFF;
    tss_desc->base_upper   = tss_addr >> 32;
    tss_desc->reserved     = 0;
    
    tss.rsp0 = (uint64_t)&kernel_main_stack[KERNEL_STACK_SIZE];

    tss.ist1 = (uint64_t)&ist1_stack[IST_STACK_SIZE];
    tss.ist2 = (uint64_t)&ist2_stack[IST_STACK_SIZE];
    tss.ist3 = (uint64_t)&ist3_stack[IST_STACK_SIZE];

    gp.limit = sizeof(gdt) - 1;
    gp.base  = (uint64_t)&gdt;

    load_gdt((uint64_t)&gp);

    uint16_t tss_sel = 0x28;
    asm volatile ("ltr %0" : : "r"(tss_sel) : "memory");
}