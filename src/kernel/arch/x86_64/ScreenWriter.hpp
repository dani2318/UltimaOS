#pragma once
#include <arch/x86_64/UEFI.hpp>

struct PSF1_Header {
    unsigned char magic[2];
    unsigned char mode;
    unsigned char charsize;
};

class ScreenWriter {
public:
    Framebuffer* target_fb;
    uint32_t cursor_x = 0;
    uint32_t cursor_y = 0;
    uint32_t color = 0xFFFFFFFF; // White (Alpha, R, G, B)
    PSF1_Header* font;
    ScreenWriter(Framebuffer* fb, PSF1_Header* font) : target_fb(fb), font(font) {}

    // Math: Base + (x * 4 bytes per pixel) + (y * pixels per line * 4 bytes)
    void put_pixel(uint32_t x, uint32_t y, uint32_t pixel_color) {
        if (x >= target_fb->Width || y >= target_fb->Height) return;
        
        uint32_t* pixel_ptr = (uint32_t*)target_fb->BaseAddress;
        pixel_ptr[x + (y * target_fb->PixelsPerScanLine)] = pixel_color;
    }

    void clear(uint32_t clear_color) {
        for (uint32_t y = 0; y < target_fb->Height; y++) {
            for (uint32_t x = 0; x < target_fb->Width; x++) {
                put_pixel(x, y, clear_color);
            }
        }
    }

    // This is the "Magic" that draws the bits from a font array
    void print_char(char c, uint32_t xOff, uint32_t yOff) {
        // Calculate where the glyph starts in memory
        // Skip the 4-byte header, then skip 'c' glyphs of 'charsize' bytes each
        unsigned char* glyph = (unsigned char*)font + sizeof(PSF1_Header) + (c * font->charsize);

        for (uint32_t y = 0; y < font->charsize; y++) {
            for (uint32_t x = 0; x < 8; x++) {
                // We use 0x80 (10000000) to mask the leftmost bit first
                if ((glyph[y] & (0x80 >> x)) > 0) {
                    put_pixel(xOff + x, yOff + y, color);
                }
            }
        }
    }

    void print(const char* str) {
        while (*str != '\0') {
            // Handle newlines
            if (*str == '\n') {
                cursor_y += font->charsize;
                cursor_x = 0;
            } else {
                print_char(*str, cursor_x, cursor_y);
                cursor_x += 8; // Advance by character width
                
                // Basic line wrapping
                if (cursor_x + 8 > target_fb->Width) {
                    cursor_x = 0;
                    cursor_y += font->charsize;
                }
            }
            str++; // Move to the next character in the string
        }
    }

};