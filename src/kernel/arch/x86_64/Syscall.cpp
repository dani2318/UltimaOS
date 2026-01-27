#include "Syscall.hpp"
#include <arch/x86_64/ScreenWriter.hpp>
#include <arch/x86_64/Multitasking/Scheduler.hpp>
#include <globals.hpp>
#include <Drivers/Input/Keyboard.hpp>

// MSR Address constants
#define MSR_EFER         0xC0000080
#define MSR_STAR         0xC0000081
#define MSR_LSTAR        0xC0000082
#define MSR_SFMASK       0xC0000084

// Inline helpers for MSR access
static inline void wrmsr(uint32_t msr, uint64_t value) {
    uint32_t low = (uint32_t)value;
    uint32_t high = (uint32_t)(value >> 32);
    asm volatile("wrmsr" : : "c"(msr), "a"(low), "d"(high));
}

static inline uint64_t rdmsr(uint32_t msr) {
    uint32_t low, high;
    asm volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(msr));
    return ((uint64_t)high << 32) | low;
}

void Syscall::Initialize() {
    uint64_t efer = rdmsr(MSR_EFER);
    wrmsr(MSR_EFER, efer | 1);

    wrmsr(MSR_LSTAR, (uint64_t)syscall_entry);

    uint64_t star = (0x13ULL << 48) | (0x08ULL << 32);
    wrmsr(MSR_STAR, star);

    wrmsr(MSR_SFMASK, 0x200);
}

// This function is called by 'syscall_entry' in assembly
extern "C" uint64_t syscall_handler(uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t num) {
    return Syscall::Handler(arg1, arg2, arg3, num);
}

uint64_t Syscall::Handler(uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t num) {
    switch (num) {
        case PRINT:
            gScreenwriter->print((const char*)arg1, false);
            return 0;

        case EXIT:
            gScheduler->Yield();
            return 0;

        case GET_CHAR:
            if (!Keyboard::HasChar()) return 0;
            return (uint64_t)Keyboard::GetChar();

        case CLEAR:
            gScreenwriter->clear(arg1);
            gScreenwriter->setCursor(0, 0);
            return 0;

        default:
            return (uint64_t)-1;
    }
}
