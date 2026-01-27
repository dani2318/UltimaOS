#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <memory/memory.hpp>
#include <globals.hpp>
#include <arch/x86_64/Paging/PagingConstants.hpp>
#include <arch/x86_64/Paging/PageTableEntry.hpp>
#include <arch/x86_64/Paging/PhysicalMemoryManager.hpp>

enum PageFlags : uint64_t
{
    PAGE_PRESENT       = 1ULL << 0,
    PAGE_WRITABLE      = 1ULL << 1,
    PAGE_USER          = 1ULL << 2,
    PAGE_WRITE_THROUGH = 1ULL << 3,
    PAGE_CACHE_DISABLE = 1ULL << 4,
    PAGE_NO_EXECUTE    = 1ULL << 63
};

struct PageTable
{
    PageTableEntry entries[512];
} __attribute__((aligned(4096)));

extern PhysicalMemoryManager gPmm;

class PageTableManager {
public:
    PageTableManager(uint64_t pml4Phys);

    // Map a virtual page to a physical page
    void mapPage(
        uint64_t virt,
        uint64_t phys,
        bool writable,
        bool user,
        bool executable
    );

    // Unmap a virtual page
    void unmapPage(uint64_t virt);

    // Get physical address from virtual address
    uint64_t getPhysicalAddress(uint64_t virt);
    PageTable* GetNextTable(PageTableEntry& entry, bool user);

    // Get the physical address of the PML4
    [[nodiscard]] uint64_t getPmL4Phys() const;

private:
    static uint64_t pml4_phys;
    static PageTable* pml4_virt;

    PageTable* getNextTable(PageTableEntry& entry, bool user);
};
