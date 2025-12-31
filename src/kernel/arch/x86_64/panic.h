#pragma once
#include <arch/x86_64/UEFI.h>
#include <arch/x86_64/ScreenWriter.h>
#include <globals.h>

static void kernel_panic(const char* msg, Framebuffer* fb, PSF1_Header* font){
    screen_writer_init(&g_screenwriter, fb, font);
    screen_writer_clear(&g_screenwriter,0xFFFF0000); 
    screen_writer_print(&g_screenwriter,msg); 
    while(1);
}
