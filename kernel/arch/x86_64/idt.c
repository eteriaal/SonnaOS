#include <arch/x86_64/idt.h>
#include <klib/string.h>
#include <klib/memory.h>
#include <drivers/fbtext.h>
#include <drivers/serial.h>

#define IDT_ENTRIES 256
#define IDT_INTERRUPT 0x8E

static struct idt_entry idt[IDT_ENTRIES];
static struct idtr idtr;

extern void isr0(void), isr1(void), isr2(void), isr3(void), isr4(void), isr5(void), isr6(void), isr7(void), isr8(void), isr9(void),
            isr10(void), isr11(void), isr12(void), isr13(void), isr14(void), isr15(void), isr16(void), isr17(void), isr18(void),
            isr19(void), isr20(void), isr21(void), isr22(void), isr23(void), isr24(void), isr25(void), isr26(void), isr27(void),  
            isr28(void), isr29(void), isr30(void), isr31(void);

void exception_handler(uint64_t vector, uint64_t error_code, uint64_t rip, uint64_t cs,
                       uint64_t rflags, uint64_t rsp, uint64_t ss) {
    fb_print("KERNEL PANIC!\n", 0xFF5555);
    serial_puts("KERNEL PANIC!\n");

    char buf[32] = {0};

    u64_to_dec(vector, buf);
    fb_print_value("Vector : ", buf, 0xAAAAAA, 0xFFFFFF);
    serial_puts("Vector : ");
    serial_puts(buf);
    fb_print("   ", 0xAAAAAA);
    serial_puts("   ");
    if (vector < 32) {
        fb_print("(exception)", 0xAAAAAA);
        serial_puts("(exception)");
    }
    fb_print("\n", 0);
    serial_puts("\n");

    u64_to_hex(error_code, buf);
    fb_print_value("Error  : ", buf, 0xAAAAAA, 0xFFFFFF);
    serial_puts("Error  : ");
    serial_puts(buf);
    fb_print("\n", 0);
    serial_puts("\n");

    u64_to_hex(rip, buf);
    fb_print_value("RIP    : ", buf, 0xAAAAAA, 0xFFFFFF);
    serial_puts("RIP    : ");
    serial_puts(buf);
    fb_print("\n", 0);
    serial_puts("\n");

    u64_to_hex(cs, buf);
    fb_print_value("CS     : ", buf, 0xAAAAAA, 0xFFFFFF);
    serial_puts("CS     : ");
    serial_puts(buf);
    fb_print("  (ring ", 0xAAAAAA);
    serial_puts("  (ring ");
    u64_to_dec(cs & 3, buf);
    fb_print(buf, 0xAAAAAA);
    serial_puts(buf);
    fb_print(")\n", 0xAAAAAA);
    serial_puts(")\n");

    u64_to_hex(rflags, buf);
    fb_print_value("RFLAGS : ", buf, 0xAAAAAA, 0xFFFFFF);
    serial_puts("RFLAGS : ");
    serial_puts(buf);
    fb_print("\n", 0);
    serial_puts("\n");

    u64_to_hex(rsp, buf);
    fb_print_value("RSP    : ", buf, 0xAAAAAA, 0xFFFFFF);
    serial_puts("RSP    : ");
    serial_puts(buf);
    fb_print("\n", 0);
    serial_puts("\n");

    u64_to_hex(ss, buf);
    fb_print_value("SS     : ", buf, 0xAAAAAA, 0xFFFFFF);
    serial_puts("SS     : ");
    serial_puts(buf);
    fb_print("\n", 0);
    serial_puts("\n");

    if (vector == 14) {
        uint64_t cr2;
        asm volatile("mov %%cr2, %0" : "=r"(cr2));
        u64_to_hex(cr2, buf);
        fb_print_value("CR2    : ", buf, 0xAAAAAA, 0xFF7777);
        serial_puts("CR2    : ");
        serial_puts(buf);
        fb_print("  -> ", 0xAAAAAA);
        serial_puts("  -> ");

        if (error_code & 1) {
            fb_print("page-protection violation", 0xFF7777);
            serial_puts("page-protection violation");
        } else {
            fb_print("page not present", 0xFF7777);
            serial_puts("page not present");
        }
        if (error_code & 2) {
            fb_print(", write attempt", 0xFF7777);
            serial_puts(", write attempt");
        }
        if (error_code & 4) {
            fb_print(", user mode", 0xFF7777);
            serial_puts(", user mode");
        }
        if (error_code & 8) {
            fb_print(", reserved bit set", 0xFF7777);
            serial_puts(", reserved bit set");
        }
        if (error_code & 16) {
            fb_print(", instruction fetch", 0xFF7777);
            serial_puts(", instruction fetch");
        }

        fb_print("\n", 0);
        serial_puts("\n");
    }

    fb_print("\nSystem halted.\n", 0xFF5555);
    serial_puts("System halted.\n\n");

    while (1) asm volatile("hlt");
}

static void idt_set_gate(uint8_t n, void *handler, uint8_t ist) {
    uint64_t addr = (uint64_t)handler;
    idt[n].offset_low  = addr & 0xFFFF;
    idt[n].selector    = 0x08;
    idt[n].ist         = ist;
    idt[n].type_attr   = IDT_INTERRUPT;
    idt[n].offset_mid  = (addr >> 16) & 0xFFFF;
    idt[n].offset_high = addr >> 32;
    idt[n].zero        = 0;
}

void idt_init(void) {
    for (int i = 0; i < IDT_ENTRIES; i++) {
        idt_set_gate(i, (void*)((uint64_t)&isr0 + i * 16), 0);
    }

    idt_set_gate(0, isr0, 0);     // divide error
    idt_set_gate(1, isr1, 0);     // debug exception
    idt_set_gate(2, isr2, 2);     // non-maskable interrupt
    idt_set_gate(3, isr3, 0);     // breakpoint
    idt_set_gate(4, isr4, 0);     // overflow
    idt_set_gate(5, isr5, 0);     // bound range exceeded
    idt_set_gate(6, isr6, 0);     // invalid opcode
    idt_set_gate(7, isr7, 0);     // device not available
    idt_set_gate(8, isr8, 1);     // double fault
    idt_set_gate(9, isr9, 0);     // coprocessor segment overrun
    idt_set_gate(10, isr10, 0);   // invalid TSS
    idt_set_gate(11, isr11, 0);   // segment not present
    idt_set_gate(12, isr12, 0);   // stack-segment fault
    idt_set_gate(13, isr13, 0);   // general protection fault
    idt_set_gate(14, isr14, 0);   // page fault
    idt_set_gate(15, isr15, 0);   // reserved
    idt_set_gate(16, isr16, 0);   // x87 floating-point exception
    idt_set_gate(17, isr17, 0);   // alignment check
    idt_set_gate(18, isr18, 3);   // machine check
    idt_set_gate(19, isr19, 0);   // SIMD floating-point exception
    idt_set_gate(20, isr20, 0);   // virtualization exception
    idt_set_gate(21, isr21, 0);   // control protection exception
    idt_set_gate(22, isr22, 0);   // reserved
    idt_set_gate(23, isr23, 0);   // reserved
    idt_set_gate(24, isr24, 0);   // reserved
    idt_set_gate(25, isr25, 0);   // reserved
    idt_set_gate(26, isr26, 0);   // reserved
    idt_set_gate(27, isr27, 0);   // reserved
    idt_set_gate(28, isr28, 0);   // hypervisor injection exception
    idt_set_gate(29, isr29, 0);   // VMM communication exception
    idt_set_gate(30, isr30, 0);   // security exception
    idt_set_gate(31, isr31, 0);   // reserved

    idtr.limit = sizeof(idt) - 1;
    idtr.base  = (uint64_t)&idt;

    asm volatile ("lidt %0" : : "m"(idtr));
}