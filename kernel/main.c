#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <limine.h>

#include <klib/memory.h>
#include <klib/string.h>
#include <arch/x86_64/gdt.h>
#include <arch/x86_64/idt.h>
#include <arch/x86_64/acpi.h>
#include <arch/x86_64/apic.h>
#include <drivers/font.h>
#include <drivers/fbtext.h>
#include <drivers/serial.h>
#include <drivers/keyboard.h>
#include <mm/pmm.h>
#include <mm/vmm.h>
#include <colors.h>
#include <stopwatch.h>

#define ESTELLA_VERSION "v0.Estella.7.0"

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
volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST_ID,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_module_request module_request = {
    .id = LIMINE_MODULE_REQUEST_ID,
    .revision = 0,
};

__attribute__((used, section(".limine_requests")))
volatile struct limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST_ID,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
volatile struct limine_rsdp_request rsdp_request = {
    .id = LIMINE_RSDP_REQUEST_ID,
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
    fb_print("SonnaOS ", COL_TITLE);
    fb_print("https://github.com/aprentxdev/SonnaOS", 0x20B2AA);

    if (firmware_type_request.response) {
        const char *fw = "Unknown";
        switch (firmware_type_request.response->firmware_type) {
            case LIMINE_FIRMWARE_TYPE_X86BIOS: fw = "BIOS"; break;
            case LIMINE_FIRMWARE_TYPE_EFI32: fw = "EFI32"; break;
            case LIMINE_FIRMWARE_TYPE_EFI64: fw = "EFI64"; break;
            case LIMINE_FIRMWARE_TYPE_SBI: fw = "SBI"; break;
        }
        fb_print("   Firmware:   ", COL_LABEL);
        fb_print(fw, COL_VALUE);
        fb_print("\n", 0);

        serial_puts("Firmware: ");
        serial_puts(fw);
        serial_puts("\n");
    }

    fb_print(ESTELLA_VERSION " x86_64 EFI | limine protocol", COL_VERSION);
    
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

        fb_print("     Bootloader: ", COL_LABEL);
        fb_print(bootloader_str, COL_VALUE);
        fb_print("\n", 0);

        serial_puts("Bootloader: ");
        serial_puts(bootloader_str);
        serial_puts("\n");
    }
}

static void print_memory_info(void) {
    size_t total = pmm_get_total_frames();
    size_t usable = pmm_get_usable_frames();
    size_t free = pmm_get_free_frames();
    size_t used = pmm_get_used_frames();

    fb_print("Memory Overview\n", COL_SECTION_TITLE);

    fb_print("> Total   ", COL_LABEL);
    fb_print_number(total * PAGE_SIZE / 1024 / 1024, COL_VALUE);
    fb_print(" MiB (", COL_VALUE);
    fb_print_number(total, COL_VALUE);
    fb_print(" pages)\n", COL_VALUE);

    fb_print("> Usable  ", COL_LABEL);
    fb_print_number(usable * PAGE_SIZE / 1024 / 1024, COL_VALUE);
    fb_print(" MiB (", COL_VALUE);
    fb_print_number(usable, COL_VALUE);
    fb_print(" pages)\n", COL_VALUE);

    fb_print("> Used    ", COL_LABEL);
    fb_print_number(used * PAGE_SIZE / 1024 / 1024, COL_USED);
    fb_print(" MiB (", COL_USED);
    fb_print_number(used, COL_USED);
    fb_print(" pages)\n", COL_USED);

    fb_print("> Free    ", COL_LABEL);
    fb_print_number(free * PAGE_SIZE / 1024 / 1024, COL_FREE);
    fb_print(" MiB (", COL_FREE);
    fb_print_number(free, COL_FREE);
    fb_print(" pages)\n\n", COL_FREE);

    char buf[80];
    serial_puts("Memory: total ");
    u64_to_dec(total * PAGE_SIZE / 1024 / 1024, buf);
    serial_puts(buf);
    serial_puts(" MiB (");
    u64_to_dec(total, buf);
    serial_puts(buf);
    serial_puts(" pages), usable ");
    u64_to_dec(usable * PAGE_SIZE / 1024 / 1024, buf);
    serial_puts(buf);
    serial_puts(" MiB (");
    u64_to_dec(usable, buf);
    serial_puts(buf);
    serial_puts(" pages), free ");
    u64_to_dec(free * PAGE_SIZE / 1024 / 1024, buf);
    serial_puts(buf);
    serial_puts(" MiB (");
    u64_to_dec(free, buf);
    serial_puts(buf);
    serial_puts(" pages)\n");
}

void run_pmm_tests(void) {
    void *p1 = pmm_alloc();
    if (!p1) goto fail;
    pmm_free(p1);

    void *p2 = pmm_alloc_frames(4);
    if (!p2) goto fail;
    pmm_free_frames(p2, 4);

    void *p3 = pmm_alloc_frames_aligned(4, PAGE_SIZE * 4);
    if (!p3 || ((uintptr_t)p3 % (PAGE_SIZE * 4) != 0)) goto fail;
    pmm_free_frames(p3, 4);

    void *p4 = pmm_alloc_zeroed();
    if (!p4) goto fail;
    if (*(uint64_t*)phys_to_virt((uint64_t)p4) != 0) goto fail;
    pmm_free(p4);

    fb_print("PMM tests: OK\n", COL_SUCCESS_INIT);
    serial_puts("PMM tests OK\n");
    return;

fail:
    fb_print("PMM tests: FAILED\n", COL_FAIL);
    serial_puts("PMM tests FAILED\n");
}

void run_vmm_tests(void) {
    uint64_t vaddr = 0xFFFF900000000000ULL;
    void *phys = pmm_alloc();
    if (!phys) {
        fb_print("VMM tests: FAILED\n", COL_FAIL);
        serial_puts("VMM tests FAILED\n");
    }

    if (!vmm_map(vaddr, (uint64_t)phys, PTE_KERNEL_RW)) pmm_free(phys);

    uint64_t *ptr = (uint64_t *)vaddr;
    *ptr = 0xDEADBEEFCAFEBABELL;
    if (*ptr != 0xDEADBEEFCAFEBABELL) vmm_unmap(vaddr);

    vmm_unmap(vaddr);
    pmm_free(phys);

    fb_print("VMM tests: OK\n", COL_SUCCESS_INIT);
    serial_puts("VMM tests OK\n");
}

void EstellaEntry(void) {
    // asm volatile("sti");
    // https://codeberg.org/Limine/limine-protocol/src/branch/trunk/PROTOCOL.md#x86-64-1
    // IF flag is cleared on entry

    serial_init();
    serial_puts("EstellaEntry\n");

    if (!LIMINE_BASE_REVISION_SUPPORTED(limine_base_revision)) hcf();
    if (!framebuffer_request.response || framebuffer_request.response->framebuffer_count == 0) hcf();
    if (!hhdm_request.response || !memmap_request.response || !module_request.response || !rsdp_request.response) hcf();

    // load font, init fbtext
    struct limine_file *font_module;
    struct psf2_header *psf2 = load_psf2_font(module_request, &font_module);
    if (!psf2) {
        serial_puts("failed to load font?\n");
        hcf();
    }
    font_t font = {
        .is_psf2 = true,
        .hdr.psf2 = psf2,
        .glyphs = (const uint8_t *)font_module->address + psf2->headersize,
        .width = psf2->width,
        .height = psf2->height,
        .line_height = psf2->height + 1,
        .glyph_count = psf2->length
    };
    struct limine_framebuffer *fb = framebuffer_request.response->framebuffers[0];
    fbtext_init(fb, &font);

    // init everything
    gdt_init(); fb_print("GDT with TSS initialized;", COL_SUCCESS_INIT);
    idt_init(); fb_print(" IDT initialized;", COL_SUCCESS_INIT);
    pmm_init(); fb_print("  PMM initialized;", COL_SUCCESS_INIT); 
    vmm_init(); fb_print("  VMM initialized;", COL_SUCCESS_INIT); 
    apic_init(); fb_print("  APIC initialized;", COL_SUCCESS_INIT);
    keyboard_init(); fb_print(" PS/2 keyboard driver initialized\n", COL_SUCCESS_INIT);
    stopwatch_init();

    run_pmm_tests(); run_vmm_tests();
    fb_print("\n", 0); print_system_info(fb);
    fb_print("\n", 0); print_memory_info();

    // Enabling interrupts
    asm volatile("sti");

    fb_print("Controls:\n", COL_INFO);
    fb_print("t : Start / Pause stopwatch\n", COL_INFO);
    fb_print("q : Trigger kernel panic (from #UD)\n", COL_INFO);
    fb_print("\n", 0);

    serial_puts("Controls: t = toggle stopwatch, q = trigger panic\n");

    while (1)
    {
        if (keyboard_has_data())
        {
            char ch = keyboard_get_char();

            if (ch != 0)
            {
                switch (ch)
                {
                    case 't':
                    case 'T':
                        stopwatch_toggle();
                        break;

                    case 'q':
                    case 'Q':
                        fb_print("\nTriggering test panic...\n", COL_FAIL);
                        serial_puts("\nTriggering test panic...\n");
                        asm ("ud2");
                        break;
                    default:
                        break;
                }
            }
        }

        uint64_t now = timer_get_tsc();
        stopwatch_update(now, tsc_frequency_hz);

        asm volatile("pause");
    }

    hcf();
}