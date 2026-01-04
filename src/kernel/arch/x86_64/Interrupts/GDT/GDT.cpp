#include "GDT.hpp"

extern "C" void gdt_flush(uint64_t);

static GDTEntry gdt[5];
static GDTPointer gdt_ptr;

TSS g_tss;

static void set_gdt_entry(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt[num].base_low = (base & 0xFFFF);
    gdt[num].base_middle = (base >> 16) & 0xFF;
    gdt[num].base_high = (base >> 24) & 0xFF;
    
    gdt[num].limit_low = (limit & 0xFFFF);
    gdt[num].granularity = (limit >> 16) & 0x0F;
    gdt[num].granularity |= gran & 0xF0;
    gdt[num].access = access;
}

void GDT::Initialize() {
    memset(gdt, 0, sizeof(gdt));
    memset(&g_tss, 0, sizeof(TSS));
    g_tss.iomap_base = sizeof(TSS);
    


    gdt_ptr.limit = (sizeof(GDTEntry) * 5) - 1;
    gdt_ptr.base = (uint64_t)&gdt;
    
    set_gdt_entry(0, 0, 0, 0, 0);                // Null segment
    set_gdt_entry(1, 0, 0xFFFFFFFF, 0x9A, 0xA0); // Code segment (0x08)
    set_gdt_entry(2, 0, 0xFFFFFFFF, 0x92, 0xA0); // Data segment (0x10)
    set_gdt_entry(3, 0, 0xFFFFFFFF, 0xFA, 0xA0); // User code segment (0x18)
    set_gdt_entry(4, 0, 0xFFFFFFFF, 0xF2, 0xA0); // User data segment (0x20)
    // 4. TSS Descriptor (Entries 5 and 6)
    // The TSS descriptor is special: it's 16 bytes long.
    uint64_t tssBase = (uint64_t)&g_tss;
    uint64_t tssLimit = sizeof(TSS) - 1;

    // -- Entry 5 (Low 8 bytes) --
    gdt[5].limit_low = tssLimit & 0xFFFF;
    gdt[5].base_low = tssBase & 0xFFFF;
    gdt[5].base_middle = (tssBase >> 16) & 0xFF;
    gdt[5].access = 0x89; // Present, Ring 0, Type 0x9 (Available TSS)
    gdt[5].granularity = 0x00; // No granularity, limit is in bytes
    gdt[5].base_high = (tssBase >> 24) & 0xFF;

    // -- Entry 6 (High 8 bytes) --
    // This is the extension for 64-bit base addresses
    uint32_t* upper_tss = (uint32_t*)&gdt[6];
    upper_tss[0] = (uint32_t)(tssBase >> 32);
    upper_tss[1] = 0; // Reserved

    // 4. Load the GDT via the assembly wrapper
    gdt_flush((uint64_t)&gdt_ptr);

    asm volatile("ltr %0" : : "r"((uint16_t)0x28));
}