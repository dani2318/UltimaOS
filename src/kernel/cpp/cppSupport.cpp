#include <cpp/cppSupport.hpp>
#include <arch/x86_64/Paging.hpp>
#include <globals.hpp>
#include <memory/memory.hpp> // <--- THIS IS CRITICAL: It defines the HeapAllocator class

extern "C" void __cxa_pure_virtual() {
    __asm__ volatile("hlt");
}

void* operator new(size_t size) {
    size_t aligned_size = (size + 15) & ~15;
    return g_heap.Alloc(aligned_size); // Now compiler knows what .Alloc is
}


void operator delete(void* p, size_t size) {
    (void)size;
    g_heap.Free(p);
}

void operator delete(void* ptr) noexcept {
    if (ptr) g_PMM.FreePage(ptr);
}

void operator delete[](void* ptr) noexcept {
    if (ptr) g_PMM.FreePage(ptr);
}

void operator delete[](void* ptr, size_t size) noexcept {
    if (ptr) g_PMM.FreePage(ptr);
}