#pragma once
#include <stdint.h>
#include <stddef.h>

using MemoryDescriptor = struct
{
    uint32_t Type;
    uint32_t Pad;
    uint64_t PhysicalStart; // Start of this chunk
    uint64_t VirtualStart;  // Usually 0 or same as Physical in bootloader
    uint64_t NumberOfPages; // Size in 4KB pages
    uint64_t Attribute;
};

#ifndef ALLOC_DEF
#define ALLOC_DEF

class BumpAllocator
{
private:
    uint64_t m_next_address;
    uint64_t m_limit;

public:
    void init(
        uint64_t start,
        uint64_t size)
    {
        m_next_address = start;
        m_limit = start + size;
    }

    void *alloc(
        size_t size)
    {
        uint64_t const alignedSize = (size + 7) & ~((uint64_t)7);

        if (m_next_address + alignedSize > m_limit)
        {
            return nullptr;
        }

        void *ptr = (void *)m_next_address;
        m_next_address += alignedSize;
        return ptr;
    }
};

struct MemorySegmentHeader
{
    size_t size;
    MemorySegmentHeader *next;
    MemorySegmentHeader *prev;
    bool free;
    uint32_t magic; // Add magic number for validation

    void combineForward()
    {
        if (next == nullptr || !next->free)
            return;
        if (next->magic != 0xDEADBEEF)
            return; // Safety check

        size += next->size + sizeof(MemorySegmentHeader);
        next = next->next;
        if (next != nullptr)
            next->prev = this;
    }
};

class HeapAllocator
{
public:
    MemorySegmentHeader *first_block{nullptr};
    void *heap_start{nullptr};
    size_t heap_size{0};
    bool initialized{false};

    HeapAllocator()  {}

    void init(void *addr, size_t size)
    {
        if ((addr == nullptr) || size < sizeof(MemorySegmentHeader) + 64)
        {
            return;
        }

        heap_start = addr;
        heap_size = size;

        first_block = (MemorySegmentHeader *)addr;
        first_block->size = size - sizeof(MemorySegmentHeader);
        first_block->next = nullptr;
        first_block->prev = nullptr;
        first_block->free = true;
        first_block->magic = 0xDEADBEEF;

        initialized = true;
    }

    void *alloc(size_t size) const
    {
        if (!initialized || (first_block == nullptr))
        {
            // Heap not initialized
            return nullptr;
        }

        // Verify first_block is still valid
        if (first_block->magic != 0xDEADBEEF)
        {
            // Heap corrupted!
            return nullptr;
        }

        size = (size + 7) & ~7;

        if (size == 0)
        {
            return nullptr;
        }

        MemorySegmentHeader *current = first_block;
        int iterations = 0;
        const int MAX_ITERATIONS = 10000;

        while (current != nullptr && iterations < MAX_ITERATIONS)
        {
            iterations++;

            // Safety check: verify magic number
            if (current->magic != 0xDEADBEEF)
            {
                // Corrupted block, stop here
                return nullptr;
            }

            // Check if block is within heap bounds
            uint64_t const blockAddr = (uint64_t)current;
            uint64_t const heapEnd = (uint64_t)heap_start + heap_size;
            if (blockAddr < (uint64_t)heap_start || blockAddr >= heapEnd)
            {
                // Block is outside heap!
                return nullptr;
            }

            if (current->free && current->size >= size)
            {
                // Found a suitable block
                if (current->size > size + sizeof(MemorySegmentHeader) + 8)
                {
                    // Split the block
                    MemorySegmentHeader *newBlock = (MemorySegmentHeader *)((uint8_t *)current + sizeof(MemorySegmentHeader) + size);

                    newBlock->size = current->size - size - sizeof(MemorySegmentHeader);
                    newBlock->free = true;
                    newBlock->next = current->next;
                    newBlock->prev = current;
                    newBlock->magic = 0xDEADBEEF;

                    if (current->next != nullptr)
                    {
                        current->next->prev = newBlock;
                    }
                    current->next = newBlock;
                    current->size = size;
                }

                current->free = false;
                return (void *)((uint8_t *)current + sizeof(MemorySegmentHeader));
            }

            current = current->next;
        }

        return nullptr; // Out of memory or too many iterations
    }

    void free(void *ptr) const
    {
        if ((ptr == nullptr) || !initialized)
            return;

        // Check if pointer is within heap bounds
        uint64_t const ptrAddr = (uint64_t)ptr;
        uint64_t const heapEnd = (uint64_t)heap_start + heap_size;
        if (ptrAddr <= (uint64_t)heap_start || ptrAddr >= heapEnd)
        {
            // Pointer outside heap
            return;
        }

        MemorySegmentHeader *block = (MemorySegmentHeader *)((uint8_t *)ptr - sizeof(MemorySegmentHeader));

        // Verify magic number
        if (block->magic != 0xDEADBEEF)
        {
            // Corrupted or invalid block
            return;
        }

        block->free = true;

        block->combineForward();
        if ((block->prev != nullptr) && block->prev->free)
        {
            block->prev->combineForward();
        }
    }

    // Debug function
    void debugPrint(void (*printFunc)(const char *)) const
    {
        if ((printFunc == nullptr) || !initialized)
            return;

        printFunc("=== Heap Debug ===\n");

        char buf[64];
        printFunc("Heap start: 0x");
        // You'll need to implement hex printing
        printFunc("\n");

        printFunc("Heap size: ");
        // Print size
        printFunc("\n");

        if (first_block == nullptr)
        {
            printFunc("First block is NULL!\n");
            return;
        }

        if (first_block->magic != 0xDEADBEEF)
        {
            printFunc("First block corrupted!\n");
            return;
        }

        printFunc("First block OK\n");

        MemorySegmentHeader *current = first_block;
        int count = 0;
        while ((current != nullptr) && count < 100)
        {
            count++;
            printFunc("  Block ");
            // Print block info
            current = current->next;
        }
    }
};

#endif

static void *memcpy(void *dst, const void *src, size_t num) 
{
    uint8_t *u8Dst = (uint8_t *)dst;
    const uint8_t *u8Src = (const uint8_t *)src;
    for (size_t i = 0; i < num; i++)
        u8Dst[i] = u8Src[i];
    return dst;
}

static void *memset(void *ptr, int value, size_t num)
{
    uint8_t *u8Ptr = (uint8_t *)ptr;

    for (uint16_t i = 0; i < num; i++)
        u8Ptr[i] = (uint8_t)value;

    return ptr;
}

static int memcmp(const void *ptr1, const void *ptr2, size_t num)
{
    const uint8_t *u8Ptr1 = (const uint8_t *)ptr1;
    const uint8_t *u8Ptr2 = (const uint8_t *)ptr2;

    for (uint16_t i = 0; i < num; i++) {
        if (u8Ptr1[i] != u8Ptr2[i])
            return 1;
}

    return 0;
}