#ifndef KERNEL_FBTEXT_H
#define KERNEL_FBTEXT_H

#include <stdint.h>
#include <stddef.h>
#include "limine.h"
#include "font.h"

void fb_put_char(struct limine_framebuffer *fb, struct psf1_header *font, const uint8_t *glyphs_addr,
                 char c, size_t x, size_t y, uint32_t fg_color);

void fb_put_string(struct limine_framebuffer *fb, struct psf1_header *font, const uint8_t *glyphs_addr,
                   const char *str, size_t x, size_t y, uint32_t fg_color);

size_t fb_put_label_value(struct limine_framebuffer *fb, struct psf1_header *font, const uint8_t *glyphs_addr,
                                 const char *label, const char *value, size_t x, size_t y, uint32_t fg_color);

#endif