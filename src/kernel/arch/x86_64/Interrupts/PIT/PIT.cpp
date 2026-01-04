#include "PIT.hpp"
#include <arch/x86_64/Serial.hpp>

#define PIT_CHANNEL0    0x40
#define PIT_COMMAND     0x43
#define PIT_FREQUENCY   1193182  // Base frequency of PIT in Hz

uint64_t Timer::ticks = 0;
uint32_t Timer::frequency = 0;

void Timer::Initialize(uint32_t freq) {
    frequency = freq;
    ticks = 0;
    
    // Calculate divisor for desired frequency
    uint32_t divisor = PIT_FREQUENCY / freq;
    
    // Send command byte: Channel 0, lobyte/hibyte, rate generator
    outb(PIT_COMMAND, 0x36);
    
    // Send frequency divisor
    outb(PIT_CHANNEL0, divisor & 0xFF);         // Low byte
    outb(PIT_CHANNEL0, (divisor >> 8) & 0xFF);  // High byte
}

void Timer::OnInterrupt() {
    ticks++;
}

uint64_t Timer::GetTicks() {
    return ticks;
}

uint64_t Timer::GetUptimeSeconds() {
    return ticks / frequency;
}

void Timer::Sleep(uint32_t milliseconds) {
    uint64_t target = ticks + (milliseconds * frequency / 1000);
    while (ticks < target) {
        asm volatile("hlt");  // Halt until next interrupt
    }
}