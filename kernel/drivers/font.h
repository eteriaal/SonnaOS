#ifndef KERNEL_FONT_H
#define KERNEL_FONT_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <limine.h>

typedef struct {
    bool is_psf2;
    union {
        struct psf1_header *psf1;
        struct psf2_header *psf2;
    } hdr;
    const uint8_t *glyphs;
    uint32_t width;
    uint32_t height;
    uint32_t line_height;
    uint32_t glyph_count;
} font_t;


#define PSF1_MAGIC0 0x36
#define PSF1_MAGIC1 0x04

struct psf1_header {
    uint8_t magic[2];
    uint8_t mode;
    uint8_t charsize;
};


#define PSF2_MAGIC 0x864AB572

struct psf2_header {
    uint32_t magic;
    uint32_t version;
    uint32_t headersize;
    uint32_t flags;
    uint32_t length;
    uint32_t charsize;
    uint32_t height;
    uint32_t width;
};

struct psf1_header* load_psf1_font(struct limine_module_request module_request, struct limine_file **out_glyphs);
struct psf2_header* load_psf2_font(struct limine_module_request module_request, struct limine_file **out_glyphs);

#endif