#pragma once
#include <stdint.h>
#include <stdbool.h>

void VGA_setCursor(int x, int y);
void VGA_clrscr();
void VGA_putc(char c);
void VGA_puts(const char* str);
void VGA_printf(const char* fmt, ...);
void VGA_print_buffer(const char* msg, const void* buffer, uint32_t count);