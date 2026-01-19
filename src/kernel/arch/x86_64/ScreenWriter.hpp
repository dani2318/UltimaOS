#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <arch/x86_64/Interrupts/PIT/PIT.hpp>
#include <globals.hpp>
#include <arch/x86_64/Serial.hpp>

struct PsF1Header
{
    unsigned char magic[2];
    unsigned char mode;
    unsigned char charsize;
};

#define SERIAL_COM1 0x3F8
#ifndef __SCREEN_WRITER_H
#define SCREEN_WRITER_H

class ScreenWriter
{
public:
    ScreenWriter(Framebuffer *fb, PsF1Header *font)
        : m_target_fb(fb),  m_font(font)
    {
        outb(SERIAL_COM1 + 1, 0x00); // Disable interrupts
        outb(SERIAL_COM1 + 3, 0x80); // Enable DLAB
        outb(SERIAL_COM1 + 0, 0x01); // Divisor low byte (115200 baud)
        outb(SERIAL_COM1 + 1, 0x00); // Divisor high byte
        outb(SERIAL_COM1 + 3, 0x03); // 8 bits, no parity, one stop bit
        outb(SERIAL_COM1 + 2, 0xC7); // Enable FIFO, clear, 14-byte threshold
        outb(SERIAL_COM1 + 4, 0x0B); // IRQs enabled, RTS/DSR set
    }

    void putPixel(uint32_t x, uint32_t y, uint32_t pixelColor)
    {
        if (x >= m_target_fb->Width || y >= m_target_fb->Height)
            return;

        uint32_t *pixelPtr = (uint32_t *)m_target_fb->BaseAddress;
        pixelPtr[x + (y * m_target_fb->PixelsPerScanLine)] = pixelColor;
    }

    void clear(uint32_t clearColor)
    {
        uint64_t const pixelsToClear = (uint64_t)m_target_fb->Height * m_target_fb->PixelsPerScanLine;
        uint32_t *pixelPtr = (uint32_t *)m_target_fb->BaseAddress;

        for (uint64_t i = 0; i < pixelsToClear; i++)
        {
            pixelPtr[i] = clearColor;
        }
    }

    void printChar(char c, uint32_t xOff, uint32_t yOff, bool useSerial = false)
    {
        if (!useSerial)
        {
            unsigned char *glyph = (unsigned char *)m_font + sizeof(PsF1Header) + ((unsigned char)c * m_font->charsize);
            for (uint32_t y = 0; y < m_font->charsize; y++)
            {
                for (uint32_t x = 0; x < 8; x++)
                {
                    if ((glyph[y] & (0x80 >> x)) != 0)
                    {
                        putPixel(xOff + x, yOff + y, color);
                    }
                    else
                    {
                        putPixel(xOff + x, yOff + y,  0x0000000); // Black background
                    }
                }
            }
        }
        else
        {
            while ((inb(SERIAL_COM1 + 5) & 0x20) == 0)
            {
            };
            outb(SERIAL_COM1, (uint8_t)c);
        }
    }

    void print(const char *str, bool useSerial = false)
    {
        if (!useSerial)
        {
            while (*str != '\0')
            {
                if (*str == '\n')
                {
                    cursor_y += m_font->charsize;
                    cursor_x = 0;
                }
                else if (*str == '\r')
                {
                    cursor_x = 0;
                }
                else if (*str == '\t')
                {
                    cursor_x += 32;
                    if (cursor_x >= m_target_fb->Width)
                    {
                        cursor_x = 0;
                        cursor_y += m_font->charsize;
                    }
                }
                else
                {
                    printChar(*str, cursor_x, cursor_y);
                    cursor_x += 8;

                    if (cursor_x + 8 > m_target_fb->Width)
                    {
                        cursor_x = 0;
                        cursor_y += m_font->charsize;
                    }

                    if (cursor_y + m_font->charsize > m_target_fb->Height)
                    {
                        cursor_y = 0;
                    }
                }
                str++;
            }
        }
        else
        {
            while (*str != 0)
            {
                printChar(*str++,cursor_x, cursor_y,true);
            }
        }
    }

    void printHex(uint64_t val, bool useSerial) {
        char buf[19]; // 0x + 16 digits + null
        buf[0] = '0'; buf[1] = 'x';
        const char* hex = "0123456789ABCDEF";
        for(int i = 17; i >= 2; i--) {
            buf[i] = hex[val & 0xF];
            val >>= 4;
        }
        buf[18] = 0;
        this->print(buf, useSerial);
    }

    void printUint(uint64_t value)
    {
        char buf[21];
        int i = 20;
        buf[i] = '\0';

        if (value == 0)
        {
            buf[--i] = '0';
        }
        else
        {
            while (value > 0)
            {
                buf[--i] = '0' + (value % 10);
                value /= 10;
            }
        }

        print(&buf[i],false);
    }

    void setColor(uint32_t value)
    {
        this->color = value;
    }

    void setCursor(uint32_t x, uint32_t y)
    {
        cursor_x = x;
        cursor_y = y;
    }
    void printDecimalPadded(uint64_t value, int width)
    {
        char buffer[20];
        int i = 0;

        // Convert to string (reversed)
        do
        {
            buffer[i++] = '0' + (value % 10);
            value /= 10;
        } while (value > 0 || i < width);

        // Print in correct order
        while (i > 0)
        {
            i--;
            char str[2] = {buffer[i], '\0'};
            print(str, false);
        }
    }

    void uptime()
    {
        uint64_t const totalSeconds = Timer::getUptimeSeconds();
        uint64_t const hours = totalSeconds / 3600;
        uint64_t const minutes = (totalSeconds % 3600) / 60;
        uint64_t const seconds = totalSeconds % 60;

        print("Uptime: ", false);

        printDecimalPadded(hours, 2);
        print(":", false);
        printDecimalPadded(minutes, 2);
        print(":", false);
        printDecimalPadded(seconds, 2);
    }
    void backspace()
    {
        if (cursor_x >= 8)
        {
            cursor_x -= 8;
            printChar(' ', cursor_x, cursor_y);
        }
    }

private:
    Framebuffer *m_target_fb;
    uint32_t cursor_x{0};
    uint32_t cursor_y{0};
    uint32_t color{0xFFFFFFFF};
    PsF1Header *m_font;
    const uint32_t CHAR_WIDTH = 8;
    const uint32_t CHAR_HEIGHT = 16;
};

#endif // __SCREEN_WRITER_H
