#include <arch/x86_64/Drivers/ACPI/ACPI.hpp>

ACPIDriver::ACPIDriver(void *rsdp_phys, PageTableManager &manager)
{
    // Cast to the base descriptor to check the revision
    RSDPDescriptor *rsdp = (RSDPDescriptor *)rsdp_phys;

    // Revision 2 or higher supports XSDT (64-bit)
    if (rsdp->Revision >= 2)
    {
        RSDPDescriptor20 *rsdp20 = (RSDPDescriptor20 *)rsdp_phys;
        if (rsdp20->XsdtAddress != 0)
        {
            root_table = (ACPIHeader *)MapTable(rsdp20->XsdtAddress, manager);
            is_xsdt = true;
        }
        else
        {
            root_table = (ACPIHeader *)MapTable(rsdp->RsdtAddress, manager);
            is_xsdt = false;
        }
    }
    else
    {
        root_table = (ACPIHeader *)MapTable(rsdp->RsdtAddress, manager);
        is_xsdt = false;
    }

    // Now find the FADT and DSDT safely
    fadt = (FADT *)FindTable("FACP");
    if (fadt)
    {
        MapTable((uint64_t)fadt, manager);
        uint64_t dsdt_phys = (fadt->X_Dsdt != 0) ? fadt->X_Dsdt : (uint64_t)fadt->Dsdt;
        MapTable(dsdt_phys, manager);
    }
    ProcessMADT(manager);
}

bool ACPIDriver::VerifyChecksum(ACPIHeader *table)
{
    uint8_t sum = 0;
    uint8_t *bytes = (uint8_t *)table;
    for (uint32_t i = 0; i < table->Length; i++)
    {
        sum += bytes[i];
    }
    return sum == 0;
}

void *ACPIDriver::MapTable(uint64_t phys_addr, PageTableManager &manager)
{
    // 1. Map the header page so we can read the 'Length' field
    manager.MapMemory((void *)(phys_addr & ~0xFFFULL), (void *)(phys_addr & ~0xFFFULL));
    ACPIHeader *header = (ACPIHeader *)phys_addr;

    // 2. Map the entire table based on its actual length
    uint32_t total_len = header->Length;
    for (uint64_t i = 0; i < total_len + 4096; i += 4096)
    {
        uint64_t p = (phys_addr & ~0xFFFULL) + i;
        manager.MapMemory((void *)p, (void *)p);
    }
    return (void *)phys_addr;
}

void ACPIDriver::ProcessMADT(PageTableManager &manager)
{
    MADT *madt = (MADT *)FindTable("APIC");
    if (!madt)
        return;

    // Standard MADT physical address for the Local APIC
    lapic_phys_addr = madt->LocalApicAddress;

    uint8_t *ptr = (uint8_t *)madt + sizeof(MADT);
    uint8_t *end = (uint8_t *)madt + madt->Header.Length;

    while (ptr < end)
    {
        MADT_EntryHeader *entry = (MADT_EntryHeader *)ptr;
        if (entry->Length == 0)
            break; // Safety break

        switch (entry->Type)
        {
        case 0:
        { // Local APIC (A CPU Core)
            MADT_LocalAPIC *lapic = (MADT_LocalAPIC *)entry;
            if (lapic->Flags & 0x01)
            { // Enabled
                lapic_ids[cpu_count++] = lapic->ApicID;
            }
            break;
        }
        case 1:
        { // I/O APIC
            MADT_IOAPIC *ioapic = (MADT_IOAPIC *)entry;
            ioapic_phys_addr = ioapic->IoApicAddress;
            break;
        }
        case 2:
        { // Interrupt Overrides
            MADT_InterruptOverride *ovr = (MADT_InterruptOverride *)entry;
            // You'll need these later to know that IRQ 0 is actually GSI 2
            break;
        }
        case 5:
        { // 64-bit Local APIC Address Override
            MADT_LocalApicOverride *lapic64 = (MADT_LocalApicOverride *)entry;
            lapic_phys_addr = lapic64->LocalApicAddress;
            break;
        }
        }
        ptr += entry->Length;
    }
}

void ACPIDriver::Reboot()
{
    if (fadt && (fadt->Flags & (1 << 10)))
    { // Check ResetReg support
        if (fadt->ResetReg.AddressSpace == 1)
        { // I/O Space
            outb(fadt->ResetReg.Address, fadt->ResetValue);
        }
    }
    // Legacy 8042 Keyboard Controller fallback
    outb(0x64, 0xFE);
    while (1)
    {
        __asm__ volatile("hlt");
    }
}

void *ACPIDriver::FindTable(const char *sig)
{
    int entry_size = is_xsdt ? 8 : 4;
    int entries = (root_table->Length - sizeof(ACPIHeader)) / entry_size;

    for (int i = 0; i < entries; i++)
    {
        ACPIHeader *table;
        if (is_xsdt)
        {
            uint64_t *ptrs = (uint64_t *)((uint8_t *)root_table + sizeof(ACPIHeader));
            table = (ACPIHeader *)ptrs[i];
        }
        else
        {
            uint32_t *ptrs = (uint32_t *)((uint8_t *)root_table + sizeof(ACPIHeader));
            table = (ACPIHeader *)(uint64_t)ptrs[i];
        }

        // Compare first 4 bytes of signature
        if (table->Signature[0] == sig[0] && table->Signature[1] == sig[1] &&
            table->Signature[2] == sig[2] && table->Signature[3] == sig[3])
        {
            return (void *)table;
        }
    }
    return nullptr;
}

void ACPIDriver::WriteACPI(GenericAddressStructure &reg, uint32_t value)
{
    if (reg.AddressSpace == 0)
    { // System Memory (MMIO)
        *(volatile uint32_t *)(reg.Address) = value;
    }
    else
    { // System I/O
        if (reg.BitWidth == 32)
            outl(reg.Address, value);
        else if (reg.BitWidth == 16)
            outw(reg.Address, value);
        else
            outb(reg.Address, value);
    }
}

void ACPIDriver::Shutdown()
{
    __asm__ volatile("cli");
    if (!fadt)
    {
        // Emergency QEMU exit if ACPI is missing
        outw(0x604, 0x2000);
        return;
    }

    // 1. SMI Handshake: Some systems require "enabling" ACPI via the SMI port
    // If bit 0 of PM1a_CNT isn't set, ACPI might not be enabled.
    if ((inw(fadt->PM1aControlBlock) & 1) == 0)
    {
        if (fadt->SMI_CommandPort != 0 && fadt->AcpiEnable != 0)
        {
            outb(fadt->SMI_CommandPort, fadt->AcpiEnable);
            // Wait for ACPI to enable
            int timeout = 0;
            while (timeout < 1000 && (inw(fadt->PM1aControlBlock) & 1) == 0)
            {
                __asm__ volatile("pause");
                timeout++;
            }
        }
    }

    uint64_t dsdt_addr = (fadt->X_Dsdt != 0) ? fadt->X_Dsdt : (uint64_t)fadt->Dsdt;
    ACPIHeader *dsdt = (ACPIHeader *)dsdt_addr;

    // Byte-by-byte check to protect RAX from string poisoning
    if (dsdt->Signature[0] != 'D' || dsdt->Signature[1] != 'S' ||
        dsdt->Signature[2] != 'D' || dsdt->Signature[3] != 'T')
    {
        outw(0x604, 0x2000); // QEMU Fallback
        return;
    }

    uint8_t *ptr = (uint8_t *)dsdt + sizeof(ACPIHeader);
    uint32_t len = dsdt->Length - sizeof(ACPIHeader);

    for (uint32_t i = 0; i < len - 4; i++)
    {
        if (ptr[i] == '_' && ptr[i + 1] == 'S' && ptr[i + 2] == '5' && ptr[i + 3] == '_')
        {
            uint8_t *s5_ptr = &ptr[i] + 4;

            // Robust AML parsing
            if (*s5_ptr == 0x12)
                s5_ptr++; // Skip OpCode
            if (*s5_ptr >= 0x40)
                s5_ptr++; // Multi-byte PkgLength
            s5_ptr++;     // PkgLength byte
            s5_ptr++;     // NumElements

            uint16_t SLP_TYPa, SLP_TYPb;

            // Check for BytePrefix (0x0A) or raw byte
            if (*s5_ptr == 0x0A)
                s5_ptr++;
            SLP_TYPa = (uint16_t)(*(s5_ptr++)) << 10;

            if (*s5_ptr == 0x0A)
                s5_ptr++;
            SLP_TYPb = (uint16_t)(*(s5_ptr++)) << 10;

            uint16_t SLP_EN = 1 << 13;

            // Perform the shutdown write
            outw(fadt->PM1aControlBlock, SLP_TYPa | SLP_EN);
            if (fadt->PM1bControlBlock != 0)
            {
                outw(fadt->PM1bControlBlock, SLP_TYPb | SLP_EN);
            }
            break;
        }
    }

    // FINAL FALLBACK: QEMU-specific debug exit
    // If ACPI fails, this magic port forces QEMU to close
    outw(0x604, 0x2000);
    while (1)
    {
        __asm__ volatile("hlt");
    }
}