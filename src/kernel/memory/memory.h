#pragma once
#include <stdint.h>
#include <stddef.h>

typedef struct {
    uint32_t Type;
    uint32_t Pad;
    uint64_t PhysicalStart;  // Start of this chunk
    uint64_t VirtualStart;   // Usually 0 or same as Physical in bootloader
    uint64_t NumberOfPages;  // Size in 4KB pages
    uint64_t Attribute;
} MemoryDescriptor;

// The most important types for your kernel
enum MemoryType {
    EfiReservedMemoryType,
    EfiLoaderCode,
    EfiLoaderData,
    EfiBootServicesCode,
    EfiBootServicesData,
    EfiRuntimeServicesCode,
    EfiRuntimeServicesData,
    EfiConventionalMemory,
    EfiUnusableMemory,
    EfiACPIReclaimMemory,
    EfiACPIMemoryNVS,
    EfiMemoryMappedIO,
    EfiMemoryMappedIOPortSpace,
    EfiPalCode,
    EfiMaxMemoryType
};

#ifndef ALLOC_DEF
#define ALLOC_DEF

/* =========================
   BUMP ALLOCATOR
   ========================= */

typedef struct {
    uint64_t next_address;
    uint64_t limit;
} BumpAllocator;

/* Initialize allocator */
static inline void BumpAllocator_Init(
    BumpAllocator* alloc,
    uint64_t start,
    uint64_t size
) {
    alloc->next_address = start;
    alloc->limit = start + size;
}

/* Allocate memory */
static inline void* BumpAllocator_Alloc(
    BumpAllocator* alloc,
    size_t size
) {
    /* Align to 8 bytes */
    uint64_t aligned_size = (size + 7) & ~((uint64_t)7);

    if (alloc->next_address + aligned_size > alloc->limit) {
        return NULL; /* Out of memory */
    }

    void* ptr = (void*)alloc->next_address;
    alloc->next_address += aligned_size;
    return ptr;
}


#endif

static void * memcpy(void * dst, const void * src, uint16_t num){
    uint8_t* u8Dst = (uint8_t *)dst;
    const uint8_t* u8Src = (const uint8_t *)src;

    for (uint16_t i = 0; i < num; i++)
        u8Dst[i] = u8Src[i];

    return dst;
}

static void * memset(void * ptr, int value, uint16_t num){
    uint8_t * u8Ptr = (uint8_t *)ptr;

    for(uint16_t i = 0; i < num; i++)
        u8Ptr[i] = (uint8_t)value;

    return ptr;
}

static int memcmp(const void * ptr1, const void * ptr2, uint16_t num){
    const uint8_t* u8Ptr1 = (const uint8_t *)ptr1;
    const uint8_t* u8Ptr2 = (const uint8_t *)ptr2;

    for (uint16_t i = 0; i < num; i++)
        if (u8Ptr1[i] != u8Ptr2[i])
            return 1;

    return 0;
}