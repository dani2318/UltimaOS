#ifndef GLOBALS_H
#define GLOBALS_H

#include <arch/x86_64/Serial.hpp>
#include <arch/x86_64/ScreenWriter.hpp>
#include <arch/x86_64/Paging.hpp>
#include <arch/x86_64/IDT/IDT.hpp>
#include <arch/x86_64/IRQ/irq.hpp>

class GDT;
class IDT;
class IRQ;

#define EXTERNC extern "C"

extern PhysicalMemoryManager g_PMM;
extern Serial* g_serialWriter;
extern ScreenWriter* g_screenwriter;
extern BumpAllocator g_allocator;
extern HeapAllocator g_heap;
extern GDT* _gdt;
extern IDT* idt;
extern IRQ* irq;

extern char _kernel_start;
extern char _kernel_end;
extern PSF1_Header* font;

#endif