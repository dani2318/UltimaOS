#pragma once
#include <libs/core/UEFI.h>
#include <globals.hpp>

static void kernel_panic(const char* msg, Framebuffer* fb, PSF1_Header* font){
    g_screenwriter->Clear(0xFFFF0000); 
    g_screenwriter->Print(msg); 
    while(1);
}
