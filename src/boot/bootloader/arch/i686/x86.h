#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <core/Defs.hpp>

void ASMCALL x86_outb(uint16_t port, uint8_t value);
uint8_t ASMCALL x86_inb(uint16_t port);

struct E820MemoryBlock{
    uint64_t Base;
    uint64_t Length;
    uint32_t Type;
    uint32_t ACPI;
};

enum E820MemoryBlockType{
    E820_USABLE             = 1,
    E820_RESERVED           = 2,
    E820_ACPI_RECLAIMABLE   = 3,
    E820_ACPI_NVS           = 4,
    E820_BAD_MEMORY         = 5,
};

int ASMCALL E820GetNextBlock(E820MemoryBlock* block, uint32_t* continuationId);