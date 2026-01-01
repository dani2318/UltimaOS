#include <arch/x86_64/IDT/IDT.hpp>


EXTERNC void isr0();   // defined in assembly

void IDT::Init(void) {
    SetIDTGate(0, (uint64_t)isr0);  // divide by zero

    IDTPtr idtp;

    idtp.limit = sizeof(idt) - 1;
    idtp.base  = (uint64_t)&idt;

    lidt(&idtp);
    g_screenwriter->Print("IDT OK!\n");
}

void IDT::SetIDTGate(int n, uint64_t handler) {
    idt[n].offset_low  = handler & 0xFFFF;
    idt[n].selector    = 0x08;        // kernel code segment
    idt[n].ist         = 0;
    idt[n].type_attr   = 0x8E;        // interrupt gate
    idt[n].offset_mid  = (handler >> 16) & 0xFFFF;
    idt[n].offset_high = (handler >> 32) & 0xFFFFFFFF;
    idt[n].zero        = 0;
}