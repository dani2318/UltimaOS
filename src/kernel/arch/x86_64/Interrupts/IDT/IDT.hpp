#pragma once
#include <stdint.h>

struct IDTEntry{
    uint16_t offset_low;
    uint16_t selector;
    uint8_t ist;
    uint8_t typeattr;
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t reserved;
} __attribute__((packed));
typedef struct IDTEntry IDTEntry;

struct IDTDescriptor{
    uint16_t Limit;
    uint64_t ptr;  // Changed from IDTEntry* to uint64_t
} __attribute__((packed));
typedef struct IDTDescriptor IDTDescriptor;

extern "C" void IDTLoad(IDTDescriptor* idtDescriptor);

constexpr int IDT_ENTRIES = 256;
static IDTEntry idt[IDT_ENTRIES];
static IDTDescriptor idtr = {sizeof(idt) - 1, (uint64_t)idt};  // Cast to uint64_t

extern void isr_common(void);

class IDT{
    public:
        static void setIDTEntry(int index, uint64_t handler, uint16_t selector, uint8_t typeattr);
        static void Initialize();
};