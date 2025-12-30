#pragma once
#include <arch/x86_64/UEFI.hpp>
#include <arch/x86_64/ScreenWriter.hpp>

namespace arch{
    namespace x86_64{
        namespace Panic{
            static void kernel_panic(const char* msg, Framebuffer* fb, PSF1_Header* font){
                ScreenWriter VGAWriter(fb, font);
                VGAWriter.clear(0xFFFF0000); 
                VGAWriter.print(msg); 
                while(1);
            }
        }
    }
}