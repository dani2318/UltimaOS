#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <memory/memory.hpp>
#include <globals.hpp>

class Bitmap
{
public:
    inline bool Get(uint64_t index) const
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
        return nullptr;
    }

    void *AllocateContiguous(size_t count)
    {
        size_t found_count = 0;
        size_t start_bit = 0;
        size_t total_bits = PageBitmap.Size * 8;

        for (size_t i = 0; i < total_bits; i++)
        {
            if (!PageBitmap.Get(i))
            {
                if (found_count == 0)
                    start_bit = i;
                found_count++;

                if (found_count == count)
                {
                    void *addr = (void *)(start_bit * PAGE_SIZE);
                    for (size_t j = 0; j < count; j++)
                    {
                        LockPage((void *)((uint64_t)addr + (j * PAGE_SIZE)));
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
        if (count == 0)
            return nullptr;
        if (count == 1)
            return RequestPage();

        return AllocateContiguous(count);
    }

    Bitmap PageBitmap;
    uint64_t TotalMemory;
    uint64_t FreeMemory;
    uint64_t UsedMemory;
};

extern PhysicalMemoryManager g_PMM;


namespace PageFlags
{
    constexpr uint64_t Present = (1ULL << 0);       // Page is present in memory
    constexpr uint64_t ReadWrite = (1ULL << 1);     // Page is writable
    constexpr uint64_t UserAccessible = (1ULL << 2); // User mode can access
    constexpr uint64_t WriteThrough = (1ULL << 3);   // Write-through caching
    constexpr uint64_t CacheDisable = (1ULL << 4);   // Disable caching
    constexpr uint64_t Accessed = (1ULL << 5);       // Page has been accessed
    constexpr uint64_t Dirty = (1ULL << 6);          // Page has been written to
    constexpr uint64_t PageSize = (1ULL << 7);       // 2MB/1GB page (not 4KB)
    constexpr uint64_t Global = (1ULL << 8);         // Don't flush from TLB on CR3 change
    constexpr uint64_t NoExecute = (1ULL << 63);     // Page is not executable
    
    // Address mask (bits 12-51)
    constexpr uint64_t AddressMask = 0x000FFFFFFFFFF000ULL;
}

struct PageTableEntry
{
    uint64_t value;
    
    // Check if a flag is set
    inline bool HasFlag(uint64_t flag) const
    {
        return (value & flag) != 0;
    }
    
    // Set or clear a flag
    inline void SetFlag(uint64_t flag, bool enable)
    {
        if (enable)
            value |= flag;
        else
            value &= ~flag;
    }
    
    // Check if entry is present
    inline bool IsPresent() const
    {
        return HasFlag(PageFlags::Present);
    }
    
    // Get physical address
    inline uint64_t GetAddress() const
    {
        return value & PageFlags::AddressMask;
    }
    
    // Set physical address (preserves flags)
    inline void SetAddress(uint64_t address)
    {
        value = (value & ~PageFlags::AddressMask) | (address & PageFlags::AddressMask);
    }
    
    // Clear the entire entry
    inline void Clear()
    {
        value = 0;
    }
    
    // Set entry with address and flags
    inline void Set(uint64_t address, uint64_t flags)
    {
        value = (address & PageFlags::AddressMask) | flags;
    }
};

struct PageTable
{
    PageTableEntry entries[512];
    
    // Get entry by index
    inline PageTableEntry& operator[](size_t index)
    {
        return entries[index];
    }
    
    inline const PageTableEntry& operator[](size_t index) const
    {
        return entries[index];
    }
} __attribute__((aligned(4096)));


class PageTableManager
{
public:
    PageTable *PML4;

    PageTableManager() : PML4(nullptr) {}
    
    // Initialize with a PML4 table
    inline void Init(PageTable *pml4)
    {
        PML4 = pml4;
    }
    
    // Get the current PML4
    inline PageTable* GetPML4() const
    {
        return PML4;
    }
    
    // Map a virtual address to a physical address
    void MapMemory(void *virtualAddr, void *physicalAddr, bool user = false, bool writable = true, bool executable = true)
    {
        uint64_t vAddr = (uint64_t)virtualAddr;
        uint64_t pAddr = (uint64_t)physicalAddr;

        // Extract indices from virtual address
        uint64_t pml4_idx = (vAddr >> 39) & 0x1FF;
        uint64_t pdpt_idx = (vAddr >> 30) & 0x1FF;
        uint64_t pd_idx = (vAddr >> 21) & 0x1FF;
        uint64_t pt_idx = (vAddr >> 12) & 0x1FF;

        // Walk/create PML4 -> PDPT
        PageTable *pdpt = GetOrCreateTable(PML4->entries[pml4_idx], user);
        if (!pdpt) return;

        // Walk/create PDPT -> PD
        PageTable *pd = GetOrCreateTable(pdpt->entries[pdpt_idx], user);
        if (!pd) return;

        // Walk/create PD -> PT
        PageTable *pt = GetOrCreateTable(pd->entries[pd_idx], user);
        if (!pt) return;

        // Set final page table entry
        uint64_t flags = PageFlags::Present;
        if (writable) flags |= PageFlags::ReadWrite;
        if (user) flags |= PageFlags::UserAccessible;
        if (!executable) flags |= PageFlags::NoExecute;
        
        pt->entries[pt_idx].Set(pAddr, flags);

        // Invalidate TLB for this page
        InvalidatePage(virtualAddr);
    }
    
    // Unmap a virtual address
    void UnmapMemory(void *virtualAddr)
    {
        uint64_t vAddr = (uint64_t)virtualAddr;

        // Extract indices
        uint64_t pml4_idx = (vAddr >> 39) & 0x1FF;
        uint64_t pdpt_idx = (vAddr >> 30) & 0x1FF;
        uint64_t pd_idx = (vAddr >> 21) & 0x1FF;
        uint64_t pt_idx = (vAddr >> 12) & 0x1FF;

        // Walk to PT (return if any level doesn't exist)
        if (!PML4->entries[pml4_idx].IsPresent()) return;
        PageTable *pdpt = (PageTable *)PML4->entries[pml4_idx].GetAddress();
        
        if (!pdpt->entries[pdpt_idx].IsPresent()) return;
        PageTable *pd = (PageTable *)pdpt->entries[pdpt_idx].GetAddress();
        
        if (!pd->entries[pd_idx].IsPresent()) return;
        PageTable *pt = (PageTable *)pd->entries[pd_idx].GetAddress();

        // Clear the entry
        pt->entries[pt_idx].Clear();

        // Invalidate TLB
        InvalidatePage(virtualAddr);
    }
    
    // Get physical address for a virtual address (returns 0 if not mapped)
    uint64_t GetPhysicalAddress(void *virtualAddr) const
    {
        uint64_t vAddr = (uint64_t)virtualAddr;

        // Extract indices
        uint64_t pml4_idx = (vAddr >> 39) & 0x1FF;
        uint64_t pdpt_idx = (vAddr >> 30) & 0x1FF;
        uint64_t pd_idx = (vAddr >> 21) & 0x1FF;
        uint64_t pt_idx = (vAddr >> 12) & 0x1FF;
        uint64_t offset = vAddr & 0xFFF;

        // Walk page table
        if (!PML4->entries[pml4_idx].IsPresent()) return 0;
        PageTable *pdpt = (PageTable *)PML4->entries[pml4_idx].GetAddress();
        
        if (!pdpt->entries[pdpt_idx].IsPresent()) return 0;
        PageTable *pd = (PageTable *)pdpt->entries[pdpt_idx].GetAddress();
        
        if (!pd->entries[pd_idx].IsPresent()) return 0;
        PageTable *pt = (PageTable *)pd->entries[pd_idx].GetAddress();
        
        if (!pt->entries[pt_idx].IsPresent()) return 0;

        return pt->entries[pt_idx].GetAddress() + offset;
    }
    
    // Check if a virtual address is mapped
    bool IsMapped(void *virtualAddr) const
    {
        return GetPhysicalAddress(virtualAddr) != 0;
    }
    
    // Map a range of memory
    void MapRange(void *virtualStart, void *physicalStart, size_t size, bool user = false, bool writable = true, bool executable = true)
    {
        uint64_t vAddr = (uint64_t)virtualStart & ~0xFFFULL;
        uint64_t pAddr = (uint64_t)physicalStart & ~0xFFFULL;
        uint64_t pages = (size + PAGE_SIZE - 1) / PAGE_SIZE;
        
        for (uint64_t i = 0; i < pages; i++)
        {
            MapMemory((void *)(vAddr + i * PAGE_SIZE), 
                     (void *)(pAddr + i * PAGE_SIZE), 
                     user, writable, executable);
        }
    }
    
    // Unmap a range of memory
    void UnmapRange(void *virtualStart, size_t size)
    {
        uint64_t vAddr = (uint64_t)virtualStart & ~0xFFFULL;
        uint64_t pages = (size + PAGE_SIZE - 1) / PAGE_SIZE;
        
        for (uint64_t i = 0; i < pages; i++)
        {
            UnmapMemory((void *)(vAddr + i * PAGE_SIZE));
        }
    }

private:
    // Get or create a page table for an entry  
    PageTable* GetOrCreateTable(PageTableEntry &entry, bool user)
    {
        if (entry.IsPresent())
        {
            return (PageTable *)entry.GetAddress();
        }
        
        // Allocate new page table
        // NOTE: This assumes physical memory is currently accessible (either through
        // identity mapping or UEFI's page table). During kernel initialization, we
        // build the new page table while still running on the bootloader's mapping.
        void *table = g_PMM.RequestPage();
        if (!table) return nullptr;
        
        // Zero the new page table
        memset(table, 0, PAGE_SIZE);
        
        // Set entry with appropriate flags
        uint64_t flags = PageFlags::Present | PageFlags::ReadWrite;
        if (user) flags |= PageFlags::UserAccessible;
        
        entry.Set((uint64_t)table, flags);
        
        return (PageTable *)table;
    }
    
    // Invalidate TLB for a page
    inline void InvalidatePage(void *virtualAddr)
    {
        __asm__ volatile("invlpg (%0)" : : "r"(virtualAddr) : "memory");
    }
};

// Create a new page table (allocates and clears it)
inline PageTable* CreatePageTable()
{
    PageTable *table = (PageTable *)g_PMM.RequestPage();
    if (table)
        memset(table, 0, PAGE_SIZE);
    return table;
}

// Switch to a different page table
inline void SwitchPageTable(PageTable *pml4)
{
    __asm__ volatile("mov %0, %%cr3" : : "r"(pml4) : "memory");
}

// Get current page table
inline PageTable* GetCurrentPageTable()
{
    PageTable *pml4;
    __asm__ volatile("mov %%cr3, %0" : "=r"(pml4));
    return pml4;
}