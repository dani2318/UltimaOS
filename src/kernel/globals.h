#ifndef GLOBALS_H
#define GLOBALS_H

#include <arch/x86_64/Serial.h>
#include <arch/x86_64/ScreenWriter.h>
#include <arch/x86_64/Paging.h>  // Include it AFTER the types it doesn't depend on

extern PhysicalMemoryManager g_PMM;
extern Serial g_serialWriter;
extern ScreenWriter g_screenwriter;
extern BumpAllocator g_allocator;

extern char _kernel_start;
extern char _kernel_end;
extern PSF1_Header* font;

#endif