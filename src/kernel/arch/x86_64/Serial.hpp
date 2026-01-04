#pragma once

#include <stdint.h>


static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline uint16_t inw(uint16_t port) {
    uint16_t result;
    __asm__ volatile ("inw %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

static inline void outw(uint16_t port, uint16_t value) {
    __asm__ volatile (
        "outw %0, %1" 
        :                 // No output operands
        : "a"(value),     // %0: value goes into EAX/AX register
          "Nd"(port)      // %1: port goes into EDX/DX or is a constant
    );
}

static inline void outl(uint16_t port, uint32_t value) {
    __asm__ volatile ("outl %0, %1" : : "a"(value), "Nd"(port));
}

#define SERIAL_COM1 0x3F8

#ifndef __SERIAL_H
#define __SERIAL_H

    class Serial{
        public:
            /* Initialize serial port */
            inline void Init(uint16_t port) {
                port_addr = port;

                outb(port + 1, 0x00);    // Disable interrupts
                outb(port + 3, 0x80);    // Enable DLAB
                outb(port + 0, 0x01);    // Divisor low byte (115200 baud)
                outb(port + 1, 0x00);    // Divisor high byte
                outb(port + 3, 0x03);    // 8 bits, no parity, one stop bit
                outb(port + 2, 0xC7);    // Enable FIFO, clear, 14-byte threshold
                outb(port + 4, 0x0B);    // IRQs enabled, RTS/DSR set
            }

            /* Write a single character */
            inline void WriteChar(char c) {
                while ((inb(port_addr + 5) & 0x20) == 0);
                outb(port_addr, (uint8_t)c);
            }

            /* Write a string */
            inline void Print(const char* str) {
                while (*str) {
                    WriteChar(*str++);
                }
            }


        uint16_t port_addr;
    } ;


#endif