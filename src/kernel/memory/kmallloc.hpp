#pragma once

#pragma once
#include <stdint.h>
#include <stddef.h>

// Kernel memory allocation functions
// These wrap the HeapAllocator in a standard C-style interface

#ifdef __cplusplus
extern "C" {
#endif

// Basic allocation/deallocation
void* kmalloc(uint64_t size);
void kfree(void* ptr);

// Zeroed allocation
void* kcalloc(uint64_t count, uint64_t size);

// Reallocation
void* krealloc(void* ptr, uint64_t new_size);

// Aligned allocation (for DMA, page tables, etc.)
void* kmemalign(uint64_t alignment, uint64_t size);
void kfree_aligned(void* ptr);

// Heap statistics
struct KernelHeapStats {
    uint64_t total_allocated;
    uint64_t total_free;
    uint64_t largest_free_block;
    uint64_t allocation_count;
};

void kheap_stats(KernelHeapStats* stats);
void kdump_heap();

#ifdef __cplusplus
}
#endif