#include <stdint.h>
#include <stdbool.h>

#include <memory/memory.hpp>
#include "libs/core/UEFI.h"
#include "arch/x86_64/ScreenWriter.hpp"
#include <arch/x86_64/Paging/Paging.hpp>
#include <globals.hpp>
#include <cpp/cppSupport.hpp>
#include <cpp/String.hpp>

#include <arch/x86_64/Interrupts/PIT/PIT.hpp>
#include <Drivers/Input/Keyboard.hpp>
#include <Drivers/firmware/ACPI/ACPI.hpp>
#include <Drivers/HAL/HAL.hpp>
#include <Drivers/ELF/ELFLoader.hpp>
#include <Drivers/Storage/Filesystem/Filesystem.hpp>
#include <arch/x86_64/Multitasking/Scheduler.hpp>

#include <arch/x86_64/Interrupts/GDT/GDT.hpp>
#include <arch/x86_64/Interrupts/IDT/IDT.hpp>
#include <Drivers/Storage/Filesystem/GPT.hpp>
#include <Drivers/Storage/Filesystem/FAT32.hpp>
#include <Drivers/ELF/ELFHeaders.hpp>

struct KernelAPI
{
    void (*Print)(const char *);
    char (*GetChar)();
    void (*Exit)();
    void (*Clear)(uint32_t);
};

// Static wrappers to match the function pointer signature
void api_print(const char *s) { gScreenwriter->print(s, false); }
char api_get_char() { return Keyboard::HasChar() ? Keyboard::GetChar() : 0; }
void api_clear(uint32_t col)
{
    gScreenwriter->clear(col);
    gScreenwriter->setCursor(0, 0);
}

PageTable *kernelPLM4;

void SwitchPageTable(PageTable *pml4)
{
    __asm__ volatile("mov %0, %%cr3" : : "r"(pml4) : "memory");
}

PsF1Header *loadConsoleFont(BootInfo *b_info)
{
    PsF1Header *font = (PsF1Header *)b_info->font_address;
    if (font->magic[0] != 0x36 || font->magic[1] != 0x04)
    {
        gScreenwriter->print("Error: Font file corrupted or invalid format.\n", true);
        while (1)
        {
            __asm__ volatile("hlt");
        }
    }
    return font;
}

struct MemoryRegion
{
    MemoryDescriptor *descriptor;
    uint64_t size;
    uint64_t bitmap_size;
    uint64_t bitmap_pages;
    uint64_t kernel_start;
    uint64_t kernel_end;
    uint64_t kernel_size;
    uint64_t kernel_pages;
};

typedef struct MemoryRegion MemoryRegion;

MemoryRegion *AnalizeMemory(BootInfo *b_info)
{
    uint64_t mmap_entries = b_info->mmap_size / b_info->descriptor_size;
    uint64_t total_mem = 0;
    uint64_t usable_mem = 0;

    for (uint64_t i = 0; i < mmap_entries; i++)
    {
        auto *desc = (MemoryDescriptor *)((uint8_t *)b_info->mmap_address + i * b_info->descriptor_size);
        uint64_t mem_size = desc->NumberOfPages * 4096;
        total_mem += mem_size;

        if (desc->Type == EfiConventionalMemory)
            usable_mem += mem_size;
    }

    uint64_t bitmap_size = (total_mem / 4096 / 8) + 1;
    uint64_t bitmap_pages = (bitmap_size + 4095) / 4096;

    uint64_t kernel_start = (uint64_t)&_kernel_start;
    uint64_t kernel_end = (uint64_t)&_kernel_end;
    uint64_t kernel_size = kernel_end - kernel_start;
    uint64_t kernel_pages = (kernel_size + 4095) / 4096;

    MemoryDescriptor *largest = nullptr;
    uint64_t max_size = 0;

    for (uint64_t i = 0; i < mmap_entries; i++)
    {
        auto *desc = (MemoryDescriptor *)((uint8_t *)b_info->mmap_address + i * b_info->descriptor_size);

        if (desc->Type != EfiConventionalMemory)
            continue;

        uint64_t size = desc->NumberOfPages * 4096;

        // Skip memory above 4GB (avoid DMA issues)
        if (desc->PhysicalStart + size >= 0x100000000ULL)
            continue;

        // Skip if overlaps with kernel
        if (desc->PhysicalStart < kernel_end && desc->PhysicalStart + size > kernel_start)
            continue;

        // Need enough space for bitmap + reasonable heap
        if (size < bitmap_pages * 4096 + 0x100000) // At least 1MB heap
            continue;

        if (size > max_size)
        {
            max_size = size;
            largest = desc;
        }
    }

    if (!largest)
    {
        gScreenwriter->print("FATAL: No suitable memory for heap\n", false);
        while (true)
            gHal->Halt();
    }

    static MemoryRegion region = {
        largest,
        max_size,
        bitmap_size,
        bitmap_pages,
        kernel_start,
        kernel_end,
        kernel_size,
        kernel_pages};

    return &region;
}

struct HeapDescriptor
{
    void *bitmap_buffer;
    uint64_t heap_phys;
    uint64_t heap_size;
};

typedef struct HeapDescriptor HeapDescriptor;

HeapDescriptor *InitializeHeap(
    MemoryDescriptor *descriptor,
    uint64_t max_size,
    uint64_t bitmap_pages,
    uint64_t bitmap_size,
    uint64_t kernel_pages,
    BootInfo *b_info,
    PageTableManager &ptm) // <-- pass PageTableManager
{
    // Physical start of bitmap
    uint64_t bitmap_phys = descriptor->PhysicalStart;

    // Heap starts after bitmap
    uint64_t heap_phys = bitmap_phys + bitmap_pages * 4096;
    uint64_t heap_size = max_size - bitmap_pages * 4096;

    // Map bitmap pages identity (or high-half)
    for (uint64_t i = 0; i < bitmap_pages; i++)
    {
        uint64_t phys = bitmap_phys + i * 4096;
        uint64_t virt = phys + KERNEL_BASE; // high-half
        ptm.mapPage(virt, phys, true, false, false);
    }

    // Map heap pages
    for (uint64_t i = 0; i < heap_size; i += 4096)
    {
        uint64_t phys = heap_phys + i;
        uint64_t virt = phys + KERNEL_BASE;
        ptm.mapPage(virt, phys, true, false, false);
    }

    // Initialize g_heap at high-half virtual address
    gHeap.init((void *)(heap_phys + KERNEL_BASE), heap_size);

    // Initialize PMM
    gPmm.initializePmm(b_info, kernel_pages, (void *)(bitmap_phys + KERNEL_BASE), bitmap_size);

    static HeapDescriptor heapDescriptor = {
        (void *)(bitmap_phys + KERNEL_BASE),
        heap_phys,
        heap_size};

    return &heapDescriptor;
}

void SwitchPageTable(PageTableManager &ptm)
{
    uint64_t cr3 = ptm.getPmL4Phys();
    asm volatile("mov %0, %%cr3" ::"r"(cr3) : "memory");
}

// Add this function before loading the ELF to test the heap
void VerifyHeapIntegrity()
{
    gScreenwriter->print("\n=== Heap Integrity Check ===\n", false);

    // Check 1: Is heap initialized?
    if (!gHeap.initialized)
    {
        gScreenwriter->print("ERROR: Heap not initialized!\n", false);
        while (true)
            gHal->Halt();
    }
    gScreenwriter->print("✓ Heap initialized\n", false);

    // Check 2: Is first_block valid?
    if (!gHeap.first_block)
    {
        gScreenwriter->print("ERROR: first_block is NULL!\n", false);
        while (true)
            gHal->Halt();
    }
    gScreenwriter->print("✓ first_block: 0x", false);
    gScreenwriter->printHex((uint64_t)gHeap.first_block, false);
    gScreenwriter->print("\n", false);

    // Check 3: Can we read first_block?
    gScreenwriter->print("  Checking magic...\n", false);
    uint32_t magic = gHeap.first_block->magic;

    gScreenwriter->print("  Magic: 0x", false);
    gScreenwriter->printHex(magic, false);
    gScreenwriter->print("\n", false);

    if (magic != 0xDEADBEEF)
    {
        gScreenwriter->print("ERROR: Heap corrupted! Magic = 0x", false);
        gScreenwriter->printHex(magic, false);
        gScreenwriter->print("\n", false);
        while (true)
            gHal->Halt();
    }
    gScreenwriter->print("✓ Magic OK\n", false);

    // Check 4: Test allocation
    gScreenwriter->print("  Testing small allocation...\n", false);
    void *test1 = gHeap.alloc(64);
    if (!test1)
    {
        gScreenwriter->print("ERROR: Cannot allocate 64 bytes!\n", false);
        while (true)
            gHal->Halt();
    }
    gScreenwriter->print("  ✓ Allocated at 0x", false);
    gScreenwriter->printHex((uint64_t)test1, false);
    gScreenwriter->print("\n", false);

    // Check 5: Test write
    gScreenwriter->print("  Testing write...\n", false);
    ((uint8_t *)test1)[0] = 0xAA;
    ((uint8_t *)test1)[63] = 0xBB;

    if (((uint8_t *)test1)[0] != 0xAA || ((uint8_t *)test1)[63] != 0xBB)
    {
        gScreenwriter->print("ERROR: Memory write failed!\n", false);
        while (true)
            gHal->Halt();
    }
    gScreenwriter->print("  ✓ Write OK\n", false);

    // Check 6: Test larger allocation
    gScreenwriter->print("  Testing 4KB allocation...\n", false);
    void *test2 = gHeap.alloc(4096);
    if (!test2)
    {
        gScreenwriter->print("ERROR: Cannot allocate 4KB!\n", false);
        gHeap.free(test1);
        while (true)
            gHal->Halt();
    }
    gScreenwriter->print("  ✓ Allocated at 0x", false);
    gScreenwriter->printHex((uint64_t)test2, false);
    gScreenwriter->print("\n", false);

    // Check 7: Test free
    gScreenwriter->print("  Testing free...\n", false);
    gHeap.free(test1);
    gHeap.free(test2);
    gScreenwriter->print("  ✓ Free OK\n", false);

    // Check 8: Test reallocation
    gScreenwriter->print("  Testing reallocation...\n", false);
    void *test3 = gHeap.alloc(128);
    if (!test3)
    {
        gScreenwriter->print("ERROR: Cannot reallocate!\n", false);
        while (true)
            gHal->Halt();
    }
    gHeap.free(test3);
    gScreenwriter->print("  ✓ Reallocation OK\n", false);

    gScreenwriter->print("✓ All heap checks passed!\n\n", false);
}

EXTERNC __attribute__((section(".text.start"))) void start(BootInfo *b_info, EFI_SYSTEM_TABLE *system_table, EFI_HANDLE *ImageHandle)
{
    gImageHandle = ImageHandle;
    gUefiSystemTable = system_table;

    gHal = new HAL();
    gHal->DisableInterrupts();
    gHal->InitializeCPU();

    PsF1Header *font = loadConsoleFont(b_info);
    gScreenwriter = new ScreenWriter(
        (Framebuffer *)&b_info->framebuffer, font);

    gScreenwriter->clear(0x00000000);
    gScreenwriter->print("NeoOS: Kernel booting...\n", false);

    // Step 1: Analyze memory
    gScreenwriter->print("Step 1: Analyzing memory...\n", false);
    MemoryRegion *region = AnalizeMemory(b_info);

    uint64_t bitmap_phys = region->descriptor->PhysicalStart;
    uint64_t heap_phys = bitmap_phys + region->bitmap_pages * 4096;
    uint64_t heap_size = region->size - region->bitmap_pages * 4096;

    // Safety check: Ensure we actually have heap space
    if (heap_size < 0x100000) {
        gScreenwriter->print("FATAL: Not enough memory for heap!\n", false);
        while(true) gHal->Halt();
    }

    char buf[32];
    gScreenwriter->print("  Bitmap: 0x", false);
    gScreenwriter->printHex(bitmap_phys, false);
    gScreenwriter->print("\n", false);

    gScreenwriter->print("  Heap: 0x", false);
    gScreenwriter->printHex(heap_phys, false);
    gScreenwriter->print(" (", false);
    gScreenwriter->print(itoa(heap_size / 1024 / 1024, buf, 10), false);
    gScreenwriter->print(" MB)\n", false);

    // Step 2: Initialize heap and PMM BEFORE building page tables
    gScreenwriter->print("Step 2: Initializing memory management...\n", false);
    gScreenwriter->print("  Initializing heap at 0x", false);
    gScreenwriter->printHex(heap_phys, false);
    gScreenwriter->print("...\n", false);

    gHeap.init((void *)heap_phys, heap_size);

    gScreenwriter->print("  Initializing PMM...\n", false);
    gPmm.initializePmm(b_info, region->kernel_pages, (void *)bitmap_phys, region->bitmap_size);

    // ================== FIX START ==================
    // Lock the pages used by the heap so the PMM doesn't hand them out again
    gScreenwriter->print("  Locking heap pages in PMM...\n", false);

    uint64_t heapPageCount = heap_size / 4096;
    if (heap_size % 4096 != 0) heapPageCount++;

    for (uint64_t i = 0; i < heapPageCount; i++)
    {
        uint64_t pageAddr = heap_phys + (i * 4096);
        gPmm.lockPage((void*)pageAddr);
    }

    gScreenwriter->print("  Done\n", false);

    // Step 3: Build page tables
    gScreenwriter->print("Step 3: Building page tables...\n", false);

    gScreenwriter->print("  Allocating PML4...\n", false);
    uint64_t pml4_phys = (uint64_t)gPmm.requestPage();
    if (!pml4_phys)
    {
        gScreenwriter->print("FATAL: Cannot allocate PML4\n", false);
        while (true)
            gHal->Halt();
    }

    gScreenwriter->print("  PML4 at 0x", false);
    gScreenwriter->printHex(pml4_phys, false);
    gScreenwriter->print("\n", false);

    PageTableManager ptm(pml4_phys);

    // Step 4: Map ALL memory 0-4GB identity
    gScreenwriter->print("Step 4: Mapping memory (0-4GB)...\n", false);
    gScreenwriter->print("  Progress: ", false);

    uint64_t last_print = 0;
    for (uint64_t addr = 0; addr < 0x100000000ULL; addr += PAGE_SIZE)
    {
        ptm.mapPage(addr, addr, true, false, true);

        // Print progress every 256MB
        if (addr - last_print >= 0x10000000ULL)
        {
            gScreenwriter->print(itoa(addr / 1024 / 1024, buf, 10), false);
            gScreenwriter->print("MB ", false);
            last_print = addr;
        }
    }

    gScreenwriter->print("\n  Complete\n", false);

    // Step 5: Map framebuffer if above 4GB
    uint64_t fb_base = b_info->framebuffer.BaseAddress;
    uint64_t fb_size = b_info->framebuffer.BufferSize;

    if (fb_base >= 0x100000000ULL)
    {
        gScreenwriter->print("Step 5: Mapping framebuffer...\n", false);
        for (uint64_t off = 0; off < fb_size; off += PAGE_SIZE)
        {
            ptm.mapPage(fb_base + off, fb_base + off, true, false, false);
        }
    }
    else
    {
        gScreenwriter->print("Step 5: Framebuffer already mapped\n", false);
    }

    // Step 6: Switch to new page tables
    gScreenwriter->print("Step 6: Activating page tables...\n", false);
    gScreenwriter->print("  CR3 <- 0x", false);
    gScreenwriter->printHex(pml4_phys, false);
    gScreenwriter->print("\n", false);

    asm volatile("mfence" ::: "memory");
    asm volatile("mov %0, %%cr3" ::"r"(pml4_phys) : "memory");
    asm volatile("mfence" ::: "memory");

    gScreenwriter->print("  ✓ Paging active\n", false);

    // Step 7: Verify heap still works after page switch
    gScreenwriter->print("Step 7: Verifying heap...\n", false);
    void *test = gHeap.alloc(128);
    if (!test)
    {
        gScreenwriter->print("  FATAL: Heap broken!\n", false);
        while (true)
            gHal->Halt();
    }
    ((uint8_t *)test)[0] = 0xAA;
    if (((uint8_t *)test)[0] != 0xAA)
    {
        gScreenwriter->print("  FATAL: Memory not writable!\n", false);
        while (true)
            gHal->Halt();
    }
    gHeap.free(test);
    gScreenwriter->print("  ✓ Heap OK\n", false);

    // Step 8: GDT and IDT (skip C++ constructors - we use 'new' instead)
    gScreenwriter->print("Step 8: GDT and IDT...\n", false);
    gGdt = new GDT();
    gGdt->Initialize();

    gIdt = new IDT();
    gIdt->Initialize();

    // Step 9: Platform
    gScreenwriter->print("Step 9: Platform...\n", false);
    gHal->InitializePlatform(b_info, ptm);
    gHal->EnableInterrupts();

    // Step 10: Scheduler
    gScreenwriter->print("Step 10: Scheduler...\n", false);
    gScheduler = new Scheduler();

    gScreenwriter->print("\n=== System Ready ===\n", false);
    gScreenwriter->print("Free memory: ", false);
    gScreenwriter->print(itoa(gPmm.freeMemory / 1024 / 1024, buf, 10), false);
    gScreenwriter->print(" MB\n\n", false);

    gScreenwriter->print("Actual Heap Size: ", false);
    gScreenwriter->print(itoa(heap_size, buf, 10), false);
    gScreenwriter->print(" bytes\n", false);

    // Step 11: Load program
    gScreenwriter->print("Loading program...\n\n", false);

    ELFLoader loader(&ptm);
    uint64_t entry = loader.LoadELFFromDisk("PROGRAMS/SHELL.ELF");

    if (entry == 0)
    {
        gScreenwriter->print("Failed to calculate entry point!\n", true);
        return;
    }

    // Verify alignment (x86-64 instructions should be at valid addresses)
    if (entry == 0 || entry < 0x1000)
    {
        gScreenwriter->print("Entry point looks invalid!\n", true);
        return;
    }

    if (entry != 0)
    {
        gScreenwriter->print("\n✓ Loaded successfully\n", false);
        gScreenwriter->print("Entry point: 0x", false);
        gScreenwriter->printHex(entry, false);
        gScreenwriter->print("\n\n", false);

        typedef void (*EntryFunc)();
        ((EntryFunc)entry)();
    }
    else
    {
        gScreenwriter->print("✗ Failed to load program\n", false);
    }

    gScreenwriter->print("\nKernel idle.\n", false);
    while (true)
        gHal->Halt();
}
