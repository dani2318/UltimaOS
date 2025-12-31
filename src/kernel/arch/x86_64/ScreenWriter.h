#pragma once
#include <arch/x86_64/UEFI.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct  {
    unsigned char magic[2];
    unsigned char mode;
    unsigned char charsize;
} PSF1_Header;

typedef struct {
    Framebuffer* target_fb;
    uint32_t cursor_x;
    uint32_t cursor_y;
    uint32_t color;
    PSF1_Header* font;
} ScreenWriter;

#ifndef __SCREEN_WRITER_H
#define __SCREEN_WRITER_H

// Function declarations
static void screen_writer_init(ScreenWriter* sw, Framebuffer* fb, PSF1_Header* font);
static void screen_writer_put_pixel(ScreenWriter* sw, uint32_t x, uint32_t y, uint32_t pixel_color);
static void screen_writer_clear(ScreenWriter* sw, uint32_t clear_color);
static void screen_writer_print_char(ScreenWriter* sw, char c, uint32_t xOff, uint32_t yOff);
static void screen_writer_print(ScreenWriter* sw, const char* str);

// Inline implementations (use static inline to avoid multiple definition errors)
static inline void screen_writer_init(ScreenWriter* sw, Framebuffer* fb, PSF1_Header* font) {
    sw->target_fb = fb;
    sw->font = font;
    sw->cursor_x = 0;
    sw->cursor_y = 0;
    sw->color = 0xFFFFFFFF; // White (ARGB format)
}

static inline void screen_writer_put_pixel(ScreenWriter* sw, uint32_t x, uint32_t y, uint32_t pixel_color) {
    if (x >= sw->target_fb->Width || y >= sw->target_fb->Height) return;
    
    uint32_t* pixel_ptr = (uint32_t*)sw->target_fb->BaseAddress;
    pixel_ptr[x + (y * sw->target_fb->PixelsPerScanLine)] = pixel_color;
}

static inline void screen_writer_clear(ScreenWriter* sw, uint32_t clear_color) {
    uint64_t pixels_to_clear = (uint64_t)sw->target_fb->Height * sw->target_fb->PixelsPerScanLine;
    uint32_t* pixel_ptr = (uint32_t*)sw->target_fb->BaseAddress;
    
    // Optimized: write directly without nested loops
    for (uint64_t i = 0; i < pixels_to_clear; i++) {
        pixel_ptr[i] = clear_color;
    }
}

static inline void screen_writer_print_char(ScreenWriter* sw, char c, uint32_t xOff, uint32_t yOff) {
    // Calculate where the glyph starts in memory
    // Skip the 4-byte header, then skip 'c' glyphs of 'charsize' bytes each
    unsigned char* glyph = (unsigned char*)sw->font + sizeof(PSF1_Header) + ((unsigned char)c * sw->font->charsize);
    
    for (uint32_t y = 0; y < sw->font->charsize; y++) {
        for (uint32_t x = 0; x < 8; x++) {
            // We use 0x80 (10000000) to mask the leftmost bit first
            if ((glyph[y] & (0x80 >> x)) != 0) {
                screen_writer_put_pixel(sw, xOff + x, yOff + y, sw->color);
            }
        }
    }
}

static inline void screen_writer_print(ScreenWriter* sw, const char* str) {

    while (*str != '\0') {
        // Handle newlines
        if (*str == '\n') {
            sw->cursor_y += sw->font->charsize;
            sw->cursor_x = 0;
        } 
        // Handle carriage return
        else if (*str == '\r') {
            sw->cursor_x = 0;
        }
        // Handle tab (4 spaces)
        else if (*str == '\t') {
            sw->cursor_x += 32; // 4 characters * 8 pixels
            if (sw->cursor_x >= sw->target_fb->Width) {
                sw->cursor_x = 0;
                sw->cursor_y += sw->font->charsize;
            }
        }
        // Regular character
        else {
            screen_writer_print_char(sw, *str, sw->cursor_x, sw->cursor_y);
            sw->cursor_x += 8; // Advance by character width
            
            // Basic line wrapping
            if (sw->cursor_x + 8 > sw->target_fb->Width) {
                sw->cursor_x = 0;
                sw->cursor_y += sw->font->charsize;
            }
            
            // Screen scrolling check (optional - comment out if you handle this elsewhere)
            if (sw->cursor_y + sw->font->charsize > sw->target_fb->Height) {
                sw->cursor_y = 0; // Simple wrap to top for now
            }
        }
        str++;
    }
}

void screen_print_hex(ScreenWriter* sw, uint64_t val) {
    char hex[] = "0123456789ABCDEF";
    char buf[17];
    buf[16] = '\0';

    for (int i = 15; i >= 0; i--) {
        buf[i] = hex[val & 0xF];
        val >>= 4;
    }

    screen_writer_print(sw, buf);
}


void screen_print_uint(ScreenWriter* sw, uint64_t value) {
    char buf[21];          // max 20 cifre + '\0'
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

    screen_writer_print(sw, &buf[i]);
}


#endif // __SCREEN_WRITER_H