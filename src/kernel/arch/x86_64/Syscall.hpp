#pragma once
#include <stdint.h>

#pragma once
#include <stdint.h>

// Forward declaration of the assembly entry point
extern "C" void syscall_entry();

class Syscall {
public:
    enum SyscallNumber {
        PRINT = 0,
        EXIT  = 1,
        GET_CHAR = 2,
        CLEAR = 3
    };

    // Initializes the MSRs for the syscall instruction
    static void Initialize();

    // The central dispatcher called from assembly
    static uint64_t Handler(uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t num);
};