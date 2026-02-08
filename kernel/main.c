// main.c v0.Estella.1.0 
//
// Entry point for the Estella kernel using the Limine bootloader protocol.
// It initializes the framebuffer, loads a PSF1 font module, initializes pmm, display some text and tests

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <libkernel/memory.h>
#include <libkernel/string.h>
#include "limine.h"
#include "font.h"
#include "fbtext.h"
#include "bootdate.h"
#include "pmm.h"

#define COL_TITLE 0x88DDFF
#define COL_VERSION 0xAADD88
#define COL_LABEL 0xCCCCCC
#define COL_VALUE 0xD0FFD0
#define COL_ADDRESS 0xB0E0FF
#define COL_OK 0x44FF88
#define COL_FAIL 0xFF5555
#define COL_WARNING 0xFFCC44
#define COL_TEST_HDR 0xFFFF99
#define COL_SEPARATOR 0x444466
#define COL_DATE 0x99EEFF

// Base revision 4 (latest)
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
static volatile struct limine_mp_request mp_request = {
    .id = LIMINE_MP_REQUEST_ID,
    .revision = 0,
    .flags = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_smbios_request smbios_request = {
    .id = LIMINE_SMBIOS_REQUEST_ID,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_efi_system_table_request efi_system_table_request = {
    .id = LIMINE_EFI_SYSTEM_TABLE_REQUEST_ID,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_date_at_boot_request date_request = {
    .id = LIMINE_DATE_AT_BOOT_REQUEST_ID,
    .revision = 0
};

// Module request - request for font file (module_path: boot():/boot/spleen/spleen-8x16.psfu)
__attribute__((used, section(".limine_requests")))
volatile struct limine_module_request module_request = {
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

// Halt and catch fire function. Infinite loop.
void hcf(void) {
    for (;;) {
        asm ("hlt");
    }
}

void kmain(void) {
    if (LIMINE_BASE_REVISION_SUPPORTED(limine_base_revision) == false) {
        hcf();
    }

    if (framebuffer_request.response == NULL ||
        framebuffer_request.response->framebuffer_count < 1) {
        hcf();
    }

    if (hhdm_request.response == NULL) {
        hcf();
    }

    struct limine_hhdm_response *hhdm_resp = hhdm_request.response;
    uint64_t hhdm_offset = hhdm_resp->offset;

    if (memmap_request.response == NULL) {
        hcf();
    }

    // Ensure module (font) is loaded.
    if (!module_request.response || module_request.response->module_count < 1) {
        hcf();
    }

    struct limine_file *font_module;
    struct psf1_header *font = load_psf1_font(&font_module);
    if (!font) hcf();

    struct limine_framebuffer *fb = framebuffer_request.response->framebuffers[0];

    const uint8_t *glyphs_addr = (uint8_t*)font_module->address;
    size_t y = 10;

    pmm_init(memmap_request.response, hhdm_offset);

    fb_put_string(fb, font, glyphs_addr, "SonnaOS", 10, y, COL_TITLE);
    fb_put_string(fb, font, glyphs_addr, "https://github.com/eteriaal/SonnaOS", 10 + 7*8 + 8, y, 0x777799);
    y += 22;

    fb_put_string(fb, font, glyphs_addr, "v0.Estella.1.0 x86_64 EFI | limine protocol", 10, y, COL_VERSION);
    y += 22;

    fb_put_string(fb, font, glyphs_addr, "Using Spleen font 8x16 .psfu (psf1)", 10, y, 0xAAAAAA);
    y += 28;

    if (bootloader_info_request.response) {
        fb_put_string(fb, font, glyphs_addr, "Bootloader: ", 20, y, COL_LABEL);
        fb_put_string(fb, font, glyphs_addr, bootloader_info_request.response->name ? bootloader_info_request.response->name 
            : "unknown", 20 + 13 * 8, y, COL_VALUE);
        fb_put_string(fb, font, glyphs_addr, 
            bootloader_info_request.response->version ? bootloader_info_request.response->version : "—", 
            20 + 20 * 8, y, COL_VALUE);
        y += 18;
    }

    if (firmware_type_request.response) {
        const char *fw = "Unknown";
        switch (firmware_type_request.response->firmware_type) {
            case LIMINE_FIRMWARE_TYPE_X86BIOS: fw = "BIOS";   break;
            case LIMINE_FIRMWARE_TYPE_EFI32:  fw = "EFI32";  break;
            case LIMINE_FIRMWARE_TYPE_EFI64:  fw = "EFI64";  break;
            case LIMINE_FIRMWARE_TYPE_SBI:    fw = "SBI";    break;
        }
        fb_put_string(fb, font, glyphs_addr, "Firmware:   ",  20, y, COL_LABEL);
        fb_put_string(fb, font, glyphs_addr, fw, 20 + 13 * 8, y, COL_VALUE);
        y += 18;
    }

    if (date_request.response) {
        struct date d;
        timestamp_to_ymd(date_request.response->timestamp, &d);

        char year[5], month[3], day[3];
        u64_to_dec(d.year, year);
        u64_to_dec(d.month, month);
        u64_to_dec(d.day, day);

        fb_put_string(fb, font, glyphs_addr, "Boot date:  ", 20, y, COL_LABEL);
        int x = 20 + 13 * 8;
        fb_put_string(fb, font, glyphs_addr, year, x, y, COL_DATE); x += strlen(year) * 8;
        fb_put_string(fb, font, glyphs_addr, "/", x, y, 0x777777); x+=8;
        fb_put_string(fb, font, glyphs_addr, month, x, y, COL_DATE); x += strlen(month) * 8;
        fb_put_string(fb, font, glyphs_addr, "/", x, y, 0x777777); x += 8;
        fb_put_string(fb, font, glyphs_addr, day, x, y, COL_DATE);
        y += 18;
    }

    char fb_addr[19];
    u64_to_hex((uint64_t)fb->address, fb_addr);
    fb_put_string(fb, font, glyphs_addr, "Framebuffer: ", 20, y, COL_LABEL);
    fb_put_string(fb, font, glyphs_addr, fb_addr, 20 + 13 * 8, y, COL_ADDRESS);
    y += 22;

    if (memmap_request.response) {
        fb_put_string(fb, font, glyphs_addr, "Memory map", 10, y, COL_TITLE);
        y += 22;

        char count[21];
        u64_to_dec(memmap_request.response->entry_count, count);
        fb_put_label_value(fb, font, glyphs_addr, "Memmap entries: ", count, 20, y, COL_LABEL);
        y += 18;

        char total[21], free[21], usable[21], used[21];
        u64_to_dec(pmm_get_total_frames(), total);
        u64_to_dec(pmm_get_usable_frames(), usable);
        u64_to_dec(pmm_get_free_frames(), free);
        u64_to_dec(pmm_get_used_frames(), used);

        fb_put_label_value(fb, font, glyphs_addr, "Total pages:   ", total,  20, y, COL_LABEL); y += 18;
        fb_put_label_value(fb, font, glyphs_addr, "Usable pages:  ", usable, 20, y, COL_LABEL); y += 18;
        fb_put_label_value(fb, font, glyphs_addr, "Free pages:    ", free,   20, y, COL_LABEL); y += 18;
        fb_put_label_value(fb, font, glyphs_addr, "Used pages:    ", used,   20, y, COL_LABEL); y += 22;
    }


    // test pmm
    y += 12;
    fb_put_string(fb, font, glyphs_addr, "Test 1 - Basic allocation / free", 10, y, COL_TEST_HDR);
    y += 22;

    size_t free_before = pmm_get_free_frames();
    char buf[32];
    u64_to_dec(free_before, buf);
    fb_put_label_value(fb, font, glyphs_addr, "Free before: ", buf, 20, y, COL_LABEL);
    y += 18;

    void* p1 = pmm_alloc();
    void* p2 = pmm_alloc();
    void* p3 = pmm_alloc();

    char addr1[19], addr2[19], addr3[19];
    u64_to_hex((uint64_t)p1, addr1);
    u64_to_hex((uint64_t)p2, addr2);
    u64_to_hex((uint64_t)p3, addr3);

    fb_put_label_value(fb, font, glyphs_addr, "p1 addr: ", addr1, 20, y, COL_ADDRESS);
    y += 18;
    fb_put_label_value(fb, font, glyphs_addr, "p2 addr: ", addr2, 20, y, COL_ADDRESS);
    y += 18;
    fb_put_label_value(fb, font, glyphs_addr, "p3 addr: ", addr3, 20, y, COL_ADDRESS);
    y += 20;

    size_t free_after_single = pmm_get_free_frames();
    u64_to_dec(free_after_single, buf);
    fb_put_label_value(fb, font, glyphs_addr, "Free after 3 single: ", buf, 20, y, COL_LABEL);
    y += 20;

    void* block = pmm_alloc_frames(8);
    char block_addr[19];
    u64_to_hex((uint64_t)block, block_addr);
    fb_put_label_value(fb, font, glyphs_addr, "8-page block: ", block_addr, 20, y, COL_ADDRESS);
    y += 20;

    size_t free_after_block = pmm_get_free_frames();
    u64_to_dec(free_after_block, buf);
    fb_put_label_value(fb, font, glyphs_addr, "Free after 8-page block: ", buf, 20, y, COL_LABEL);
    y += 22;

    pmm_free_frames(block, 8);
    pmm_free(p3);
    pmm_free(p2);
    pmm_free(p1);

    size_t free_final = pmm_get_free_frames();
    u64_to_dec(free_final, buf);
    fb_put_label_value(fb, font, glyphs_addr, "Free after release: ", buf, 20, y, COL_LABEL);
    y += 22;

    if (free_final == free_before) {
        fb_put_string(fb, font, glyphs_addr, "Test 1 PASSED - memory recovered", 20, y, COL_OK);
        y += 20;

        void* reuse = pmm_alloc();
        if (reuse) {
            char reuse_addr[19];
            u64_to_hex((uint64_t)reuse, reuse_addr);
            fb_put_label_value(fb, font, glyphs_addr, "Reused page: ", reuse_addr, 20, y, COL_OK);
            y += 18;
            pmm_free(reuse);
        } else {
            fb_put_string(fb, font, glyphs_addr, "Reuse failed (unexpected)", 20, y, COL_WARNING);
            y += 18;
        }
    } else {
        fb_put_string(fb, font, glyphs_addr, "Test 1 FAILED - leak detected", 20, y, COL_FAIL);
        y += 20;

        int64_t delta = (int64_t)free_before - (int64_t)free_final;
        char delta_str[16];
        if (delta > 0) {
            delta_str[0] = '+';
            u64_to_dec((uint64_t)delta, delta_str + 1);
        } else {
            delta_str[0] = '-';
            u64_to_dec((uint64_t)(-delta), delta_str + 1);
        }
        fb_put_string(fb, font, glyphs_addr, "Delta: ", 20, y, COL_FAIL);
        fb_put_string(fb, font, glyphs_addr, delta_str, 100, y, COL_FAIL);
        fb_put_string(fb, font, glyphs_addr, " pages", 120, y, COL_FAIL);
        y += 18;
    }

    y += 20;


    fb_put_string(fb, font, glyphs_addr, "Test 2 - Fragmentation check", 10, y, COL_TEST_HDR);
    y += 22;

    size_t free_start = pmm_get_free_frames();
    char tmp[32];
    u64_to_dec(free_start, tmp);
    fb_put_label_value(fb, font, glyphs_addr, "Free before: ", tmp, 20, y, COL_LABEL);
    y += 18;

    void* a = pmm_alloc();
    void* b = pmm_alloc();
    char a1[19], b1[19];
    u64_to_hex((uint64_t)a, a1);
    u64_to_hex((uint64_t)b, b1);
    fb_put_label_value(fb, font, glyphs_addr, "a addr: ", a1, 20, y, COL_ADDRESS);
    y += 18;
    fb_put_label_value(fb, font, glyphs_addr, "b addr: ", b1, 20, y, COL_ADDRESS);
    y += 20;

    pmm_free(b);
    pmm_free(a);

    void* a2 = pmm_alloc();
    void* b2 = pmm_alloc();
    char a2s[19], b2s[19];
    u64_to_hex((uint64_t)a2, a2s);
    u64_to_hex((uint64_t)b2, b2s);
    fb_put_label_value(fb, font, glyphs_addr, "a2 addr: ", a2s, 20, y, COL_OK);
    y += 18;
    fb_put_label_value(fb, font, glyphs_addr, "b2 addr: ", b2s, 20, y, COL_OK);
    y += 20;

    pmm_free(b2);
    pmm_free(a2);

    #define BIG_COUNT 64
    void* big = pmm_alloc_frames(BIG_COUNT);
    if (big) {
        char big_addr[19];
        u64_to_hex((uint64_t)big, big_addr);
        fb_put_label_value(fb, font, glyphs_addr, "64-page block: ", big_addr, 20, y, COL_ADDRESS);
        y += 18;
        pmm_free_frames(big, BIG_COUNT);
    } else {
        fb_put_string(fb, font, glyphs_addr, "64-page allocation failed", 20, y, COL_WARNING);
        y += 18;
    }

    #define SMALL_COUNT 128
    void* smalls[SMALL_COUNT];

    int i = 0;
    for (i = 0; i < SMALL_COUNT; i++) {
        smalls[i] = pmm_alloc();
        if (!smalls[i]) {
            break;
        }
    }

    char cnt[32];
    u64_to_dec(i, cnt);
    fb_put_string(fb, font, glyphs_addr, "Allocated", 20, y, COL_LABEL);
    fb_put_string(fb, font, glyphs_addr, cnt, 100, y, COL_VALUE);
    fb_put_string(fb, font, glyphs_addr, " / ", 124, y, 0x777777);
    u64_to_dec(SMALL_COUNT, cnt);
    fb_put_string(fb, font, glyphs_addr, cnt, 148, y, COL_VALUE);
    fb_put_string(fb, font, glyphs_addr, "single pages", 180, y, COL_LABEL);
    y += 20;

    void* big_after = pmm_alloc_frames(32);
    if (big_after) {
        fb_put_string(fb, font, glyphs_addr, "32-page block OK after fragmentation", 20, y, COL_OK);
        pmm_free_frames(big_after, 32);
    } else {
        fb_put_string(fb, font, glyphs_addr, "32-page block FAILED after fragmentation", 20, y, COL_FAIL);
    }
    y += 20;

    for (int j = 0; j < i; j++) {
        if (smalls[j]) pmm_free(smalls[j]);
    }

    size_t free_end = pmm_get_free_frames();
    u64_to_dec(free_end, tmp);
    fb_put_label_value(fb, font, glyphs_addr, "Free after release: ", tmp, 20, y, COL_LABEL);
    y += 20;

    if (free_end == free_start) {
        fb_put_string(fb, font, glyphs_addr, "Test 2 PASSED - no leak", 20, y, COL_OK);
    } else {
        fb_put_string(fb, font, glyphs_addr, "Test 2 FAILED - leak detected", 20, y, COL_FAIL);
    }
    y += 24;

    fb_put_string(fb, font, glyphs_addr, "All PMM tests completed", 10, y, 0xAAAAAA);
    hcf();
}