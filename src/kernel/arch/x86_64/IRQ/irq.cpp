#include <arch/x86_64/IRQ/irq.hpp>
#include <stddef.h>

#define PIC_REMAP_OFFSET 0x20
#define SIZE(array) (sizeof(array) / sizeof(array[0]))

IRQHandler g_IRQHandlers[16];
static const PICDriver* g_Driver = NULL;

void i686_IRQ_Handler(ISRFrame* regs){
    int irq = regs->int_no - PIC_REMAP_OFFSET;
    if(g_IRQHandlers[irq] != NULL){
        g_IRQHandlers[irq](regs);
    }
    g_Driver->SendEOI(irq);
}

void IRQ::Initialize(){
    
    const PICDriver* drivers[] = {
        i8259_GetDriver(),
    };

    for(int i = 0; i < SIZE(drivers); i++){
        if(drivers[i]->Probe(drivers[i])){
            g_Driver = drivers[i];
            break;
        }
    }

    if(g_Driver == NULL){
        g_screenwriter->Print("WARNING: No PIC!\r\n");
    }

    g_Driver->Initialize(PIC_REMAP_OFFSET, PIC_REMAP_OFFSET + 8,false);

    for(int i = 0; i < 16; i++){
        //i686_ISR_RegisterHandler(PIC_REMAP_OFFSET + i, i686_IRQ_Handler);
    }
    __asm__ volatile("sti");
}

void IRQ::RegisterHandler(int irq, IRQHandler handler){
    g_IRQHandlers[irq] = handler;
}