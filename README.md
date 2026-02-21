# *SonnaOS* ![x86-64](https://img.shields.io/badge/Arch-x86--64-blue) ![UEFI](https://img.shields.io/badge/FW-UEFI-orange?logo=uefi) [![Limine](https://img.shields.io/badge/Bootloader-Limine-9cf)](https://github.com/limine-bootloader/limine) [![License](https://img.shields.io/github/license/aprentxdev/SonnaOS)](./LICENSE)

Writing an operating system to explore low-level architecture and hardware programming.  
(because I want to occupy my time)

![screenshot](sonnaos.png)

### Hardware Requirements
- **Firmware**: UEFI
- **CPU**: 64-bit x86 processor **with x2APIC** support
- **ACPI 2.0+**

## Kernel
**Estella** - x86_64 EFI kernel using the Limine bootloader protocol.

## Current status
- ✅ UEFI boot via Limine bootloader on x86_64
- ✅ Long mode + higher-half kernel - by Limine
- ✅ GDT + TSS 
- ✅ IDT + basic exception handlers - ISRs
- ✅ ACPI parsing (RSDP, XSDT, MADT, HPET)
- ✅ x2APIC support
- ✅ LAPIC timer in TSC-deadline mode (when invariant TSC available)
- ✅ TSC frequency detection (CPUID 0x15/0x16 + HPET fallback calibration)
- ✅ Physical Memory Manager (PMM) with self-tests
- ✅ Virtual Memory Manager (VMM) with self-tests
- ✅ PS/2 keyboard driver
    - Debug hotkeys: `t` → toggle stopwatch, `q` → test panic
- ✅ Serial (COM1) debug output
- ✅ Framebuffer text console (PSF2 font: Spleen 12x24)

kernel shell

### Requirements
- clang + ld.lld
- QEMU + OVMF
- make, git, curl/wget

### Build & Run
```bash
git clone https://github.com/aprentxdev/SonnaOS
cd SonnaOS
make run
```