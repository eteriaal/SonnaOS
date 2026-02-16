#include <stdint.h>
#include <stdbool.h>

#include <limine.h>

#include <arch/x86_64/acpi.h>
#include <arch/x86_64/apic.h>
#include <mm/vmm.h>
#include <drivers/serial.h>
#include <klib/string.h>

volatile uint64_t lapic_ticks = 0;
uint64_t lapic_phys = 0;
uint64_t lapic_va = 0;

ioapic_t ioapics[8];
size_t ioapic_count = 0;

void lapic_timer_handler(void) {
    lapic_ticks++;

    volatile uint32_t* lapic = (volatile uint32_t*)lapic_va;
    lapic[LAPIC_EOI / 4] = 0;
}

void ioapic_init_all(void* madt_ptr);

void apic_init() {
    void *rsdp_ptr = rsdp_request.response->address;
    struct madt* madt = acpi_get_madt(rsdp_ptr);
    if (!madt) {
        return;
    }

    lapic_phys = madt->lapic_address;
    lapic_va = lapic_phys + hhdm_offset;

    vmm_map(lapic_va, lapic_phys, PTE_KERNEL_RW | PTE_PCD | PTE_PWT);
    volatile uint32_t* lapic = (volatile uint32_t*)lapic_va;
    lapic[LAPIC_ESR / 4] = 0;
    lapic[LAPIC_TPR / 4] = 0;
    lapic[LAPIC_SVR / 4] = LAPIC_SVR_ENABLE | LAPIC_SPURIOUS_VECTOR;
    lapic[LAPIC_LVT_TIMER / 4] = LAPIC_MASKED | LAPIC_TIMER_VECTOR;
    lapic[LAPIC_LVT_THERMAL / 4] = LAPIC_MASKED;
    lapic[LAPIC_LVT_PERF / 4] = LAPIC_MASKED;
    lapic[LAPIC_LVT_LINT0 / 4] = LAPIC_MASKED;
    lapic[LAPIC_LVT_LINT1 / 4] = LAPIC_MASKED;
    lapic[LAPIC_LVT_ERROR / 4] = LAPIC_MASKED | LAPIC_ERROR_VECTOR;
    lapic[LAPIC_TIMER_DCR / 4] = 0b0011;
    lapic[LAPIC_LVT_TIMER / 4] = LAPIC_TIMER_VECTOR | LAPIC_MODE_PERIODIC;
    lapic[LAPIC_TIMER_INIT / 4] = 1000000;
    lapic[LAPIC_LVT_ERROR / 4] &= ~LAPIC_MASKED;

    ioapic_init_all(madt);
}


static inline void ioapic_write_internal(volatile uint32_t* base, uint32_t reg, uint32_t value) {
    base[0] = reg;
    base[4] = value;
}

static inline uint32_t ioapic_read_internal(volatile uint32_t* base, uint32_t reg) {
    base[0] = reg;
    return base[4];
}

uint32_t ioapic_read(ioapic_t* io, uint32_t reg) {
    if (!io || !io->mmio_base_va) return 0xFFFFFFFF;
    volatile uint32_t* base = (volatile uint32_t*)io->mmio_base_va;
    return ioapic_read_internal(base, reg);
}

void ioapic_write(ioapic_t* io, uint32_t reg, uint32_t value) {
    if (!io || !io->mmio_base_va) return;
    volatile uint32_t* base = (volatile uint32_t*)io->mmio_base_va;
    ioapic_write_internal(base, reg, value);
}

void ioapic_init_one(struct madt_ioapic* entry) {
    if (ioapic_count >= 8) {
        serial_puts("Too many IOAPICs (>8)\n");
        return;
    }

    uint64_t phys = entry->ioapic_address;
    uint64_t virt = phys + hhdm_offset;

    vmm_map(virt, phys, PTE_PRESENT | PTE_WRITE | PTE_PCD | PTE_PWT);

    ioapic_t* io = &ioapics[ioapic_count++];

    io->ioapic_id = entry->ioapic_id;
    io->gsi_base = entry->global_system_interrupt_base;
    io->mmio_base_va = virt;

    uint32_t version = ioapic_read(io, IOAPIC_VERSION);
    io->max_redirection = (version >> 16) & 0xFF;

    for (int i = 0; i <= io->max_redirection; i++) {
        uint32_t reg_low = IOAPIC_REDIR_TBL_OFFSET + i*2;
        uint32_t reg_high = reg_low + 1;

        ioapic_write(io, reg_low, IOREDTBL_MASKED);
        ioapic_write(io, reg_high, 0);
    }
}

void ioapic_init_all(void* madt_ptr) {
    struct madt* madt = madt_ptr;
    uint8_t* ptr = (uint8_t*)madt + sizeof(struct madt);
    uint8_t* end = (uint8_t*)madt + madt->header.length;

    while (ptr < end) {
        struct madt_entry_header* hdr = (struct madt_entry_header*)ptr;

        switch (hdr->type) {
            case MADT_ENTRY_TYPE_IO_APIC: {
                struct madt_ioapic* io = (struct madt_ioapic*)hdr;
                ioapic_init_one(io);
                break;
            }

            case MADT_ENTRY_TYPE_INTERRUPT_OVERRIDE: {
                // Handle ISO later...
                break;
            }

            // Other types can be ignored for now
        }

        ptr += hdr->length;
    }
}


static ioapic_t* ioapic_for_gsi(uint32_t gsi, uint32_t* pin) {
    for (size_t i = 0; i < ioapic_count; i++) {
        ioapic_t* io = &ioapics[i];
        if (gsi >= io->gsi_base && gsi < io->gsi_base + io->max_redirection + 1) {
            *pin = gsi - io->gsi_base;
            return io;
        }
    }
    return NULL;
}

void ioapic_set_irq(uint32_t irq, uint8_t vector, bool level_triggered,
                    bool active_low, uint8_t delivery_mode, uint32_t destination) {
    uint32_t gsi = irq;
    bool level = level_triggered;
    bool low = active_low;

    uint32_t pin;
    ioapic_t* io = ioapic_for_gsi(gsi, &pin);
    if (!io) {
        return;
    }

    uint32_t low_reg = IOAPIC_REDIR_TBL_OFFSET + pin * 2;
    uint32_t high_reg = low_reg + 1;

    uint32_t low_val = vector | delivery_mode |
                       (level ? IOREDTBL_TRIGGER_LEVEL : IOREDTBL_TRIGGER_EDGE) |
                       (low   ? IOREDTBL_POLARITY_LOW  : IOREDTBL_POLARITY_HIGH);

    uint32_t high_val = destination << 24;

    ioapic_write(io, low_reg,  low_val  | IOREDTBL_MASKED);
    ioapic_write(io, high_reg, high_val);
    ioapic_write(io, low_reg,  low_val);
}

void ioapic_mask_irq(uint32_t irq) {
    uint32_t gsi = irq;
    uint32_t pin;
    ioapic_t* io = ioapic_for_gsi(gsi, &pin);
    if (!io) return;

    uint32_t reg = IOAPIC_REDIR_TBL_OFFSET + pin * 2;
    uint32_t val = ioapic_read(io, reg);
    ioapic_write(io, reg, val | IOREDTBL_MASKED);
}

void ioapic_unmask_irq(uint32_t irq) {
    uint32_t gsi = irq;
    uint32_t pin;
    ioapic_t* io = ioapic_for_gsi(gsi, &pin);
    if (!io) return;

    uint32_t reg = IOAPIC_REDIR_TBL_OFFSET + pin * 2;
    uint32_t val = ioapic_read(io, reg);
    ioapic_write(io, reg, val & ~IOREDTBL_MASKED);
}