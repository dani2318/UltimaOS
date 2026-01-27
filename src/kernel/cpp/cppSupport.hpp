#pragma once
#include <stddef.h> 
#include <stdint.h> 

using constructor = void (*)();
extern "C" constructor __init_array_start;
extern "C" constructor __init_array_end;

inline void callGlobalConstructors() {
    for (constructor* i = &__init_array_start; i < &__init_array_end; i++) {
        if (*i != nullptr) (*i)();
    }
}

static bool gConstructorsInitialized = false;

// Global flag to prevent double initialization
static bool gConstructorsCalled = false;

// Call all global constructors
inline void initializeConstructors()
{
    // Prevent calling twice
    if (gConstructorsCalled) {
        return;
    }
    gConstructorsCalled = true;
    
    // Get constructor array bounds from linker symbols
    constructor* ctorStart = &__init_array_start;
    constructor* ctorEnd = &__init_array_end;
    
    // Sanity check: make sure pointers are valid
    if ((uint64_t)ctorEnd < (uint64_t)ctorStart) {
        // Invalid constructor table, skip
        return;
    }
    
    // Calculate number of constructors
    uint64_t const count = ((uint64_t)ctorEnd - (uint64_t)ctorStart) / sizeof(constructor);
    
    // Safety check: don't call too many (probable corruption)
    if (count > 10000) {
        return;
    }
    
    // Call each constructor
    for (constructor* ctor = ctorStart; ctor < ctorEnd; ctor++) {
        constructor fn = *ctor;
        if (fn != nullptr && fn != (constructor)0xFFFFFFFFFFFFFFFFULL) {
            fn();
        }
    }
}