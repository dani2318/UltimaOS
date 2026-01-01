#pragma once
#include <arch/x86_64/UEFI.h>
#include <globals.hpp>

static void kernel_panic(const char* msg, Framebuffer* fb, PSF1_Header* font){
    g_screenwriter->Clear(0xFFFF0000); 
    g_screenwriter->Print(msg); 
    while(1);
}
