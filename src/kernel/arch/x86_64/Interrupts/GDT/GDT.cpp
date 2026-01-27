#include "GDT.hpp"

extern "C" void gdt_flush(uint64_t);

static GDTEntry gdt[10];
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

    // Standard segments (0x00 to 0x20)
    set_gdt_entry(0, 0, 0, 0, 0);                // 0x00: Null
    set_gdt_entry(1, 0, 0xFFFFFFFF, 0x9A, 0xA0); // 0x08: Kernel Code
    set_gdt_entry(2, 0, 0xFFFFFFFF, 0x92, 0xA0); // 0x10: Kernel Data
    set_gdt_entry(3, 0, 0xFFFFFFFF, 0xFA, 0xA0); // 0x18: User Code
    set_gdt_entry(4, 0, 0xFFFFFFFF, 0xF2, 0xA0); // 0x20: User Data

    // TSS Descriptor (Indices 5 and 6)
    uint64_t tss_base = (uint64_t)&g_tss;
    uint32_t tss_limit = sizeof(TSS) - 1;

    TSSEntry* tss_desc = (TSSEntry*)&gdt[5];
    tss_desc->limit_low = tss_limit;
    tss_desc->base_low = tss_base & 0xFFFF;
    tss_desc->base_mid = (tss_base >> 16) & 0xFF;
    tss_desc->access = 0x89; 
    tss_desc->flags = 0x40;  
    tss_desc->base_high = (tss_base >> 24) & 0xFF;
    tss_desc->base_upper = (tss_base >> 32) & 0xFFFFFFFF;

    gdt_ptr.limit = (sizeof(GDTEntry) * 8) - 1; // Limit for 8 entries (0-7)
    gdt_ptr.base = (uint64_t)&gdt;
    
    gdt_flush((uint64_t)&gdt_ptr);

    asm volatile("ltr %%ax" : : "a"((uint16_t)0x28));
}