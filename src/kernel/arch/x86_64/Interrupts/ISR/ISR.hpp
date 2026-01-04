#pragma once
#include <stdint.h>
#include <globals.hpp>

// Interrupt frame pushed by CPU and our stub
struct InterruptFrame {
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
};

// Extended frame with registers we push
struct Registers {
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
    uint64_t int_no, err_code;
    struct InterruptFrame frame;
};

// Common interrupt handler in C
EXTERNC void isr_handler(struct Registers *regs);
EXTERNC void irq_handler(struct Registers *regs);
EXTERNC void isr0(void);
EXTERNC void isr14(void);
EXTERNC void isr13(void);


EXTERNC void irq0();
EXTERNC void irq1();
EXTERNC void irq2();
EXTERNC void irq3();
EXTERNC void irq4();
EXTERNC void irq5();
EXTERNC void irq6();
EXTERNC void irq7();
EXTERNC void irq8();
EXTERNC void irq9();
EXTERNC void irq10();
EXTERNC void irq11();
EXTERNC void irq12();
EXTERNC void irq13();
EXTERNC void irq14();
EXTERNC void irq15();

