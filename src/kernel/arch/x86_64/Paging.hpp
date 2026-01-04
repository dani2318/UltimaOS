#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <memory/memory.hpp>
#include <globals.hpp>

class Bitmap
{
public:
    inline bool Get(uint64_t index)
    {
        if (index >= Size * 8)
            return true;
        return (Buffer[index / 8] & (1 << (index % 8))) != 0;
    }

    inline void Set(uint64_t index, bool value)
    {
        if (index >= Size * 8)
            return;
        if (value)
            Buffer[index / 8] |= (1 << (index % 8));
        else
            Buffer[index / 8] &= ~(1 << (index % 8));
    }

    size_t Size;
    uint8_t *Buffer;
};

#define PAGE_SIZE 4096

class PhysicalMemoryManager
{
private:
    uint64_t last_free_index = 0;

public:
    inline void FreePage(void *address)
    {
        uint64_t index = (uint64_t)address / PAGE_SIZE;
        if (!PageBitmap.Get(index))
            return;

        PageBitmap.Set(index, false);
        FreeMemory += PAGE_SIZE;
        UsedMemory -= PAGE_SIZE;

        if (index < last_free_index)
            last_free_index = index;
    }

    inline void LockPage(void *address)
    {
        uint64_t index = (uint64_t)address / PAGE_SIZE;
        if (PageBitmap.Get(index))
            return;

        PageBitmap.Set(index, true);
        FreeMemory -= PAGE_SIZE;
        UsedMemory += PAGE_SIZE;
    }

    inline void FreePages(void *address, uint64_t count)
    {
        for (uint64_t i = 0; i < count; i++)
        {
            FreePage((void *)((uint64_t)address + i * PAGE_SIZE));
        }
    }

    inline void LockPages(void *address, uint64_t count)
    {
        for (uint64_t i = 0; i < count; i++)
        {
            LockPage((void *)((uint64_t)address + i * PAGE_SIZE));
        }
    }

    inline void *RequestPage()
    {
        for (uint64_t i = last_free_index; i < PageBitmap.Size * 8; i++)
        {
            if (!PageBitmap.Get(i))
            {
                last_free_index = i;
                void *addr = (void *)(i * PAGE_SIZE);
                LockPage(addr);
                return addr;
            }
        }
        for (uint64_t i = 0; i < last_free_index; i++)
        {
            if (!PageBitmap.Get(i))
            {
                last_free_index = i;
                void *addr = (void *)(i * PAGE_SIZE);
                LockPage(addr);
                return addr;
            }
        }
        return NULL;
    }

    void *AllocateContiguous(size_t count)
    {
        size_t found_count = 0;
        size_t start_bit = 0;
        size_t total_bits = PageBitmap.Size * 8;

        for (size_t i = 0; i < total_bits; i++)
        {
            // Use your existing PageBitmap object instead of undefined BitmapBuffer
            if (!PageBitmap.Get(i)) 
            {
                if (found_count == 0)
                    start_bit = i;
                found_count++;

                if (found_count == count)
                {
                    void* addr = (void*)(start_bit * PAGE_SIZE);
                    // Lock all pages in the range
                    for (size_t j = 0; j < count; j++)
                    {
                        LockPage((void*)((uint64_t)addr + (j * PAGE_SIZE)));
                    }
                    return addr;
                }
            }
            else
            {
                found_count = 0;
            }
        }
        return nullptr;
    }

    void *RequestPages(size_t count)
    {
        if (count == 0) return nullptr;
        if (count == 1) return RequestPage();
        
        // Always use the contiguous allocator for multiple pages to ensure 
        // they are physically sequential
        return AllocateContiguous(count);
    }

    Bitmap PageBitmap;
    uint64_t TotalMemory;
    uint64_t FreeMemory;
    uint64_t UsedMemory;
};

class PageTableEntry
{
public:
    inline void SetFlag(uint64_t flag, bool enable)
    {
        if (enable)
            value |= flag;
        else
            value &= ~flag;
    }

    inline bool GetFlag(uint64_t flag)
    {
        return (value & flag) != 0;
    }

    inline void SetAddress(uint64_t address)
    {
        value &= 0xFFFULL;
        value |= address & 0x000FFFFFFFFFF000ULL;
    }

    inline uint64_t GetAddress()
    {
        return value & 0x000FFFFFFFFFF000ULL;
    }

    uint64_t value;
};

struct PageTable
{
    PageTableEntry entries[512];
} __attribute__((aligned(4096)));

extern PhysicalMemoryManager g_PMM;

class PageTableManager
{
public:
    PageTable *PML4;

    inline void Init(PageTable *pml4)
    {
        PML4 = pml4;
    }

    inline void MapMemory(void *virtualAddr, void *physicalAddr, bool user = false, bool executable = true)
    {
        uint64_t vAddr = (uint64_t)virtualAddr;

        uint64_t i4 = (vAddr >> 39) & 0x1FF;
        uint64_t i3 = (vAddr >> 30) & 0x1FF;
        uint64_t i2 = (vAddr >> 21) & 0x1FF;
        uint64_t i1 = (vAddr >> 12) & 0x1FF;

        PageTableEntry *entry;
        PageTable *PDP;
        PageTable *PD;
        PageTable *PT;
        uint64_t userFlag = (1ULL << 2);

        entry = &PML4->entries[i4];
        if (!entry->GetFlag(1ULL << 0))
        {
            PDP = (PageTable *)g_PMM.RequestPage();
            memset(PDP, 0, PAGE_SIZE);
            entry->SetAddress((uint64_t)PDP);
            entry->SetFlag(1ULL << 0, true); // Present
            entry->SetFlag(1ULL << 1, true); // RW
        }
        else
        {
            PDP = (PageTable *)entry->GetAddress();
        }

        entry = &PDP->entries[i3];
        if (!entry->GetFlag(1ULL << 0))
        {
            PD = (PageTable *)g_PMM.RequestPage();
            memset(PD, 0, PAGE_SIZE);
            entry->SetAddress((uint64_t)PD);
            entry->SetFlag(1ULL << 0, true);
            entry->SetFlag(1ULL << 1, true);
        }
        else
        {
            PD = (PageTable *)entry->GetAddress();
        }

        entry = &PD->entries[i2];
        if (!entry->GetFlag(1ULL << 0))
        {
            PT = (PageTable *)g_PMM.RequestPage();
            memset(PT, 0, PAGE_SIZE);
            entry->SetAddress((uint64_t)PT);
            entry->SetFlag(1ULL << 0, true);
            entry->SetFlag(1ULL << 1, true);
        }
        else
        {
            PT = (PageTable *)entry->GetAddress();
        }

        if (user) entry->SetFlag(userFlag, true);
        PT = (PageTable *)entry->GetAddress();

        entry = &PT->entries[i1];
        entry->SetAddress((uint64_t)physicalAddr);
        entry->SetFlag(1ULL << 0, true); // Present
        entry->SetFlag(1ULL << 1, true); // RW
        if (user) entry->SetFlag(userFlag, true);

        if (!executable)
        {
            entry->SetFlag(1ULL << 63, false); // NX bit, example (depends on your PageTableEntry)
        }

        __asm__ volatile("invlpg (%0)" : : "r"(virtualAddr) : "memory");
    }
};