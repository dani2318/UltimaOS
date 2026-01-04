#pragma once
#include <libs/core/UEFI.h>
#include <stdint.h>
#include <stdbool.h>
#include <arch/x86_64/Interrupts/PIT/PIT.hpp>
#include <globals.hpp>

struct  PSF1_Header{
    unsigned char magic[2];
    unsigned char mode;
    unsigned char charsize;
} ;


#ifndef __SCREEN_WRITER_H
#define __SCREEN_WRITER_H



class ScreenWriter {
    public:
        ScreenWriter(Framebuffer* fb, PSF1_Header* font) 
            : target_fb(fb), cursor_x(0), cursor_y(0), font(font), color(0xFFFFFFFF) {

        }

        inline void PutPixel(uint32_t x, uint32_t y, uint32_t pixel_color) {
            if (x >= target_fb->Width || y >= target_fb->Height) return;
            
            uint32_t* pixel_ptr = (uint32_t*)target_fb->BaseAddress;
            pixel_ptr[x + (y * target_fb->PixelsPerScanLine)] = pixel_color;
        }

        inline void Clear(uint32_t clear_color) {
            uint64_t pixels_to_clear = (uint64_t)target_fb->Height * target_fb->PixelsPerScanLine;
            uint32_t* pixel_ptr = (uint32_t*)target_fb->BaseAddress;
            
            for (uint64_t i = 0; i < pixels_to_clear; i++) {
                pixel_ptr[i] = clear_color;
            }
        }
        inline void PrintChar(char c, uint32_t xOff, uint32_t yOff) {
            unsigned char* glyph = (unsigned char*)font + sizeof(PSF1_Header) + ((unsigned char)c * font->charsize);
            for (uint32_t y = 0; y < font->charsize; y++) {
                for (uint32_t x = 0; x < 8; x++) {
                    if ((glyph[y] & (0x80 >> x)) != 0) {
                        PutPixel(xOff + x, yOff + y, color);
                    } else {
                        // IMPORTANT: Clear the background pixel!
                        PutPixel(xOff + x, yOff + y, NORMAL_CLEAR_COLOR); // Black background
                    }
                }
            }
        }
        
        inline void Print(const char* str) {

            while (*str != '\0') {
                if (*str == '\n') {
                    cursor_y += font->charsize;
                    cursor_x = 0;
                } 
                else if (*str == '\r') {
                    cursor_x = 0;
                }
                else if (*str == '\t') {
                    cursor_x += 32;
                    if (cursor_x >= target_fb->Width) {
                        cursor_x = 0;
                        cursor_y += font->charsize;
                    }
                }
                else {
                    PrintChar(*str, cursor_x, cursor_y);
                    cursor_x += 8;
                    
                    if (cursor_x + 8 > target_fb->Width) {
                        cursor_x = 0;
                        cursor_y += font->charsize;
                    }
                    
                    if (cursor_y + font->charsize > target_fb->Height) {
                        cursor_y = 0;
                    }
                }
                str++;
            }
        }

        void PrintHex(uint64_t val) {
            char hex[] = "0123456789ABCDEF";
            char buf[17];
            buf[16] = '\0';

            for (int i = 15; i >= 0; i--) {
                buf[i] = hex[val & 0xF];
                val >>= 4;
            }

            Print(buf);
        }


        void PrintUint(uint64_t value) {
            char buf[21];
            int i = 20;
            buf[i] = '\0';

            if (value == 0) {
                buf[--i] = '0';
            } else {
                while (value > 0) {
                    buf[--i] = '0' + (value % 10);
                    value /= 10;
                }
            }

            Print(&buf[i]);
        }

        void SetColor(uint32_t value){
            this->color = value;
        }

        void SetCursor(uint32_t x, uint32_t y){
            cursor_x = x;
            cursor_y = y;
        }
        void PrintDecimalPadded(uint64_t value, int width) {
            char buffer[20];
            int i = 0;
            
            // Convert to string (reversed)
            do {
                buffer[i++] = '0' + (value % 10);
                value /= 10;
            } while (value > 0 || i < width);
            
            // Print in correct order
            while (i > 0) {
                i--;
                char str[2] = {buffer[i], '\0'};
                Print(str);
            }
        }

        void Uptime() {
            uint64_t total_seconds = Timer::GetUptimeSeconds();
            uint64_t hours = total_seconds / 3600;
            uint64_t minutes = (total_seconds % 3600) / 60;
            uint64_t seconds = total_seconds % 60;
            
            Print("Uptime: ");
            
            PrintDecimalPadded(hours, 2);
            Print(":");
            PrintDecimalPadded(minutes, 2);
            Print(":");
            PrintDecimalPadded(seconds, 2);
        }
        void Backspace() {
            if (cursor_x >= 8) {
                cursor_x -= 8;
                PrintChar(' ', cursor_x, cursor_y);
            }
        }
    private:
        Framebuffer* target_fb;
        uint32_t cursor_x;
        uint32_t cursor_y;
        uint32_t color;
        PSF1_Header* font;
        const uint32_t char_width = 8;
        const uint32_t char_height = 16;
};




#endif // __SCREEN_WRITER_H