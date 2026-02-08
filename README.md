# SonnaOS
Writing an operating system because I want to occupy my time...  

## Kernel
**Estella** - x86_64 EFI kernel using the Limine bootloader.  

### Current status
- Boots via Limine kernel protocol
- GDT: kernel and user segments
- Simple bitmap-based physical memory manager
- Framebuffer console with Spleen 8x16 font (PSF1)
- Debug output: boot info, memory map, PMM alloc/free tests

Next: IDT, PSF2, TSS

![screenshot](sonnaos.png)

### Requirements
- clang + ld.lld
- QEMU + OVMF
- make, git, curl/wget

### build & run
```bash
make run
```