#include "Syscall.hpp"
#include <arch/x86_64/ScreenWriter.hpp>
#include <arch/x86_64/Multitasking/Scheduler.hpp>
#include <globals.hpp>
#include <arch/x86_64/Drivers/Keyboard.hpp>

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
    // 1. Enable SCE (System Call Extensions) in EFER
    uint64_t efer = rdmsr(MSR_EFER);
    wrmsr(MSR_EFER, efer | 1);

    // 2. Set the address of our assembly 'syscall_entry' in LSTAR
    wrmsr(MSR_LSTAR, (uint64_t)syscall_entry);

    // 3. Set up selectors in STAR
    // Bits 32-47: Kernel CS/SS (0x08)
    // Bits 48-63: User CS/SS (0x13 or 0x1B depending on your GDT)
    // Note: STAR expects the base, sysret adds +8 or +16 to find the actual selectors
    uint64_t star = (0x13ULL << 48) | (0x08ULL << 32);
    wrmsr(MSR_STAR, star);

    // 4. Set RFLAGS mask in SFMASK
    // We mask the Interrupt Flag (0x200) to disable interrupts during syscall entry
    wrmsr(MSR_SFMASK, 0x200);
}

// This function is called by 'syscall_entry' in assembly
extern "C" uint64_t syscall_handler(uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t num) {
    return Syscall::Handler(arg1, arg2, arg3, num);
}

uint64_t Syscall::Handler(uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t num) {
    switch (num) {
        case PRINT:
            g_screenwriter->Print((const char*)arg1);
            return 0;

        case EXIT:
            Scheduler::ExitTask();
            return 0;

        case GET_CHAR:
            if (!Keyboard::HasChar()) return 0;
            return (uint64_t)Keyboard::GetChar();

        case CLEAR:
            g_screenwriter->Clear(arg1);
            g_screenwriter->SetCursor(0, 0);
            return 0;

        default:
            return (uint64_t)-1;
    }
}