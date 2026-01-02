#pragma once

#include <arch/x86_64/IDT/isr_h.hpp>
#include <arch/x86_64/PIC/i8259.hpp>
#include <arch/x86_64/PIC/pic.hpp>
#include <globals.hpp>


typedef void (*IRQHandler) (ISRFrame* regs);

class IRQ{
    public:
        void Initialize();
        void RegisterHandler(int irq, IRQHandler handler);
};
