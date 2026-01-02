#pragma once
#include <stdint.h>
#include <globals.hpp>

typedef struct  {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t  ist;
    uint8_t  type_attr;
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t zero;
} __attribute__((packed)) IDTEntry;

typedef struct  {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed)) IDTPtr;

extern "C" void lidt(void*);

class IDT {
    public:
        inline void SetIDTGate(int n, uint64_t handler);
        void Init(void);
    private:
        IDTEntry idt[256];
};
