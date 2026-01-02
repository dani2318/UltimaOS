#include <arch/x86_64/GDT/GDT.hpp>
#include <stdint.h>


#define GDT_LIMIT_LOW(limit)                    (limit & 0xFFFF)
#define GDT_BASE_LOW(base)                      (base  & 0xFFFF)
#define GDT_BASE_MIDDLE(base)                   ((base >> 16)  & 0xFF)
#define GDT_FLAGS_LIMIT_HI(limit, flags)        ((limit >> 16) & 0xF | (flags & 0xF0))
#define GDT_BASE_HIGH(base)                     ((base >> 24) & 0xFF)

#define GDT_ENTRY(base, limit, access, flags) { \
    GDT_LIMIT_LOW(limit),                       \
    GDT_BASE_LOW(base),                         \
    GDT_BASE_MIDDLE(base),                      \
    access,                                     \
    GDT_FLAGS_LIMIT_HI(limit, flags),           \
    GDT_BASE_HIGH(base)                         \
}

GDTEntry g_GDT[] = {
    // NULL
    GDT_ENTRY(0,0,0,0),
    // Kernel 64-bit code segment                        
    GDT_ENTRY(0,
              0xFFFFF,
              GDT_ACCESS_PRESENT | GDT_ACCESS_RING0 | GDT_ACCESS_CODE_SEGMENT | GDT_ACCESS_CODE_READABLE,
              GDT_FLAG_64BIT | GDT_FLAG_GRANULARITY_4K),
    // Kernel 64-bit data segment                        
    GDT_ENTRY(0,
              0xFFFFF,
              GDT_ACCESS_PRESENT | GDT_ACCESS_RING0 | GDT_ACCESS_DATA_SEGMENT | GDT_ACCESS_CODE_WRITABLE,
              GDT_FLAG_64BIT | GDT_FLAG_GRANULARITY_4K),
};

GDTDescriptor g_GDTDescriptor = {sizeof(g_GDT) - 1, g_GDT};

EXTERNC void x86_64_GDT_Load(GDTDescriptor* descriptor, uint16_t codeSegment, uint16_t dataSegment);

void GDT::Init(){
    x86_64_GDT_Load(&g_GDTDescriptor, i686_GDT_CODE_SEGMENT, i686_GDT_DATA_SEGMENT);
    g_screenwriter->Print("GDT OK!");
}