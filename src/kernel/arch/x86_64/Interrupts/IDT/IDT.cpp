#include <arch/x86_64/Interrupts/IDT/IDT.hpp>
#include <memory/memory.hpp>

#include <arch/x86_64/Interrupts/ISR/ISR.hpp>
#include <arch/x86_64/Interrupts/PIC/PIC.hpp>

#define FLAG_SET(x, flag)   x |= flag
#define FLAG_UNSET(x, flag) x &= ~flag

EXTERNC void Timer_Handler();

void IDT::setIDTEntry(int index, uint64_t handler, uint16_t selector, uint8_t typeattr){
    idt[index].offset_low = handler & 0xFFFF;
    idt[index].selector = selector;
    idt[index].ist = 0;
    idt[index].typeattr = typeattr;
    idt[index].offset_mid = (handler >> 16) & 0xFFFF;   
    idt[index].offset_high = (handler >> 32) & 0xFFFFFFFF;   
    idt[index].reserved = 0;
}

void IDT::Initialize(){
    memset(idt, 0, sizeof(idt));

    setIDTEntry(0, (uint64_t)isr0, 0x08, 0x8E);    // Divide by zero
    setIDTEntry(13, (uint64_t)isr13, 0x08, 0x8E);  // General protection
    setIDTEntry(14, (uint64_t)isr14, 0x08, 0x8E);  // General protection

    // IRQ Setup
    setIDTEntry(32, (uint64_t)Timer_Handler, 0x08, 0x8E);   // Timer
    setIDTEntry(33, (uint64_t)irq1, 0x08, 0x8E);   // Keyboard
    setIDTEntry(34, (uint64_t)irq2, 0x08, 0x8E);   // Cascade
    setIDTEntry(35, (uint64_t)irq3, 0x08, 0x8E);   // COM2
    setIDTEntry(36, (uint64_t)irq4, 0x08, 0x8E);   // COM1
    setIDTEntry(37, (uint64_t)irq5, 0x08, 0x8E);   // LPT2
    setIDTEntry(38, (uint64_t)irq6, 0x08, 0x8E);   // Floppy
    setIDTEntry(39, (uint64_t)irq7, 0x08, 0x8E);   // LPT1
    setIDTEntry(40, (uint64_t)irq8, 0x08, 0x8E);   // RTC (was irq10, should be irq8)
    setIDTEntry(41, (uint64_t)irq9, 0x08, 0x8E);   // (was irq11, should be irq9)
    setIDTEntry(42, (uint64_t)irq10, 0x08, 0x8E);  // (was irq12, should be irq10)
    setIDTEntry(43, (uint64_t)irq11, 0x08, 0x8E);  // (was irq13, should be irq11)
    setIDTEntry(44, (uint64_t)irq12, 0x08, 0x8E);  // PS/2 Mouse (was irq14, should be irq12)
    setIDTEntry(45, (uint64_t)irq13, 0x08, 0x8E);  // FPU (was irq15, should be irq13)
    setIDTEntry(46, (uint64_t)irq14, 0x08, 0x8E);  // Primary ATA (was irq2, should be irq14)
    setIDTEntry(47, (uint64_t)irq15, 0x08, 0x8E);  // Secondary ATA (was irq2, should be irq15)

    
    IDTLoad(&idtr);
    PIC::Initialize();
    PIC::SetMask(0);
    PIC::ClearMask(1);

}
