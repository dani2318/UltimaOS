#pragma once
#include <stddef.h> 

typedef void (*constructor)();
extern "C" constructor __init_array_start;
extern "C" constructor __init_array_end;

// Use 'inline' so multiple inclusions don't cause linker errors
inline void callGlobalConstructors() {
    for (constructor* i = &__init_array_start; i < &__init_array_end; i++) {
        if (*i) (*i)();
    }
}

extern "C" inline void InitializeConstructors() {
    callGlobalConstructors();
}