#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <arch/x86_64/ScreenWriter.hpp>
#include <globals.hpp>
#include <memory/memory.hpp>
#include <arch/x86_64/Paging/Bitmap.hpp>

#define PAGE_SIZE 4096

class PhysicalMemoryManager
{
private:
    uint64_t m_last_free_index = 0;

public:
    void freePage(void *address)
    {
        uint64_t const index = (uint64_t)address / PAGE_SIZE;
        if (!pageBitmap.get(index))
            return;

        pageBitmap.set(index, false);
        freeMemory += PAGE_SIZE;
        usedMemory -= PAGE_SIZE;

        if (index < m_last_free_index)
            m_last_free_index = index;
    }

    void lockPage(void *address)
    {
        uint64_t const index = (uint64_t)address / PAGE_SIZE;
        if (pageBitmap.get(index))
            return;

        pageBitmap.set(index, true);
        freeMemory -= PAGE_SIZE;
        usedMemory += PAGE_SIZE;
    }

    void freePages(void *address, uint64_t count)
    {
        for (uint64_t i = 0; i < count; i++)
        {
            freePage((void *)((uint64_t)address + i * PAGE_SIZE));
        }
    }

    void lockPages(void *address, uint64_t count)
    {
        for (uint64_t i = 0; i < count; i++)
        {
            lockPage((void *)((uint64_t)address + i * PAGE_SIZE));
        }
    }

    void* requestPageVirtual() {
        void* physAddr = requestPage();
        return PHYS_TO_VIRT(physAddr);  // Convert to accessible virtual address
    }

    void *requestPage()
    {
        const uint64_t MIN_PAGE = 256; // Start from 1MB

        gScreenwriter->print("PMM: RequestPage called, last_free_index=", true);
        gScreenwriter->printHex(m_last_free_index, true);
        gScreenwriter->print("\n", true);

        uint64_t const start = (m_last_free_index < MIN_PAGE) ? MIN_PAGE : m_last_free_index;

        gScreenwriter->print("PMM: Searching from page ", true);
        gScreenwriter->printHex(start, true);
        gScreenwriter->print("\n", true);

        for (uint64_t i = start; i < pageBitmap.size * 8; i++)
        {
            if (!pageBitmap.get(i))
            {
                m_last_free_index = i;
                void *addr = (void *)(i * PAGE_SIZE);

                gScreenwriter->print("PMM: Found free page ", true);
                gScreenwriter->printHex(i, true);
                gScreenwriter->print(" at address 0x", true);
                gScreenwriter->printHex((uint64_t)addr, true);
                gScreenwriter->print("\n", true);

                lockPage(addr);
                return addr;
            }
        }

        // Wrap around
        for (uint64_t i = MIN_PAGE; i < start; i++)
        {
            if (!pageBitmap.get(i))
            {
                m_last_free_index = i;
                void *addr = (void *)(i * PAGE_SIZE);

                gScreenwriter->print("PMM: Found free page (wrapped) ", true);
                gScreenwriter->printHex(i, true);
                gScreenwriter->print(" at address 0x", true);
                gScreenwriter->printHex((uint64_t)addr, true);
                gScreenwriter->print("\n", true);

                lockPage(addr);
                return addr;
            }
        }

        gScreenwriter->print("PMM: ERROR - No free pages!\n", true);
        return nullptr;
    }

    void initializePmm(BootInfo *bInfo, uint64_t kernelPages, void *bitmapBuffer, uint64_t bitmapSize)
    {
        gScreenwriter->print("K: InitializePMM started\n", true);

        m_last_free_index = 0; // Start from 0 temporarily

        uint64_t memSize = 0;
        for (uint64_t i = 0; i < bInfo->mmap_size; i += bInfo->descriptor_size)
        {
            MemoryDescriptor *desc = (MemoryDescriptor *)((uint64_t)bInfo->mmap_address + i);
            memSize += desc->NumberOfPages * 4096;
        }
        gPmm.totalMemory = memSize;

        memset(bitmapBuffer, 0xFF, bitmapSize);
        gPmm.pageBitmap.buffer = (uint8_t *)bitmapBuffer;
        gPmm.pageBitmap.size = bitmapSize;

        for (uint64_t i = 0; i < bInfo->mmap_size; i += bInfo->descriptor_size)
        {
            MemoryDescriptor *desc = (MemoryDescriptor *)((uint64_t)bInfo->mmap_address + i);
            if (desc->Type == EfiConventionalMemory)
            {
                gPmm.freePages((void *)desc->PhysicalStart, desc->NumberOfPages);
            }
        }

        gScreenwriter->print("K: Locking 0-2MB for safety\n", true);
        for (uint64_t i = 0; i < 512; i++)
        { // 512 pages = 2MB
            pageBitmap.set(i, true);
        }

        // *** NOW set last_free_index to skip locked area ***
        m_last_free_index = 512; // Start allocations from 2MB

        gScreenwriter->print("K: Locking kernel pages\n", true);
        gPmm.lockPages((void *)&_kernel_start, kernelPages);

        uint64_t const bitmapPages = (bitmapSize + 4095) / 4096;
        gScreenwriter->print("K: Locking bitmap pages\n", true);
        gPmm.lockPages(bitmapBuffer, bitmapPages);

        gScreenwriter->print("K: InitializePMM complete, last_free_index = ", true);
        gScreenwriter->printHex(m_last_free_index, true);
        gScreenwriter->print("\n", true);
    }

    void *allocateContiguous(size_t count)
    {
        const uint64_t MIN_PAGE = 256; // Start from 1MB
        size_t foundCount = 0;
        size_t startBit = 0;
        size_t const totalBits = pageBitmap.size * 8;

        for (size_t i = MIN_PAGE; i < totalBits; i++) // ← Start from MIN_PAGE
        {
            if (!pageBitmap.get(i))
            {
                if (foundCount == 0)
                    startBit = i;
                foundCount++;

                if (foundCount == count)
                {
                    void *addr = (void *)(startBit * PAGE_SIZE);
                    for (size_t j = 0; j < count; j++)
                    {
                        lockPage((void *)((uint64_t)addr + (j * PAGE_SIZE)));
                    }
                    return addr;
                }
            }
            else
            {
                foundCount = 0;
            }
        }
        return nullptr;
    }

    void *requestPages(size_t count)
    {
        if (count == 0)
            return nullptr;
        if (count == 1)
            return requestPage();

        // Always use the contiguous allocator for multiple pages to ensure
        // they are physically sequential
        return allocateContiguous(count);
    }

    Bitmap pageBitmap{};
    uint64_t totalMemory{};
    uint64_t freeMemory{};
    uint64_t usedMemory{};
};
