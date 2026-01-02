#pragma once
#include <stdint.h>
#include <globals.hpp>

typedef struct  {
    // Registers we pushed (in order on stack, low to high address)
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
    
    // Our pushed values
    uint64_t int_no;
    uint64_t err_code;
    
    // CPU automatically pushes these (in this order)
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
} __attribute__((packed)) ISRFrame;

typedef void (*ISRHandler) (ISRFrame* regs);

