#include <stdint.h>
#include <stdbool.h>

#include <arch/x86_64/PIC/i8259.hpp>
#include <globals.hpp>

#define PIC1_COMMAND_PORT          0x20
#define PIC1_DATA_PORT             0x21
#define PIC2_COMMAND_PORT          0xA0
#define PIC2_DATA_PORT             0xA1

enum {
    PIC_ICW1_ICW4                = 0X01,
    PIC_ICW1_SINGLE              = 0X02,
    PIC_ICW1_INTERVAL4           = 0X04,
    PIC_ICW1_LEVEL               = 0X08,
    PIC_ICW1_INITIALIZE          = 0X10,
} PIC_ICW1;

enum {
    PIC_ICW4_8086                = 0X01,
    PIC_ICW4_AUTO_EOI            = 0X02,
    PIC_ICW4_BUFFER_MASTER       = 0X04,
    PIC_ICW4_BUFFER_SLAVE        = 0X08,
    PIC_ICW4_BUFFERRED           = 0X10,
} PIC_ICW2;

enum {
    PIC_CMD_END_OF_INTERRUPT     = 0X20,
    PIC_CMD_READ_IRR             = 0X0A,
    PIC_CMD_READ_ISR             = 0X0B,
} PIC_CMD;

#define PIC_RESET 0
#define PIC_DISABLE 0xFFFF

static uint16_t g_picmask = 0xFF;
static bool g_AutoEOI = false;

void i8259_SetMask(uint16_t newMask){
    g_picmask = newMask;
    outb(PIC1_DATA_PORT, g_picmask & 0xFF);
    outb(0x80, 0);
    outb(PIC2_DATA_PORT, g_picmask >> 8);
    outb(0x80, 0);
}

uint16_t i8259_GetMask(){
    return inb(PIC1_DATA_PORT) | (inb(PIC2_DATA_PORT) << 8);
}

uint16_t i8259_ReadIRQRequestRegister(){
    outb(PIC1_COMMAND_PORT, PIC_CMD_READ_IRR);
    outb(PIC2_COMMAND_PORT, PIC_CMD_READ_IRR);
    return inb(PIC1_COMMAND_PORT) | (inb(PIC2_COMMAND_PORT) << 8);
}

uint16_t i8259_ReadInServiceRegister(){
    outb(PIC1_COMMAND_PORT, PIC_CMD_READ_ISR);
    outb(PIC2_COMMAND_PORT, PIC_CMD_READ_ISR);
    return inb(PIC1_COMMAND_PORT) | (inb(PIC2_COMMAND_PORT) << 8);
}

void i8259_SendEOI(int irq){
    if (irq >= 8){
        outb(PIC2_COMMAND_PORT, PIC_CMD_END_OF_INTERRUPT);
    }
    outb(PIC1_COMMAND_PORT, PIC_CMD_END_OF_INTERRUPT);

}

void i8259_Configure(uint8_t offsetPic1, uint8_t offsetPic2, bool autoEOI){
    i8259_SetMask(0xFFFF);

    outb(PIC1_COMMAND_PORT, PIC_ICW1_ICW4 | PIC_ICW1_INITIALIZE);
    outb(0x80, 0);
    outb(PIC2_COMMAND_PORT, PIC_ICW1_ICW4 | PIC_ICW1_INITIALIZE);
    outb(0x80, 0);

    outb(PIC1_DATA_PORT, offsetPic1);
    outb(0x80, 0);
    outb(PIC2_DATA_PORT, offsetPic2);
    outb(0x80, 0);

    outb(PIC1_DATA_PORT, 0x4);
    outb(0x80, 0);
    outb(PIC2_DATA_PORT, 0x2);
    outb(0x80, 0);

    uint8_t icw4 = PIC_ICW4_8086;

    if(autoEOI){
        g_AutoEOI = autoEOI;
        icw4 |= PIC_ICW4_AUTO_EOI;
    }

    outb(PIC1_DATA_PORT, icw4);
    outb(0x80, 0);
    outb(PIC2_DATA_PORT, icw4);
    outb(0x80, 0);

    i8259_SetMask(0xFFFF);

}

void i8259_Disable(){
    i8259_SetMask(0xFFFF);
}

void i8259_Mask(int irq){
    i8259_SetMask(g_picmask | 1 << irq);
}

void i8259_Unmask(int irq){
    i8259_SetMask(g_picmask & ~(1 << irq));
}

bool i8259_Probe(const PICDriver* self){
    (void)self;
    i8259_Disable();
    i8259_SetMask(0x1337);
    return i8259_GetMask() == 0x1337;
}


static const PICDriver* g_PicDriver = new PICDriver(
    &i8259_Probe,
    &i8259_Configure,
    &i8259_Disable,
    &i8259_SendEOI,
    &i8259_Mask,
    &i8259_Unmask,
    "i8259_PIC"
);

const PICDriver* i8259_GetDriver(){
    return g_PicDriver;
}