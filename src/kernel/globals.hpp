#pragma once
#ifndef GLOBALS_H
#define GLOBALS_H
#include <Uefi.h>
#include <libs/core/UEFI.h>
#include <arch/x86_64/Paging/Paging.hpp>


#define EXCEPTION_CLEAR_COLOR = 0x00FF0000,
#define NORMAL_CLEAR_COLOR = 0x0000000,
#define HHDM_OFFSET = 0xFFFF800000000000ULL,
#define PHYS_MAP_OFFSET = 0xFFFF888000000000ULL


#define EXTERNC extern "C"

class HAL;
class ScreenWriter;
class PhysicalMemoryManager;
class HeapAllocator;
class ACPIDriver;
class GDT;
class IDT;
class PageTable;
class Scheduler;

extern HAL* gHal;
extern ScreenWriter* gScreenwriter;
extern PhysicalMemoryManager gPmm;
extern HeapAllocator gHeap;
extern ACPIDriver* gAcpi;
extern GDT* gGdt;
extern IDT* gIdt;
extern EFI_SYSTEM_TABLE* gUefiSystemTable;
extern EFI_HANDLE gImageHandle;
extern PageTable *kernelPLM4;
extern struct TSS gTss;
extern uint64_t _kernel_start;
extern uint64_t _kernel_end;
extern Scheduler* gScheduler;
#endif
