#include <cpp/cppSupport.hpp>
#include <globals.hpp>
#include <memory/memory.hpp> // <--- THIS IS CRITICAL: It defines the HeapAllocator class

extern "C" void __cxa_pure_virtual() {
    __asm__ volatile("hlt");
}

void* operator new(size_t size) {
    size_t aligned_size = (size + 15) & ~15;
    return g_heap.Alloc(aligned_size); // Now compiler knows what .Alloc is
}

void operator delete(void* p) {
    if (p == nullptr) return;
    g_heap.Free(p); // Now compiler knows what .Free is
}

void operator delete(void* p, size_t size) {
    (void)size;
    g_heap.Free(p);
}