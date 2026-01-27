// PIC.cpp
#include "PIC.hpp"
#include <arch/x86_64/Serial.hpp>  // For outb/inb functions

#define PIC1_COMMAND 0x20
#define PIC1_DATA    0x21
#define PIC2_COMMAND 0xA0
#define PIC2_DATA    0xA1

#define ICW1_INIT    0x10
#define ICW1_ICW4    0x01
#define ICW4_8086    0x01

void PIC::Initialize() {
    // Save masks
    uint8_t mask1 = inb(PIC1_DATA);
    uint8_t mask2 = inb(PIC2_DATA);
    
    // Start initialization sequence
    outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    
    // Set vector offsets
    outb(PIC1_DATA, 32);  // Master PIC offset to 32 (IRQ 0-7 -> INT 32-39)
    outb(PIC2_DATA, 40);  // Slave PIC offset to 40 (IRQ 8-15 -> INT 40-47)
    
    outb(PIC1_DATA, 0x04);
    outb(PIC2_DATA, 0x02);
    
    // Set mode to 8086
    outb(PIC1_DATA, ICW4_8086);
    outb(PIC2_DATA, ICW4_8086);
    
    // Restore masks
    outb(PIC1_DATA, mask1);
    outb(PIC2_DATA, mask2);
}

void PIC::SendEOI(uint8_t irq) {
    if (irq >= 8) {
        outb(PIC2_COMMAND, 0x20);  // Send EOI to slave
    }
    outb(PIC1_COMMAND, 0x20);  // Send EOI to master
}

void PIC::SetMask(uint8_t irq) {
    uint16_t port;
    if (irq < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq -= 8;
    }
    uint8_t value = inb(port) | (1 << irq);
    outb(port, value);
}

void PIC::ClearMask(uint8_t irq) {
    uint16_t port;
    if (irq < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq -= 8;
    }
    uint8_t value = inb(port) & ~(1 << irq);
    outb(port, value);
}