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

#ifndef ALLOC_DEF
#define ALLOC_DEF


class BumpAllocator{
    private:
        uint64_t next_address;
        uint64_t limit;
    public:
        inline void Init(
            uint64_t start,
            uint64_t size
        ) {
            next_address = start;
            limit = start + size;
        }

        inline void* Alloc(
            size_t size
        ) {
            uint64_t aligned_size = (size + 7) & ~((uint64_t)7);

            if (next_address + aligned_size > limit) {
                return NULL;
            }

            void* ptr = (void*)next_address;
            next_address += aligned_size;
            return ptr;
        }


} ;

struct MemorySegmentHeader {
    size_t size;
    MemorySegmentHeader* next;
    MemorySegmentHeader* prev;
    bool free;

    void CombineForward() {
        if (next == nullptr || !next->free) return;
        size += next->size + sizeof(MemorySegmentHeader);
        next = next->next;
        if (next != nullptr) next->prev = this;
    }
};

class HeapAllocator {
    MemorySegmentHeader* first_block;

public:
    void Init(void* addr, size_t size) {
        first_block = (MemorySegmentHeader*)addr;
        first_block->size = size - sizeof(MemorySegmentHeader);
        first_block->next = nullptr;
        first_block->prev = nullptr;
        first_block->free = true;
    }

    void* Alloc(size_t size) {
        size = (size + 7) & ~7;
        MemorySegmentHeader* current = first_block;

        while (current != nullptr) {
            if (current->free && current->size >= size) {
                if (current->size > size + sizeof(MemorySegmentHeader) + 8) {
                    MemorySegmentHeader* new_block = (MemorySegmentHeader*)((uint8_t*)current + sizeof(MemorySegmentHeader) + size);
                    
                    new_block->size = current->size - size - sizeof(MemorySegmentHeader);
                    new_block->free = true;
                    new_block->next = current->next;
                    new_block->prev = current;
                    
                    if (current->next) current->next->prev = new_block;
                    current->next = new_block;
                    current->size = size;
                }
                
                current->free = false;
                return (void*)((uint8_t*)current + sizeof(MemorySegmentHeader));
            }
            current = current->next;
        }
        return nullptr; // Out of memory
    }

    void Free(void* ptr) {
        if (!ptr) return;
        MemorySegmentHeader* block = (MemorySegmentHeader*)((uint8_t*)ptr - sizeof(MemorySegmentHeader));
        block->free = true;

        block->CombineForward();
        if (block->prev && block->prev->free) {
            block->prev->CombineForward();
        }
    }
};



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