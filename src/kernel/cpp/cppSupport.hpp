#pragma once
#include <stddef.h> // For size_t
#include <globals.hpp>

extern "C" void __cxa_pure_virtual() {
    __asm__ volatile("hlt");
}

void* operator new(size_t size) {
    size_t aligned_size = (size + 15) & ~15;
    return g_heap.Alloc(aligned_size);
}

void operator delete(void* p) {
    g_heap.Free(p);
}

typedef void (*constructor)();
extern "C" constructor __init_array_start;
extern "C" constructor __init_array_end;

void callGlobalConstructors() {
    for (constructor* i = &__init_array_start; i < &__init_array_end; i++) {
        (*i)();
    }
}
typedef void (*constructor)();
extern "C" constructor __ctors_start;
extern "C" constructor __ctors_end;

extern "C" void InitializeConstructors() {
    for (constructor* cptr = &__init_array_start; cptr < & __init_array_end; cptr++) {
        if (*cptr) (*cptr)();
    }
}