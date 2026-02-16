# SonnaOS
Writing an operating system because I want to occupy my time...  

### Hardware Requirements
- **Firmware**: UEFI
- **CPU**: 64-bit x86 processor with x2APIC support
- **ACPI 2.0+**

## Kernel
**Estella** - x86_64 EFI kernel using the Limine bootloader protocol.

### Current status
- Boots on x86_64 UEFI via Limine
- GDT, TSS, IDT + ISRs ready
- ACPI: RSDP/XSDT/MADT parsed, LAPIC + IOAPIC initialized
- PMM + VMM up and passing self-tests
- PS/2 keyboard driver working
- Framebuffer console (Spleen 12×24 .psfu)

### Output
- Serial debug
- Framebuffer text mode

### Next:
- x2APIC TSC-deadline

---
![screenshot](sonnaos.png)
---

### Requirements
- clang + ld.lld
- QEMU + OVMF
- make, git, curl/wget

### build & run
```bash
make run
```