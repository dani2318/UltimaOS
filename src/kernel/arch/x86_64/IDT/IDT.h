#pragma once
#include <stdint.h>

typedef struct {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t  ist;
    uint8_t  type_attributes;
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t reserved;
} __attribute__((packed)) IDTEntry;

IDTEntry g_IDT[256];

void IDT_SetDescriptor(uint8_t vector, void* handler, uint8_t flags) {
    uint64_t addr = (uint64_t)handler;
    g_IDT[vector].offset_low = addr & 0xFFFF;
    g_IDT[vector].selector = 0x08; // Your Kernel Code Segment index
    g_IDT[vector].ist = 0;
    g_IDT[vector].type_attributes = flags;
    g_IDT[vector].offset_mid = (addr >> 16) & 0xFFFF;
    g_IDT[vector].offset_high = (addr >> 32) & 0xFFFFFFFF;
    g_IDT[vector].reserved = 0;
}