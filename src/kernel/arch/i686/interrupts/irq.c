#include <arch/i686/interrupts/irq.h>
#include <arch/i686/io.h>
#include <stdio.h>
#include <util/arrays.h>
#include <stddef.h>

#define PIC_REMAP_OFFSET 0x20

IRQHandler g_IRQHandlers[16];
static const PICDriver* g_Driver = NULL;

void i686_IRQ_Handler(Registers* regs){
    int irq = regs->interrupt - PIC_REMAP_OFFSET;
    if(g_IRQHandlers[irq] != NULL){
        g_IRQHandlers[irq](regs);
    }else{
        printf("Unhandled IRQ %d ...\n", irq);
    }
    g_Driver->SendEOI(irq);
}

void i686_IRQ_Initialize(){
    
    const PICDriver* drivers[] = {
        i8259_GetDriver(),
    };

    for(int i = 0; i < SIZE(drivers); i++){
        if(drivers[i]->Probe()){
            g_Driver = drivers[i];
            break;
        }
    }

    if(g_Driver == NULL){
        printf("WARNING: No PIC!\r\n");
    }

    printf("Found %s\n\r", g_Driver->Name);
    g_Driver->Initialize(PIC_REMAP_OFFSET, PIC_REMAP_OFFSET + 8,false);

    for(int i = 0; i < 16; i++){
        i686_ISR_RegisterHandler(PIC_REMAP_OFFSET + i, i686_IRQ_Handler);
    }
    i686_sti();
}

void i686_IRQ_RegisterHandler(int irq, IRQHandler handler){
    g_IRQHandlers[irq] = handler;
}