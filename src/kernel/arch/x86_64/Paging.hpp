#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <memory/memory.hpp>

struct Bitmap {
    size_t Size;    // Size in bytes
    uint8_t* Buffer;

    bool operator[](uint64_t index) {
        return (Buffer[index / 8] & (1 << (index % 8))) != 0;
    }

    void Set(uint64_t index, bool value) {
        if (value) Buffer[index / 8] |= (1 << (index % 8));
        else Buffer[index / 8] &= ~(1 << (index % 8));
    }
};

class PhysicalMemoryManager {
public:
    Bitmap PageBitmap; // This matches your InitializePMM logic
    uint64_t TotalMemory;
    uint64_t FreeMemory;
    uint64_t UsedMemory;

    void FreePage(void* address) {
        uint64_t index = (uint64_t)address / 4096;
        if (PageBitmap[index] == false) return;
        PageBitmap.Set(index, false);
        FreeMemory += 4096;
        UsedMemory -= 4096;
    }

    void LockPage(void* address) {
        uint64_t index = (uint64_t)address / 4096;
        if (PageBitmap[index] == true) return;
        PageBitmap.Set(index, true);
        FreeMemory -= 4096;
        UsedMemory += 4096;
    }

    void FreePages(void* address, uint64_t count) {
        for (uint64_t i = 0; i < count; i++) {
            FreePage((void*)((uint64_t)address + (i * 4096)));
        }
    }

    void LockPages(void* address, uint64_t count) {
        for (uint64_t i = 0; i < count; i++) {
            LockPage((void*)((uint64_t)address + (i * 4096)));
        }
    }

    void* RequestPage() {
        for (uint64_t i = 0; i < PageBitmap.Size * 8; i++) {
            if (PageBitmap[i] == false) {
                void* addr = (void*)(i * 4096);
                LockPage(addr);
                return addr;
            }
        }
        return nullptr; // Out of memory
    }
};

extern PhysicalMemoryManager globalPMM;

class PageTableEntry
{

public:
    void SetFlag(uint64_t flag, bool enable);
    bool GetFlag(uint64_t flag);
    void SetAddress(uint64_t address);
    uint64_t GetAddress();

private:
    uint64_t value;
};

class PageTable
{
public:
    PageTableEntry entries[512];
};

class PageTableManager
{
public:
    PageTable *PML4;

    PageTableManager(PageTable *pml4Address) : PML4(pml4Address) {}
    void MapMemory(void *virtualAddr, void *physicalAddr)
    {
        uint64_t vAddr = (uint64_t)virtualAddr;

        // 1. Extract indices
        uint64_t i4 = (vAddr >> 39) & 0x1FF;
        uint64_t i3 = (vAddr >> 30) & 0x1FF;
        uint64_t i2 = (vAddr >> 21) & 0x1FF;
        uint64_t i1 = (vAddr >> 12) & 0x1FF;

        PageTableEntry PDE;

        // --- LEVEL 4 -> LEVEL 3 ---
        PDE = PML4->entries[i4];
        PageTable *PDP;
        if (!PDE.GetFlag(1 << 0))
        { // If Level 3 Table doesn't exist
            PDP = (PageTable *)globalPMM.RequestPage();
            memset(PDP, 0, 4096);
            PDE.SetAddress((uint64_t)PDP);
            PDE.SetFlag(1 << 0, true); // Present
            PDE.SetFlag(1 << 1, true); // Writable
            PML4->entries[i4] = PDE;
        }
        else
        {
            PDP = (PageTable *)PDE.GetAddress();
        }

        // --- LEVEL 3 -> LEVEL 2 ---
        PDE = PDP->entries[i3]; // Use PDP, not PML4!
        PageTable *PD;
        if (!PDE.GetFlag(1 << 0))
        {
            PD = (PageTable *)globalPMM.RequestPage();
            memset(PD, 0, 4096);
            PDE.SetAddress((uint64_t)PD);
            PDE.SetFlag(1 << 0, true);
            PDE.SetFlag(1 << 1, true);
            PDP->entries[i3] = PDE;
        }
        else
        {
            PD = (PageTable *)PDE.GetAddress();
        }

        // --- LEVEL 2 -> LEVEL 1 ---
        PDE = PD->entries[i2]; // Use PD, not PDP!
        PageTable *PT;
        if (!PDE.GetFlag(1 << 0))
        {
            PT = (PageTable *)globalPMM.RequestPage();
            memset(PT, 0, 4096);
            PDE.SetAddress((uint64_t)PT);
            PDE.SetFlag(1 << 0, true);
            PDE.SetFlag(1 << 1, true);
            PD->entries[i2] = PDE;
        }
        else
        {
            PT = (PageTable *)PDE.GetAddress();
        }

        // --- LEVEL 1 -> ACTUAL PHYSICAL PAGE ---
        PageTableEntry &finalEntry = PT->entries[i1];
        finalEntry.SetAddress((uint64_t)physicalAddr);
        finalEntry.SetFlag(1 << 0, true); // Present
        finalEntry.SetFlag(1 << 1, true); // Writable
    };
};