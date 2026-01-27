#pragma once
#include <stdint.h>
#include <arch/x86_64/Interrupts/TSS.hpp>
#include <memory/memory.hpp>

struct GDTEntry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_middle;
    uint8_t access;
    uint8_t granularity;
    uint8_t base_high;
} __attribute__((packed));

struct GDTPointer {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

class GDT {
public:
    static void Initialize();
};