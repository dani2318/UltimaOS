#pragma once
#include <stdint.h>


struct Framebuffer{
    uint64_t BaseAddress;
    uint64_t BufferSize;
    uint32_t Width;
    uint32_t Height;
    uint32_t PixelsPerScanLine;
} ;

typedef struct Framebuffer Framebuffer;

typedef struct {
    void* kernel_entry;
    void* rsdt_address;      
    void* mmap_address;      
    unsigned int mmap_size;
    unsigned int descriptor_size; // Added: kernel needs this to parse mmap
    void* font_address;
    Framebuffer framebuffer;     // Added: Graphics info
    uint64_t kernel_size;
} BootInfo;

// In your kernel main.c or acpi.h
struct RSDPDescriptor {
    char Signature[8];      // Should be "RSD PTR "
    uint8_t Checksum;
    char OEMID[6];
    uint8_t Revision;
    uint32_t RsdtAddress;   // This is the 32-bit pointer to the RSDT
} __attribute__ ((packed));

struct RSDPDescriptor20 {
    struct RSDPDescriptor firstPart;
    uint32_t Length;
    uint64_t XsdtAddress;   // This is the 64-bit pointer to the XSDT (Modern)
    uint8_t ExtendedChecksum;
    uint8_t reserved[3];
} __attribute__ ((packed));

