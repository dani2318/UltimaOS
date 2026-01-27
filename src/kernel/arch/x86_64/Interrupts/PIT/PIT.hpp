#pragma once
#include <stdint.h>

class Timer {
public:
    static void initialize(uint32_t frequency);
    static uint64_t getTicks();
    static uint64_t getUptimeSeconds();
    static void sleep(uint32_t milliseconds);
    static void onInterrupt();  // Called from IRQ 0 handler
    
private:
    static uint64_t ticks;
    static uint32_t frequency;
};