#pragma once
#include <stdint.h>

class MSR
{
public:
    static inline uint64_t Read(uint32_t msr)
    {
        uint32_t low, high;
        asm volatile(
            "rdmsr"
            : "=a"(low), "=d"(high)
            : "c"(msr)
        );
        return ((uint64_t)high << 32) | low;
    }

    static inline void Write(uint32_t msr, uint64_t value)
    {
        uint32_t low  = (uint32_t)value;
        uint32_t high = (uint32_t)(value >> 32);
        asm volatile(
            "wrmsr"
            :
            : "c"(msr), "a"(low), "d"(high)
        );
    }
};
