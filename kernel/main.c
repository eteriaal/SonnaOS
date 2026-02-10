#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <limine.h>

#include <klib/memory.h>
#include <klib/string.h>
#include <arch/x86_64/gdt.h>
#include <arch/x86_64/idt.h>
#include <mm/pmm.h>
#include <drivers/font.h>
#include <drivers/fbtext.h>
#include <drivers/serial.h>
#include <colors.h>

#define ESTELLA_VERSION "v0.Estella.3.2"

__attribute__((used, section(".limine_requests")))
static volatile uint64_t limine_base_revision[] = LIMINE_BASE_REVISION(4);

__attribute__((used, section(".limine_requests")))
static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST_ID,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_bootloader_info_request bootloader_info_request = {
    .id = LIMINE_BOOTLOADER_INFO_REQUEST_ID,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_firmware_type_request firmware_type_request = {
    .id = LIMINE_FIRMWARE_TYPE_REQUEST_ID,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST_ID,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_module_request module_request = {
    .id = LIMINE_MODULE_REQUEST_ID,
    .revision = 0,
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST_ID,
    .revision = 0
};

__attribute__((used, section(".limine_requests_start")))
static volatile uint64_t limine_requests_start_marker[] = LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests_end")))
static volatile uint64_t limine_requests_end_marker[] = LIMINE_REQUESTS_END_MARKER;

void hcf(void) {
    for (;;) {
        asm ("hlt");
    }
}

static void print_system_info(struct limine_framebuffer *fb) {
    fb_print_value("SonnaOS", "https://github.com/eteriaal/SonnaOS \n", COL_TITLE, 0x20B2AA);
    fb_print(ESTELLA_VERSION " x86_64 EFI | limine protocol\n", COL_VERSION);
    fb_print("Using Spleen font 12x24 .psfu (psf2) \n", COL_INFO);

    fb_print("\n", 0);

    if (bootloader_info_request.response) {
        char bootloader_str[64];
        {
            size_t pos = 0;

            const char *name = bootloader_info_request.response->name;
            const char *version = bootloader_info_request.response->version;

            for (size_t i = 0; name[i] && pos + 1 < sizeof(bootloader_str); i++)
                bootloader_str[pos++] = name[i];

            if (pos + 1 < sizeof(bootloader_str))
                bootloader_str[pos++] = ' ';

            for (size_t i = 0; version[i] && pos + 1 < sizeof(bootloader_str); i++)
                bootloader_str[pos++] = version[i];

            bootloader_str[pos < sizeof(bootloader_str) ? pos : sizeof(bootloader_str) - 1] = '\0'; 
        }
        fb_print_value("Bootloader: ", bootloader_str, COL_LABEL, COL_VALUE);
        fb_print("\n", 0);
    }

    if (firmware_type_request.response) {
        const char *fw = "Unknown";
        switch (firmware_type_request.response->firmware_type) {
            case LIMINE_FIRMWARE_TYPE_X86BIOS: fw = "BIOS"; break;
            case LIMINE_FIRMWARE_TYPE_EFI32: fw = "EFI32"; break;
            case LIMINE_FIRMWARE_TYPE_EFI64: fw = "EFI64"; break;
            case LIMINE_FIRMWARE_TYPE_SBI: fw = "SBI"; break;
        }
        fb_print_value("Firmware:   ", fw, COL_LABEL, COL_VALUE);
        fb_print("\n", 0);
    }

    char fb_addr[20] = {0};
    u64_to_hex((uint64_t)fb->address, fb_addr);
    fb_print_value("Framebuffer:", fb_addr, COL_LABEL, COL_ADDRESS);
    fb_print("\n\n", 0);
}

static void print_memory_info(void) {
    if (!memmap_request.response) return;

    fb_print("Memory map\n", COL_TITLE);

    char buf[32] = {0};
    u64_to_dec(memmap_request.response->entry_count, buf);
    fb_print_value("Memmap entries:", buf, COL_LABEL, COL_VALUE);
    fb_print("\n", 0);

    u64_to_dec(pmm_get_total_frames(),  buf); fb_print_value("Total pages:   ", buf, COL_LABEL, COL_VALUE); fb_print("\n", 0);
    u64_to_dec(pmm_get_usable_frames(), buf); fb_print_value("Usable pages:  ", buf, COL_LABEL, COL_VALUE); fb_print("\n", 0);
    u64_to_dec(pmm_get_free_frames(),   buf); fb_print_value("Free pages:    ", buf, COL_LABEL, COL_VALUE); fb_print("\n", 0);
    u64_to_dec(pmm_get_used_frames(),   buf); fb_print_value("Used pages:    ", buf, COL_LABEL, COL_VALUE); fb_print("\n\n", 0);
}

static void run_pmm_tests(void) {
    fb_print("PMM Test: alloc / free / reuse\n", COL_TEST_HDR);

    size_t free_before = pmm_get_free_frames();
    char buf[32] = {0};
    u64_to_dec(free_before, buf);
    fb_print_value("Free before:", buf, COL_LABEL, COL_VALUE);
    fb_print("\n", 0);

    void *p1 = pmm_alloc();
    void *p2 = pmm_alloc();
    void *p3 = pmm_alloc();
    void *block = pmm_alloc_frames(8);

    if (!p1 || !p2 || !p3 || !block) {
        fb_print("Allocation failed.\n", COL_FAIL);
        return;
    }

    char addr[20];
    u64_to_hex((uint64_t)p1, addr); fb_print_value("p1:", addr, COL_LABEL, COL_ADDRESS); fb_print("\n", 0);
    u64_to_hex((uint64_t)p2, addr); fb_print_value("p2:", addr, COL_LABEL, COL_ADDRESS); fb_print("\n", 0);
    u64_to_hex((uint64_t)p3, addr); fb_print_value("p3:", addr, COL_LABEL, COL_ADDRESS); fb_print("\n", 0);
    u64_to_hex((uint64_t)block, addr); fb_print_value("block:", addr, COL_LABEL, COL_ADDRESS); fb_print("\n", 0);

    size_t free_mid = pmm_get_free_frames();
    u64_to_dec(free_mid, buf);
    fb_print_value("Free after alloc:", buf, COL_LABEL, COL_VALUE);
    fb_print("\n", 0);

    pmm_free(p1);
    pmm_free(p2);
    pmm_free(p3);
    pmm_free_frames(block, 8);

    size_t free_after = pmm_get_free_frames();
    u64_to_dec(free_after, buf);
    fb_print_value("Free after free: ", buf, COL_LABEL, COL_VALUE);
    fb_print("\n", 0);

    if (free_after == free_before) {
        fb_print("PASSED - memory fully recovered\n", COL_OK);

        void *reuse = pmm_alloc();
        if (reuse) {
            u64_to_hex((uint64_t)reuse, addr);
            fb_print_value("Reuse page:", addr, COL_OK, COL_OK);
            fb_print("\n", 0);
            pmm_free(reuse);
        } else {
            fb_print("Reuse failed (unexpected)\n", COL_WARNING);
        }
    } else {
        fb_print("FAILED - leak detected!\n", COL_FAIL);

        int64_t delta = (int64_t)free_before - (int64_t)free_after;
        char delta_str[32] = {0};
        if (delta > 0) {
            delta_str[0] = '+';
            u64_to_dec((uint64_t)delta, delta_str + 1);
        } else {
            delta_str[0] = '-';
            u64_to_dec((uint64_t)(-delta), delta_str + 1);
        }
        fb_print_value("Delta:", delta_str, COL_FAIL, COL_FAIL);
        fb_print(" pages\n", COL_FAIL);
    }

    fb_print("\n", 0);
}

static void run_exception_test(void) {
    fb_print("Testing #UD (invalid opcode (ud2))...\n", COL_TEST_HDR);
    asm volatile("ud2");

    // fb_print("Testing #DE (divide by zero)...\n", COL_TEST_HDR);
    // asm volatile("mov $0, %%eax; idiv %%eax" : : : "eax");

    // fb_print("Testing #GP (invalid selector)...\n", 0xFFFF00);
    // asm volatile("mov $0x28, %%ax; mov %%ax, %%fs; mov %%fs:0, %%rax" : : : "rax", "ax");

    // fb_print("Testing #PF (dereference null)...\n", 0xFFFF00);
    // volatile uint64_t *null_ptr = (uint64_t *)0x0;
    // uint64_t dummy = *null_ptr;

    // fb_print("Testing #BP (int3)...\n", 0xFFFF00);
    // asm volatile("int3");

    // fb_print("Testing #TS (invalid TSS)...\n", 0xFFFF00);
    // asm volatile(
    //     "mov $0x28, %%ax\n\t"
    //     "ltr %%ax\n\t"
    //     "mov $0xFFFF, %%ax\n\t"
    //     "ltr %%ax"
    //     : : : "ax"
    // );
}

void kmain(void) {
    serial_init();
    serial_puts("kernel started\n");
    if (!LIMINE_BASE_REVISION_SUPPORTED(limine_base_revision)) hcf();
    if (!framebuffer_request.response || framebuffer_request.response->framebuffer_count == 0) hcf();
    if (!hhdm_request.response || !memmap_request.response || !module_request.response) hcf();

    uint64_t hhdm_offset = hhdm_request.response->offset;
    struct limine_framebuffer *fb = framebuffer_request.response->framebuffers[0];

    struct limine_file *font_module;
    struct psf2_header *psf2 = load_psf2_font(module_request, &font_module);
    if (!psf2) {serial_puts("!psf2\n");hcf();}

    font_t font = {
        .is_psf2 = true,
        .hdr.psf2 = psf2,
        .glyphs = (const uint8_t *)font_module->address + psf2->headersize,
        .width = psf2->width,
        .height = psf2->height,
        .line_height = psf2->height + 1,
        .glyph_count = psf2->length
    };
    fbtext_init(fb, &font);

    gdt_init(); fb_print("GDT + TSS initialized\n", COL_SUCCESS_INIT); serial_puts("GDT + TSS initialized\n");
    idt_init(); fb_print("IDT + ISR initialized\n", COL_SUCCESS_INIT); serial_puts("IDT + ISR initialized\n");
    pmm_init(memmap_request.response, hhdm_offset);
    fb_print("PMM bitmap initialized\n", COL_SUCCESS_INIT); serial_puts("PMM bitmap initialized\n");

    fb_print("\n", 0);

    print_system_info(fb);
    print_memory_info();

    run_pmm_tests();

    serial_puts("Run exception test\n");
    run_exception_test();

    hcf();
}