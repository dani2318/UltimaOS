#pragma once
#include <stdint.h>

// Write a byte to an I/O port
extern "C" inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );
}

extern "C" inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ( "inb %1, %0" : "=a"(ret) : "Nd"(port) );
    return ret;
}

class Serial {
public:
    enum Port : uint16_t { COM1 = 0x3f8 };

    Serial(Port port) : port_addr(port) {
        outb(port + 1, 0x00);    // Disable interrupts
        outb(port + 3, 0x80);    // Enable DLAB (set baud rate divisor)
        outb(port + 0, 0x01);    // Set divisor to 1 (115200 baud) - Fast and standard for QEMU
        outb(port + 1, 0x00);    //                  - high byte
        outb(port + 3, 0x03);    // 8 bits, no parity, one stop bit
        outb(port + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
        outb(port + 4, 0x0B);    // IRQs enabled, RTS/DSR set
    }

    void write(char c) {
        while ((inb(port_addr + 5) & 0x20) == 0);
        outb(port_addr, c);
    }

    void print(const char* str) {
        while (*str) write(*str++);
    }

    void print_char(char c) {
        while ((inb(0x3f8 + 5) & 0x20) == 0);
        write(c);
    }

private:
    uint16_t port_addr;
};
