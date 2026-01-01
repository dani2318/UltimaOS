#include <stdint.h>

#include <globals.hpp>

extern "C" {

struct ISRFrame {
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
    uint64_t int_no;
    uint64_t err_code;
} __attribute__((packed));

void isr_handler(ISRFrame* frame) {
    (void)frame;
    g_screenwriter->Clear(0xFFFF0000);
    g_screenwriter->Print("ISR Exception triggered!\n");

    // If this runs, your interrupt works
    for (;;) {
        asm volatile ("hlt");
    }
}

}
