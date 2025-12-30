#include <stdint.h>

#include <memory/memory.hpp>
#include <arch/x86_64/UEFI.hpp>
#include <arch/x86_64/Serial.hpp>
#include <arch/x86_64/cpp/cppSupport.hpp>
#include <arch/x86_64/ScreenWriter.hpp>
#include <arch/x86_64/panic.hpp>
#include <arch/x86_64/Paging.hpp>

BumpAllocator g_allocator;
PhysicalMemoryManager globalPMM;

extern "C" char _kernel_start;
extern "C" char _kernel_end;

void SwitchPageTable(PageTable* pml4) {
    __asm__ volatile("mov %0, %%cr3" : : "r"(pml4) : "memory");
}

void print_memory_map(BootInfo* b_info) {
    Serial serialOut(Serial::COM1);

    uint64_t entry_count = b_info->mmap_size / b_info->descriptor_size;

    for (uint64_t i = 0; i < entry_count; i++) {
        MemoryDescriptor* desc = (MemoryDescriptor*)((uint8_t*)b_info->mmap_address + (i * b_info->descriptor_size));

        // Print Type as a character (0-9)
        char t = (desc->Type < 10) ? (desc->Type + '0') : '?';
        serialOut.print_char(t);
        serialOut.print(" ");
        
        if (i % 8 == 7) serialOut.print("\n"); // New line every 8 entries
    }
    serialOut.print("\n");
}

PSF1_Header* loadConsoleFont(BootInfo* b_info, Serial serialOut){
    PSF1_Header* font = (PSF1_Header*)b_info->font_address;
    // Check Magic: 0x36 0x04
    if (font->magic[0] != 0x36 || font->magic[1] != 0x04) {
        serialOut.print("Error: Font file corrupted or invalid format.\n");
        while(1);
    }
    return font;
}


void InitializePMM(BootInfo* b_info, uint64_t kernel_pages) {

    // 1. Calculate how much RAM we have
    uint64_t memSize = 0;
    for (uint64_t i = 0; i < b_info->mmap_size; i += b_info->descriptor_size) {
        MemoryDescriptor* desc = (MemoryDescriptor*)((uint64_t)b_info->mmap_address + i);
        memSize += desc->NumberOfPages * 4096;
    }
    globalPMM.TotalMemory = memSize;

    // 2. Find a spot for the bitmap (use your bump allocator or a fixed safe addr)
    // For 4GB RAM, you need ~128KB for the bitmap.
    uint64_t bitmapSize = memSize / 4096 / 8;
    void* bitmapBuffer = g_allocator.alloc(bitmapSize); 
    memset(bitmapBuffer, 0xFF, bitmapSize); // Start by locking EVERYTHING

    globalPMM.PageBitmap.Buffer = (uint8_t*)bitmapBuffer;
    globalPMM.PageBitmap.Size = bitmapSize;

    for (uint64_t i = 0; i < b_info->mmap_size; i += b_info->descriptor_size) {
        MemoryDescriptor* desc = (MemoryDescriptor*)((uint64_t)b_info->mmap_address + i);
        
        if (desc->Type == 7) { // EfiConventionalMemory
            // Cast the physical address to void*
            globalPMM.FreePages((void*)desc->PhysicalStart, desc->NumberOfPages);
        }
    }

    // Do the same for your kernel locking
    globalPMM.LockPages((void*)0x2000000, kernel_pages);
    globalPMM.LockPages((void*)bitmapBuffer, (bitmapSize / 4096) + 1);
}

extern "C" __attribute__((section(".text.start"))) 
void start(BootInfo* b_info) {
    Serial serialOut(Serial::COM1);
    for (constructor* i = &__ctors_start; i < &__ctors_end; i++) {
        (*i)();
    }

    if (b_info->font_address == nullptr) {
        serialOut.print("Error: Font not found in BootInfo!\n");
        while(1);
    }

    PSF1_Header* font = loadConsoleFont(b_info, serialOut);
    ScreenWriter testVGAWriter(b_info->framebuffer, font);

    testVGAWriter.clear(0x000000FF); 

    testVGAWriter.print("This is a test of writing to the screen. \n");
    serialOut.print("K: C++ Entry Reached\n");

    uint64_t mmap_entries = b_info->mmap_size / b_info->descriptor_size;
    MemoryDescriptor* largest_free_block = nullptr;
    uint64_t max_size = 0;

    for (uint64_t i = 0; i < mmap_entries; i++) {
        MemoryDescriptor* desc = (MemoryDescriptor*)((uint8_t*)b_info->mmap_address + (i * b_info->descriptor_size));
        
        // Find the biggest chunk of usable RAM
        if (desc->Type == EfiConventionalMemory) {
            uint64_t size = desc->NumberOfPages * 4096;
            if (size > max_size) {
                max_size = size;
                largest_free_block = desc;
            }
        }
    }

    if (largest_free_block) {
        g_allocator.init(largest_free_block->PhysicalStart, max_size);
        serialOut.print("K: Memory Manager online\n");
    }

    serialOut.print("K: Memory map:\n");
    print_memory_map(b_info);

    uint64_t kernel_size = (uint64_t)&_kernel_end - (uint64_t)&_kernel_start;
    uint64_t kernel_pages = (kernel_size + 4095) / 4096;

    InitializePMM(b_info, kernel_pages);

    PageTable* main_pml4 = (PageTable*)globalPMM.RequestPage();
    memset(main_pml4, 0, 4096);
    PageTableManager manager(main_pml4);

    // Explicitly map the kernel's memory range
    for (uint64_t i = (uint64_t)&_kernel_start; i < (uint64_t)&_kernel_end; i += 4096) {
        manager.MapMemory((void*)i, (void*)i);
    }

    uint64_t fb_base = b_info->framebuffer->BaseAddress;
    uint64_t fb_size = b_info->framebuffer->BufferSize;
    for (uint64_t i = fb_base; i < fb_base + fb_size; i += 4096) {
        manager.MapMemory((void*)i, (void*)i);
    }

    for (uint64_t i = 0; i < 0x100000; i += 4096) {
        manager.MapMemory((void*)i, (void*)i);
    }

    SwitchPageTable(main_pml4);
    serialOut.print("K: Paging Active!\n");


    while(1);
}