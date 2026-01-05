#ifndef GLOBALS_H
#define GLOBALS_H
#include <Uefi.h>

#define EXCEPTION_CLEAR_COLOR 0x00FF0000
#define NORMAL_CLEAR_COLOR 0x000000FF
#define HHDM_OFFSET 0xFFFF800000000000ULL

#define EXTERNC extern "C"
#pragma once
#include <libs/core/UEFI.h>

// Forward declarations are not enough for the .cpp file, 
// but fine for pointers in the .hpp
class HAL;
class Serial;
class ScreenWriter;
class PhysicalMemoryManager;
class HeapAllocator;
class ACPIDriver;
class GDT;
class IDT;
class PageTable;

// Use extern for everything to be safe
extern HAL* g_hal;
extern Serial* g_serialWriter;
extern ScreenWriter* g_screenwriter;
extern PhysicalMemoryManager g_PMM;
extern HeapAllocator g_heap;
extern ACPIDriver* g_acpi;
extern GDT* g_gdt;
extern IDT* g_idt;
extern EFI_SYSTEM_TABLE* g_uefi_system_table;
extern EFI_HANDLE gImageHandle; // Note: EFI_HANDLE is usually already a pointer
extern PageTable *kernelPLM4;
extern struct TSS g_tss;
extern uint64_t _kernel_start;
extern uint64_t _kernel_end;
#endif