#include <stdint.h>
#include <stddef.h>
#include <memory/memory.hpp>
#include <arch/x86_64/ScreenWriter.hpp>
#include <cpp/String.hpp>
#include <globals.hpp>

// Kernel heap allocator wrapper
// This provides C-style malloc/free interface for kernel code

extern "C" void* kmalloc(uint64_t size) {
    if (size == 0) {
        return nullptr;
    }

    // Use the global heap allocator
    void* ptr = gHeap.alloc(size);

    if (ptr == nullptr) {
        // Optional: Add debug logging
        if (gScreenwriter != nullptr) {
            gScreenwriter->print("kmalloc: Out of memory (requested ", true);
            char buf[32];
            gScreenwriter->print(itoa(size, buf, 10), true);
            gScreenwriter->print(" bytes)\n", true);
        }
        return nullptr;
    }

    return ptr;
}

extern "C" void kfree(void* ptr) {
    if (ptr == nullptr) {
        return;
    }

    // Use the global heap allocator
    gHeap.free(ptr);
}

// Optional: Additional utility functions for kernel memory management

extern "C" void* kcalloc(uint64_t count, uint64_t size) {
    uint64_t const total = count * size;

    // Check for overflow
    if (count != 0 && total / count != size) {
        return nullptr;
    }

    void* ptr = kmalloc(total);
    if (ptr != nullptr) {
        memset(ptr, 0, total);
    }

    return ptr;
}

extern "C" void* krealloc(void* ptr, uint64_t newSize) {
    if (ptr == nullptr) {
        return kmalloc(newSize);
    }

    if (newSize == 0) {
        kfree(ptr);
        return nullptr;
    }

    // Allocate new block
    void* newPtr = kmalloc(newSize);
    if (newPtr == nullptr) {
        return nullptr;
    }

    // Get old size from header
    MemorySegmentHeader* oldHeader = (MemorySegmentHeader*)((uint8_t*)ptr - sizeof(MemorySegmentHeader));
    size_t const oldSize = oldHeader->size;

    // Copy old data
    size_t const copySize = (oldSize < newSize) ? oldSize : newSize;
    memcpy(newPtr, ptr, copySize);

    // Free old block
    kfree(ptr);

    return newPtr;
}

// Aligned allocation for DMA and special requirements
extern "C" void* kmemalign(uint64_t alignment, uint64_t size) {
    if (alignment == 0 || (alignment & (alignment - 1)) != 0) {
        // Alignment must be power of 2
        return nullptr;
    }

    // Allocate extra space for alignment adjustment
    uint64_t const allocSize = size + alignment + sizeof(void*);
    void* rawPtr = kmalloc(allocSize);

    if (rawPtr == nullptr) {
        return nullptr;
    }

    // Calculate aligned address
    uint64_t const rawAddr = (uint64_t)rawPtr;
    uint64_t const alignedAddr = (rawAddr + sizeof(void*) + alignment - 1) & ~(alignment - 1);

    // Store original pointer before aligned address
    void** storedPtr = (void**)(alignedAddr - sizeof(void*));
    *storedPtr = rawPtr;

    return (void*)alignedAddr;
}

extern "C" void kfreeAligned(void* ptr) {
    if (ptr == nullptr) {
        return;
    }

    // Retrieve original pointer
    void** storedPtr = (void**)((uint64_t)ptr - sizeof(void*));
    void* rawPtr = *storedPtr;

    kfree(rawPtr);
}

// Get allocation statistics
struct KernelHeapStats {
    uint64_t totalAllocated;
    uint64_t totalFree;
    uint64_t largestFreeBlock;
    uint64_t allocationCount;
};

extern "C" void kheapStats(KernelHeapStats* stats) {
    if (stats == nullptr) return;

    stats->totalAllocated = 0;
    stats->totalFree = 0;
    stats->largestFreeBlock = 0;
    stats->allocationCount = 0;

    // Walk the heap to gather statistics
    MemorySegmentHeader* current = (MemorySegmentHeader*)((uint8_t*)&gHeap - offsetof(HeapAllocator, first_block));

    // This assumes we can access first_block - you may need to add a getter method
    // For now, this is a simplified version
}

// Debugging: Dump heap state
extern "C" void kdumpHeap() {
    if (gScreenwriter == nullptr) return;

    gScreenwriter->print("\n=== Kernel Heap Dump ===\n", true);

    // Note: This requires access to HeapAllocator's first_block
    // You may need to add a public method to HeapAllocator for this

    gScreenwriter->print("Heap dump not fully implemented\n", true);
    gScreenwriter->print("(requires HeapAllocator API extension)\n", true);
}
