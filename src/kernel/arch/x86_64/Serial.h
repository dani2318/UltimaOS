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

#define SERIAL_COM1 0x3F8

#ifndef __SERIAL_H
#define __SERIAL_H
typedef struct {
    uint16_t port_addr;
} Serial;

/* Initialize serial port */
static inline void Serial_Init(Serial* serial, uint16_t port) {
    serial->port_addr = port;

    outb(port + 1, 0x00);    // Disable interrupts
    outb(port + 3, 0x80);    // Enable DLAB
    outb(port + 0, 0x01);    // Divisor low byte (115200 baud)
    outb(port + 1, 0x00);    // Divisor high byte
    outb(port + 3, 0x03);    // 8 bits, no parity, one stop bit
    outb(port + 2, 0xC7);    // Enable FIFO, clear, 14-byte threshold
    outb(port + 4, 0x0B);    // IRQs enabled, RTS/DSR set
}

/* Write a single character */
static inline void Serial_WriteChar(Serial* serial, char c) {
    while ((inb(serial->port_addr + 5) & 0x20) == 0);
    outb(serial->port_addr, (uint8_t)c);
}

/* Write a string */
static inline void Serial_Print(Serial* serial, const char* str) {
    while (*str) {
        Serial_WriteChar(serial, *str++);
    }
}

#endif