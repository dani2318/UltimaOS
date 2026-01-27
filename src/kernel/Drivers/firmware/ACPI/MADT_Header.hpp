#pragma once
#include <stdint.h>
#include <Drivers/firmware/ACPI/ACPI_Header.hpp>

struct MADT {
    ACPIHeader Header;
    uint32_t LocalApicAddress; // Default physical address of the LAPIC
    uint32_t Flags;            // PC-AT Compatibility (Dual 8259) Flag
} __attribute__((packed));

struct MADT_EntryHeader {
    uint8_t Type;
    uint8_t Length;
} __attribute__((packed));

// Type 0: Processor Local APIC
struct MADT_LocalAPIC {
    MADT_EntryHeader Header;
    uint8_t ProcessorID;
    uint8_t ApicID;
    uint32_t Flags; // Bit 0: Enabled, Bit 1: Online Capable
} __attribute__((packed));

// Type 1: I/O APIC
struct MADT_IOAPIC {
    MADT_EntryHeader Header;
    uint8_t IoApicID;
    uint8_t Reserved;
    uint32_t IoApicAddress;
    uint32_t GlobalSystemInterruptBase;
} __attribute__((packed));

// Type 2: Interrupt Source Override (Maps ISA IRQs to I/O APIC pins)
struct MADT_InterruptOverride {
    MADT_EntryHeader Header;
    uint8_t BusSource; // Usually 0 (ISA)
    uint8_t IrqSource; // ISA IRQ (e.g., 0 for timer)
    uint32_t GlobalSystemInterrupt; // The actual I/O APIC pin
    uint16_t Flags; // Polarities and Trigger Modes
} __attribute__((packed));

// Type 5: Local APIC Address Override (64-bit address for LAPIC)
struct MADT_LocalApicOverride {
    MADT_EntryHeader Header;
    uint16_t Reserved;
    uint64_t LocalApicAddress;
} __attribute__((packed));