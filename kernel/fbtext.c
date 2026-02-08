#include "fbtext.h"
#include "font.h"
#include <libkernel/string.h>

#define PSF1_CHAR_WIDTH 8

void fb_put_char(struct limine_framebuffer *fb, struct psf1_header *font, const uint8_t *glyphs_addr,
                 char c, size_t x, size_t y, uint32_t fg_color) {
    if (x + PSF1_CHAR_WIDTH > fb->width ||
        y + font->charsize > fb->height)
        return;

    uint32_t *fbp = (uint32_t *)fb->address;
    size_t stride = fb->pitch / sizeof(uint32_t);

    // Glyphs start after the PSF1 header
    const uint8_t *glyphs = glyphs_addr + sizeof(struct psf1_header);
    uint8_t uc = (uint8_t)c;
    const uint8_t *glyph = glyphs + uc * font->charsize;

    for (uint32_t row = 0; row < font->charsize; row++)
    {
        uint8_t bits = glyph[row];

        for (uint32_t col = 0; col < 8; col++)
        {
            if (bits & (0x80 >> col))
            {
                fbp[(y + row) * stride + (x + col)] = fg_color;
            }
        }
    }
}

void fb_put_string(struct limine_framebuffer *fb, struct psf1_header *font, const uint8_t *glyphs_addr,
                   const char *str, size_t x, size_t y, uint32_t fg_color) {
    size_t currentx = x;
    size_t currenty = y;

    for (size_t i = 0; str[i]; i++)
    {
        if (str[i] == '\n')
        {
            currentx = x;
            currenty += font->charsize;
            continue;
        }

        fb_put_char(fb, font, glyphs_addr, str[i], currentx, currenty, fg_color);
        currentx += 8; // Fixed width of 8 pixels for PSF1 character
    }
}

size_t fb_put_label_value(struct limine_framebuffer *fb, struct psf1_header *font, const uint8_t *glyphs_addr,
                                 const char *label, const char *value, size_t x, size_t y, uint32_t fg_color) {
    fb_put_string(fb, font, glyphs_addr, label, x, y, fg_color);
    size_t offset = strlen(label) * 8;
    fb_put_string(fb, font, glyphs_addr, value, x + offset, y, fg_color);
    return x + offset + strlen(value) * 8;
}