#include <stdint.h>
#include <memory/memory.hpp> // For memcmp
#include <arch/x86_64/Serial.hpp>
#include <arch/x86_64/Paging.hpp>
#include <arch/x86_64/Drivers/ACPI/ACPI_Header.hpp>
#include <arch/x86_64/Drivers/ACPI/MADT_Header.hpp>
#include <arch/x86_64/ScreenWriter.hpp>
#include <globals.hpp>

#define LAPIC_SVR 0x0F0
#define LAPIC_ENABLE_BIT 0x100

class ACPIDriver
{
private:
    ACPIHeader *root_table;
    FADT *fadt;
    bool is_xsdt;
    uint8_t lapic_ids[256];
    uint8_t cpu_count = 0;
    uint64_t lapic_phys_addr = 0;
    uint64_t ioapic_phys_addr = 0;

    bool VerifyChecksum(ACPIHeader *table);
    void *MapTable(uint64_t phys_addr, PageTableManager &manager);
    void ProcessMADT(PageTableManager &manager);

public:
    ACPIDriver(void *rsdp_phys, PageTableManager &manager);
    void *FindTable(const char *sig);
    void WriteACPI(GenericAddressStructure &reg, uint32_t value);
    void Shutdown();
    void Reboot();

    uint64_t GetLAPICAddress() { return lapic_phys_addr; }
    uint64_t GetIOAPICAddress() { return ioapic_phys_addr; }
    uint8_t GetCPUCount() { return cpu_count; }
    uint8_t *GetCPUList() { return lapic_ids; }

    void DisablePIC()
    {
        outb(0x21, 0xFF); // Mask all interrupts on Master PIC
        outb(0xA1, 0xFF); // Mask all interrupts on Slave PIC
    }

    void EnableLocalAPIC()
    {
        // Write to the Spurious Interrupt Vector Register
        // Use a high vector like 0xFF for spurious and set the enable bit (bit 8)
        uint32_t val = *(volatile uint32_t *)(lapic_phys_addr + LAPIC_SVR);
        *(volatile uint32_t *)(lapic_phys_addr + LAPIC_SVR) = val | LAPIC_ENABLE_BIT | 0xFF;
    }

    FADT *GetFADT() { return fadt; };
};