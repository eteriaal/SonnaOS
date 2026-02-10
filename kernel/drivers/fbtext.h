#ifndef KERNEL_FBTEXT_H
#define KERNEL_FBTEXT_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include <limine.h>
#include <drivers/font.h>

void fbtext_init(struct limine_framebuffer *fb, font_t *font);
void fb_print(const char *str, uint32_t color);
void fb_print_value(const char *label, const char *value, uint32_t label_color, uint32_t value_color);
void fb_set_y(size_t y);

void fb_put_char(uint32_t codepoint, uint32_t color);

#endif