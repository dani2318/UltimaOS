#include <arch/x86_64/Interrupts/ISR/ISR.hpp>
#include <arch/x86_64/Interrupts/PIC/PIC.hpp>
#include <arch/x86_64/Interrupts/PIT/PIT.hpp>

#include <arch/x86_64/Drivers/Keyboard.hpp>
#include <arch/x86_64/Multitasking/Scheduler.hpp>
#include <arch/x86_64/ScreenWriter.hpp> // <--- ADD THIS LINE
#include <globals.hpp>


// uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
// uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
// uint64_t int_no, err_code;
// struct InterruptFrame frame;

void print_registers(struct Registers *regs){
    
    g_screenwriter->Print("\r\n");
    g_screenwriter->Print("r15: ");
    g_screenwriter->PrintHex(regs->r15);

    g_screenwriter->Print("\trbp: ");
    g_screenwriter->PrintHex(regs->rbp);

    g_screenwriter->Print("\terr_code: ");
    g_screenwriter->PrintHex(regs->err_code);

    g_screenwriter->Print("\r\n");

    g_screenwriter->Print("r14: ");
    g_screenwriter->PrintHex(regs->r14);
    
    g_screenwriter->Print("\trdi: ");
    g_screenwriter->PrintHex(regs->rdi);
    
    g_screenwriter->Print("\tint_no: ");
    g_screenwriter->PrintHex(regs->int_no);

    g_screenwriter->Print("\r\n");

    g_screenwriter->Print("r13: ");
    g_screenwriter->PrintHex(regs->r13);
    
    g_screenwriter->Print("\trsi: ");
    g_screenwriter->PrintHex(regs->rsi);
    g_screenwriter->Print("\r\n");

    g_screenwriter->Print("r12: ");
    g_screenwriter->PrintHex(regs->r12);

    g_screenwriter->Print("\trdx: ");
    g_screenwriter->PrintHex(regs->rdx);
    g_screenwriter->Print("\r\n");

    g_screenwriter->Print("r11: ");
    g_screenwriter->PrintHex(regs->r11);

    g_screenwriter->Print("\trcx: ");
    g_screenwriter->PrintHex(regs->rcx);
    g_screenwriter->Print("\r\n");

    g_screenwriter->Print("r10: ");
    g_screenwriter->PrintHex(regs->r10);

    g_screenwriter->Print("\trbx: ");
    g_screenwriter->PrintHex(regs->rbx);
    g_screenwriter->Print("\r\n");

    g_screenwriter->Print("r9: ");
    g_screenwriter->PrintHex(regs->r9);

    g_screenwriter->Print("\t rax: ");
    g_screenwriter->PrintHex(regs->rax);
    g_screenwriter->Print("\r\n");

    g_screenwriter->Print("r8: ");
    g_screenwriter->PrintHex(regs->r8);

    g_screenwriter->Print("\r\n");

}

void isr_handler(struct Registers *regs) {
    g_screenwriter->Clear(EXCEPTION_CLEAR_COLOR);

    // Handle the interrupt based on interrupt number
    switch(regs->int_no) {
        case 0:
            g_screenwriter->Print("Exception: Division by Zero.\n\r");
            break;
        case 13:
            g_screenwriter->Print("Exception: General protection fault.\n\r");
            break;
        case 14:
            g_screenwriter->Print("Exception: Page fault.\n\r");
            break;
        default:
            g_screenwriter->Print("Exception: Unknown interrupt.\r\n");
            break;
    }

    print_registers(regs);

    while(1) { __asm__ volatile("hlt"); }  



}

extern "C" void irq_handler(struct Registers *regs) {
    uint8_t irq = regs->int_no - 32;

    if (irq == 0) {
        Timer::OnInterrupt();
        PIC::SendEOI(irq);
        Scheduler::Schedule();
        return; 
    }

    switch(irq) {
        case 1:
            Keyboard::OnInterrupt();
            break;
        default:
            g_screenwriter->Print("IRQ ");
            g_screenwriter->PrintHex(irq);
            g_screenwriter->Print("\n\r");
            break;
    }

    PIC::SendEOI(irq);
}