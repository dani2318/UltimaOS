#include <arch/x86_64/Interrupts/ISR/ISR.hpp>
#include <arch/x86_64/Interrupts/PIC/PIC.hpp>
#include <arch/x86_64/Interrupts/PIT/PIT.hpp>

#include <Drivers/Input/Keyboard.hpp>
#include <arch/x86_64/Multitasking/Scheduler.hpp>
#include <arch/x86_64/ScreenWriter.hpp> // <--- ADD THIS LINE
#include <globals.hpp>

void print_registers(struct Registers *regs)
{

    gScreenwriter->print("\r\n", false);
    gScreenwriter->print("r15: ", false);
    gScreenwriter->printHex(regs->r15, false);

    gScreenwriter->print("\trbp: ", false);
    gScreenwriter->printHex(regs->rbp, false);

    gScreenwriter->print("\terr_code: ", false);
    gScreenwriter->printHex(regs->err_code, false);

    gScreenwriter->print("\r\n", false);

    gScreenwriter->print("r14: ", false);
    gScreenwriter->printHex(regs->r14, false);

    gScreenwriter->print("\trdi: ", false);
    gScreenwriter->printHex(regs->rdi, false);

    gScreenwriter->print("\tint_no:", false);
    gScreenwriter->printHex(regs->int_no, false);

    gScreenwriter->print("\r\n", false);

    gScreenwriter->print("r13: ", false);
    gScreenwriter->printHex(regs->r13, false);

    gScreenwriter->print("\trsi:", false);
    gScreenwriter->printHex(regs->rsi, false);
    gScreenwriter->print("\r\n", false);

    gScreenwriter->print("r12: ", false);
    gScreenwriter->printHex(regs->r12, false);

    gScreenwriter->print("\trdx: ", false);
    gScreenwriter->printHex(regs->rdx, false);
    gScreenwriter->print("\r\n", false);

    gScreenwriter->print("r11: ", false);
    gScreenwriter->printHex(regs->r11, false);

    gScreenwriter->print("\trcx: ", false);
    gScreenwriter->printHex(regs->rcx, false);
    gScreenwriter->print("\r\n", false);

    gScreenwriter->print("r10: ", false);
    gScreenwriter->printHex(regs->r10, false);

    gScreenwriter->print("\trbx: ", false);
    gScreenwriter->printHex(regs->rbx, false);
    gScreenwriter->print("\r\n", false);

    gScreenwriter->print("r9: ", false);
    gScreenwriter->printHex(regs->r9, false);

    gScreenwriter->print("\t rax: ", false);
    gScreenwriter->printHex(regs->rax, false);
    gScreenwriter->print("\r\n", false);

    gScreenwriter->print("r8: ", false);
    gScreenwriter->printHex(regs->r8, false);

    gScreenwriter->print("\r\n", false);
}


void isr_handler(struct Registers *regs)
{
    gScreenwriter->clear(EXCEPTION_CLEAR_COLOR);

    // Handle the interrupt based on interrupt number
    switch (regs->int_no)
    {
    case 0:
        gScreenwriter->print("Exception: Division by Zero.\n\r", false);
        break;
    case 13:
        gScreenwriter->print("Exception: General protection fault.\n\r", false);
        break;
    case 14:
        gScreenwriter->print("Exception: Page fault.\n\r", false);
        break;
    default:
        gScreenwriter->print("Exception: Unknown interrupt.\r\n", false);
        break;
    }

    print_registers(regs);

    while (1)
    {
        __asm__ volatile("hlt");
    }
}


extern "C" void irq_handler(struct Registers *regs)
{
    uint8_t irq = regs->int_no - 32;

    if (irq == 0)
    {
        Timer::onInterrupt();
        PIC::SendEOI(irq);
        gScheduler->Schedule();
        return;
    }

    switch (irq)
    {
    case 1:
        Keyboard::OnInterrupt();
        break;
    default:
        gScreenwriter->print("IRQ ", false);
        gScreenwriter->printHex(irq, false);
        gScreenwriter->print("\n\r", false);
        break;
    }

    PIC::SendEOI(irq);
}
