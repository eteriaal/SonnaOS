#ifndef ESTELLA_DRIVERS_FBTEXT_H
#define ESTELLA_DRIVERS_FBTEXT_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include <limine.h>
#include <drivers/font.h>

void fbtext_init(struct limine_framebuffer *fb, font_t *font);
void fb_put_char(uint32_t codepoint, uint32_t color);
void fb_print(const char *str, uint32_t color);
void fb_print_at(const char *str, uint32_t color, int x, int y);
void fb_print_number(uint64_t, uint32_t color);

#endif