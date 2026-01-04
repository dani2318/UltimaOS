#pragma once
#include <stdint.h>

class Timer {
public:
    static void Initialize(uint32_t frequency);
    static uint64_t GetTicks();
    static uint64_t GetUptimeSeconds();
    static void Sleep(uint32_t milliseconds);
    static void OnInterrupt();  // Called from IRQ 0 handler
    
private:
    static uint64_t ticks;
    static uint32_t frequency;
};