#include <arch/x86_64/IDT/isr_h.hpp>
#include <globals.hpp>

ISRHandler g_ISRHandler[256];

extern "C" {

static const char* const g_Exceptions[] = {
    "Divide by zero error",
    "Debug",
    "Non-maskable Interrupt",
    "Breakpoint",
    "Overflow",
    "Bound Range Exceeded",
    "Invalid Opcode",
    "Device Not Available",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Invalid TSS",
    "Segment Not Present",
    "Stack-Segment Fault",
    "General Protection Fault",
    "Page Fault",
    "",
    "x87 Floating-Point Exception",
    "Alignment Check",
    "Machine Check",
    "SIMD Floating-Point Exception",
    "Virtualization Exception",
    "Control Protection Exception",
    "",
    "",
    "",
    "",
    "",
    "",
    "Hypervisor Injection Exception",
    "VMM Communication Exception",
    "Security Exception",
    ""
};



void isr_handler(ISRFrame* frame) {
    g_screenwriter->Clear(0xFF0000FF);
    
    // Print exception name
    if (frame->int_no < 32 && g_Exceptions[frame->int_no][0] != '\0') {
        g_screenwriter->Print(g_Exceptions[frame->int_no]);
    } else {
        g_screenwriter->Print("Unknown Exception");
    }
    g_screenwriter->Print("\n\n");
    
    // Print exception number
    g_screenwriter->Print("Interrupt: 0x");
    g_screenwriter->PrintHex(frame->int_no);
    g_screenwriter->Print("\n");
    
    // Print error code
    g_screenwriter->Print("Error Code: 0x");
    g_screenwriter->PrintHex(frame->err_code);
    g_screenwriter->Print("\n");
    
    // Print instruction pointer
    g_screenwriter->Print("RIP: 0x");
    g_screenwriter->PrintHex(frame->rip);
    g_screenwriter->Print("\n");
    
    // Print flags
    g_screenwriter->Print("RFLAGS: 0x");
    g_screenwriter->PrintHex(frame->rflags);
    g_screenwriter->Print("\n");
    
    // Halt forever
    for (;;) {
        asm volatile ("hlt");
    }
}

}
