#pragma once 
#include <stdint.h>

// GDT Entry structure
typedef struct {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_middle;
    uint8_t  access;
    uint8_t  attributes;
    uint8_t  base_high;
} __attribute__((packed)) GDTEntry;

// The structure you pass to the LGDT instruction
typedef struct {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed)) GDTDescriptor;

// Define your GDT
GDTEntry g_GDT[3]; // Null, Code, Data
GDTDescriptor g_GDTDescriptor;

void GDT_Init() {
    // Null Descriptor
    g_GDT[0] = (GDTEntry){0, 0, 0, 0, 0, 0};
    
    // Kernel Code: Access 0x9A (Present, Ring 0, Exec/Read), Flags 0x2 (Long Mode)
    g_GDT[1] = (GDTEntry){0, 0, 0, 0x9A, 0xAF, 0};
    
    // Kernel Data: Access 0x92 (Present, Ring 0, Read/Write)
    g_GDT[2] = (GDTEntry){0, 0, 0, 0x92, 0xCF, 0};

    g_GDTDescriptor.limit = sizeof(g_GDT) - 1;
    g_GDTDescriptor.base = (uint64_t)&g_GDT;

    // Load it using Assembly
    __asm__ volatile("lgdt %0" : : "m"(g_GDTDescriptor));
}