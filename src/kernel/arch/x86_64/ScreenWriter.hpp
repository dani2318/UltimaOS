#pragma once
#include <arch/x86_64/UEFI.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct  {
    unsigned char magic[2];
    unsigned char mode;
    unsigned char charsize;
} PSF1_Header;


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


    private:
        Framebuffer* target_fb;
        uint32_t cursor_x;
        uint32_t cursor_y;
        uint32_t color;
        PSF1_Header* font;
};




#endif // __SCREEN_WRITER_H