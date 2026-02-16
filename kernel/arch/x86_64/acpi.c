#include <stdint.h>
#include <stddef.h>

#include <arch/x86_64/acpi.h>
#include <mm/vmm.h>

extern uint64_t hhdm_offset;

struct xsdt* acpi_get_xsdt(void* rsdp_ptr) {
    struct rsdp2* rsdp = (struct rsdp2*)rsdp_ptr;

    if (rsdp->revision < 2) return NULL;

    return (struct xsdt*)((uintptr_t)rsdp->xsdt_address + hhdm_offset);
}

struct acpi_sdt_header* acpi_find_table(struct xsdt* xsdt, const char* signature) {
    size_t entries = (xsdt->header.length - sizeof(struct acpi_sdt_header)) / 8;
    for (size_t i = 0; i < entries; i++) {
        struct acpi_sdt_header* header = (struct acpi_sdt_header*)(uintptr_t)(xsdt->entries[i] + hhdm_offset);
        int match = 1;
        for (int j = 0; j < 4; j++) {
            if (header->signature[j] != signature[j]) {
                match = 0;
                break;
            }
        }
        if (match) return header;
    }
    return NULL;
}

struct madt* acpi_get_madt(void* rsdp_ptr) {
    struct xsdt* xsdt = acpi_get_xsdt(rsdp_ptr);
    if (!xsdt) return NULL;

    return (struct madt*)acpi_find_table(xsdt, "APIC");
}