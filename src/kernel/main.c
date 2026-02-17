#include <stdint.h>
#include <hal/hal.h>
#include <arch/i686/io.h>
#include <arch/i686/interrupts/irq.h>
#include <arch/generic/cpu.h>
#include <arch/vga_text.h>

#include "stdio.h"
#include "memory.h"
#include "debug.h"

extern void _init();

void timer(Registers* regs){
}


void start(uint16_t bootDrive){
    // call global constructors
    _init();

    VGA_clrscr();
    printf("Loaded Kernel !!!\r\n");

    HAL_Inizialize();

    printf("Initialized HAL !!!\r\n");

    i686_IRQ_RegisterHandler(0, timer);

    print_cpu_info();

    end:
        for(;;);

}
