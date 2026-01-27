#include <stddef.h>
#include <stdint.h>
#include <globals.hpp>
#include <memory/memory.hpp>

extern "C" void __cxa_pure_virtual() {
    __asm__ volatile("hlt");
}

void* operator new(size_t size) {
    size_t const alignedSize = (size + 15) & ~15;
    return gHeap.alloc(alignedSize); // Now compiler knows what .Alloc is
}


void operator delete(void* p, size_t size) {
    (void)size;
    gHeap.free(p);
}

void operator delete(void* ptr) noexcept {
    if (ptr != nullptr) gPmm.freePage(ptr);
}

void operator delete[](void* ptr) noexcept {
    if (ptr != nullptr) gPmm.freePage(ptr);
}

void operator delete[](void* ptr, size_t  /*size*/) noexcept {
    if (ptr != nullptr) gPmm.freePage(ptr);
}

extern "C" int __cxa_guard_acquire(const uint64_t* guard) {
    // Return 1 if initialization needed, 0 if already initialized
    if (*((uint8_t*)guard) == 0) {
        return 1;
    }
    return 0;
}

extern "C" void __cxa_guard_release(uint64_t* guard) {
    // Mark as initialized
    *((uint8_t*)guard) = 1;
}

extern "C" void __cxa_guard_abort(uint64_t* guard) {
    // Called if initialization throws an exception
    // In kernel without exceptions, just do nothing
}
