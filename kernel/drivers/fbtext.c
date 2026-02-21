#include <drivers/fbtext.h>
#include <drivers/font.h>
#include <klib/string.h>

#define LEFT_MARGIN 20

static struct limine_framebuffer *g_fb = NULL;
static font_t *g_font = NULL;
static size_t g_cursor_x = LEFT_MARGIN;
static size_t g_cursor_y = 15;
static bool overwrite = false;
static size_t overwrite_pos = 0;

void fbtext_init(struct limine_framebuffer *fb, font_t *font)
{
    g_fb   = fb;
    g_font = font;

    if (!font || !font->glyphs || font->width == 0 || font->height == 0) {
        return;
    }

    g_cursor_x = LEFT_MARGIN;
    g_cursor_y = 15;
}

static inline void fb_newline(void)
{
    g_cursor_x = LEFT_MARGIN;
    g_cursor_y += g_font->line_height;
}

static inline void fb_advance(size_t pixels)
{
    g_cursor_x += pixels;
    if (g_cursor_x + g_font->width > g_fb->width) {
        fb_newline();
    }
}

static void fb_clear_area(size_t x, size_t y, size_t width, size_t height, uint32_t color)
{
    if (!g_fb || x >= g_fb->width || y >= g_fb->height) return;

    uint32_t *pixels = (uint32_t *)g_fb->address;
    size_t stride = g_fb->pitch / sizeof(uint32_t);

    size_t max_x = g_fb->width;
    size_t max_y = g_fb->height;

    if (x + width > max_x)  width  = max_x - x;
    if (y + height > max_y) height = max_y - y;
    if (width == 0 || height == 0) return;

    for (size_t dy = 0; dy < height; dy++) {
        size_t py = y + dy;
        for (size_t dx = 0; dx < width; dx++) {
            size_t px = x + dx;
            pixels[py * stride + px] = color;
        }
    }
}

void fb_put_char(uint32_t codepoint, uint32_t color)
{
    if (!g_fb || !g_font || !g_font->glyphs) {
        return;
    }

    if (codepoint > 255) {
        codepoint = '?';
    }

    if (codepoint == '\n') {
        fb_newline();
        overwrite = false;
        return;
    }
    if (codepoint == '\r') {
        g_cursor_x = LEFT_MARGIN;
        overwrite = true;
        overwrite_pos = LEFT_MARGIN;
        return;
    }

    if (g_cursor_x + g_font->width > g_fb->width) {
        fb_newline();
        overwrite = false;
    }
    if (g_cursor_y + g_font->height > g_fb->height) {
        return;
    }

    if(overwrite) {
        fb_clear_area(
            g_cursor_x,
            g_cursor_y,
            g_font->width,
            g_font->height,
            0x00000000
        );

        size_t new_right = g_cursor_x + g_font->width;
        if (new_right > overwrite_pos) {
            overwrite = new_right;
        }
    }

    uint32_t *fb_pixels = (uint32_t *)g_fb->address;
    size_t fb_stride = g_fb->pitch / sizeof(uint32_t);

    size_t glyph_offset;
    if (g_font->is_psf2) {
        glyph_offset = (size_t)codepoint * g_font->hdr.psf2->charsize;
    } else {
        glyph_offset = (size_t)codepoint * g_font->height;
    }

    const uint8_t *glyph = g_font->glyphs + glyph_offset;
    uint32_t bytes_per_row = (g_font->width + 7) / 8;

    for (uint32_t row = 0; row < g_font->height; row++) {
        for (uint32_t col = 0; col < g_font->width; col++) {
            uint32_t byte_idx = row * bytes_per_row + (col / 8);
            uint8_t bit_mask = 0x80 >> (col % 8);

            if (glyph[byte_idx] & bit_mask) {
                size_t px = g_cursor_x + col;
                size_t py = g_cursor_y + row;
                if (px < g_fb->width && py < g_fb->height) {
                    fb_pixels[py * fb_stride + px] = color;
                }
            }
        }
    }

    fb_advance(g_font->width);
}

void fb_print(const char *str, uint32_t color)
{
    if (!str) return;

    while (*str) {
        fb_put_char((uint8_t)*str++, color);
    }
}

void fb_print_at(const char *str, uint32_t color, int x, int y)
{
    if (x < 0 || y < 0) return;

    size_t old_x = g_cursor_x;
    size_t old_y = g_cursor_y;

    g_cursor_x = (size_t)x;
    g_cursor_y = (size_t)y;

    fb_print(str, color);

    g_cursor_x = old_x;
    g_cursor_y = old_y;
}

void fb_print_number(uint64_t n, uint32_t color)
{
    char buf[32];
    char *p = buf + sizeof(buf);
    *--p = '\0';

    do {
        *--p = '0' + (n % 10);
        n /= 10;
    } while (n != 0);

    fb_print(p, color);
}