#include <stddef.h>
#include <stdint.h>
#include <memory/memory.hpp>

extern "C" void* malloc(size_t size){
        return g_allocator.alloc(size);
}

void* operator new(size_t size) {
    return malloc(size);
}

void operator delete(void* p, size_t size) {
    // Basic free logic
}