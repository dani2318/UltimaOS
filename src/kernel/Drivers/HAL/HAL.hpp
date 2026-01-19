#pragma once
#include <stdint.h>
#include <libs/core/UEFI.h>

class PageTableManager;

class HAL
{
public:
    static void DisableInterrupts();
    static void EnableInterrupts();
    static void Halt();

    static void InitializeCPU();
    static void InitializePlatform(BootInfo*, PageTableManager&);
    static void SwitchPageTable(void* pml4);
    static uint64_t ReadTSC();
};
