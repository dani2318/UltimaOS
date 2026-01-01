#pragma once 
#include <stdint.h>

typedef struct {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_middle;
    uint8_t  access;
    uint8_t  attributes;
    uint8_t  base_high;
} __attribute__((packed)) GDTEntry;

typedef struct {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed)) GDTDescriptor;

constexpr uint8_t GDT_ACCESS_CODE = 0b10011010;
constexpr uint8_t GDT_ACCESS_DATA = 0b10010010;
constexpr uint8_t GDT_FLAGS_CODE = 0b00100000;
constexpr uint8_t GDT_FLAGS_DATA = 0b00000000;


GDTEntry g_GDT[3];
GDTDescriptor g_GDTDescriptor;

void GDT_Init() {
    g_GDT[0] = {0,0,0,0,0,0};
    g_GDT[1] = {0,0,0,GDT_ACCESS_CODE,GDT_FLAGS_CODE,0};
    g_GDT[2] = {0,0,0,GDT_ACCESS_DATA,GDT_FLAGS_DATA,0};

    g_GDTDescriptor.limit = sizeof(g_GDT) - 1;
    g_GDTDescriptor.base  = (uint64_t)&g_GDT;

    __asm__ volatile("lgdt %0" : : "m"(g_GDTDescriptor));

    __asm__ volatile (
        "pushq $0x08\n"
        "lea 1f(%%rip), %%rax\n"
        "pushq %%rax\n"
        "lretq\n"
        "1:\n"
        : : : "rax"
    );

    __asm__ volatile (
        "mov $0x10, %ax\n"
        "mov %ax, %ds\n"
        "mov %ax, %es\n"
        "mov %ax, %ss\n"
        "mov %ax, %fs\n"
        "mov %ax, %gs\n"
    );

    g_screenwriter->Print("GDT OK!");
}
