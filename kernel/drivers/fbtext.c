#include <klib/string.h>
#include <drivers/fbtext.h>
#include <drivers/font.h>

#define LEFT_MARGIN 20

static struct limine_framebuffer *g_fb = NULL;
static font_t *g_font = NULL;
static size_t g_cursor_x = LEFT_MARGIN;
static size_t g_cursor_y = 15;

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

void fb_set_y(size_t y)
{
    g_cursor_y = y;
    g_cursor_x = LEFT_MARGIN;
}

static void fb_newline(void)
{
    g_cursor_x = LEFT_MARGIN;
    g_cursor_y += g_font->line_height;
}

static void fb_advance(size_t pixels)
{
    g_cursor_x += pixels;
    if (g_cursor_x + g_font->width > g_fb->width) {
        fb_newline();
    }
}

void fb_put_char(uint32_t codepoint, uint32_t color)
{
    if (!g_fb || !g_font || !g_font->glyphs) return;

    if (codepoint > 255) codepoint = '?';

    if (g_cursor_x + g_font->width > g_fb->width) {
        fb_newline();
    }
    if (g_cursor_y + g_font->height > g_fb->height) {
        return;
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

            uint32_t byte_index = row * bytes_per_row + (col / 8);
            uint8_t bit_mask = 0x80 >> (col % 8);

            if (glyph[byte_index] & bit_mask) {
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
        if (*str == '\n') {
            fb_newline();
            str++;
            continue;
        }
        fb_put_char((uint8_t)*str, color);
        str++;
    }
}

void fb_print_value(const char *label, const char *value, uint32_t label_color, uint32_t value_color)
{
    if (!label) return;

    fb_print(label, label_color);

    size_t label_len = strlen(label);
    size_t desired_x = LEFT_MARGIN + (label_len + 1) * g_font->width;

    if (g_cursor_x < desired_x)
        g_cursor_x = desired_x;

    if (value && *value)
        fb_print(value, value_color);
}