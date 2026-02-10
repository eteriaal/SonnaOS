#include <drivers/font.h>

struct psf1_header* load_psf1_font(struct limine_module_request module_request, struct limine_file **out_glyphs) {
    if (!module_request.response) return NULL;

    for(size_t i = 0; i < module_request.response->module_count; i++) {
        struct limine_file *module = module_request.response->modules[i];
        struct psf1_header *hdr = (struct psf1_header*)module->address;

        if (hdr->magic[0] == PSF1_MAGIC0 && hdr->magic[1] == PSF1_MAGIC1) {
            *out_glyphs = module;
            return hdr;
        }
    }

    return NULL;
}

struct psf2_header* load_psf2_font(struct limine_module_request module_request, struct limine_file **out_glyphs) {
    if (!module_request.response) return NULL;

    for (size_t i = 0; i < module_request.response->module_count; i++) {
        struct limine_file *module = module_request.response->modules[i];
        if (module->size < sizeof(struct psf2_header)) continue;

        struct psf2_header *hdr = (struct psf2_header*) module->address;

        if (hdr->magic == PSF2_MAGIC) {
            if (module->size >= hdr->headersize + hdr->length * hdr->charsize) {
                *out_glyphs = module;
                return hdr;
            }
        }
    }
    return NULL;
}