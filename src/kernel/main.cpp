#include <stdint.h>
#include <stdbool.h>

#include <memory/memory.hpp>
#include <arch/x86_64/UEFI.h>
#include <arch/x86_64/Serial.hpp>
#include <arch/x86_64/ScreenWriter.hpp>
#include <arch/x86_64/panic.hpp>
#include <arch/x86_64/Paging.hpp>
#include <arch/x86_64/GDT/GDT.h>
#include <arch/x86_64/IDT/IDT.hpp>
#include <globals.hpp>
#include <cpp/cppSupport.hpp>

PhysicalMemoryManager g_PMM;
Serial *g_serialWriter;
ScreenWriter *g_screenwriter;
HeapAllocator g_heap;
IDT *idt;

void SwitchPageTable(PageTable *pml4)
{
    __asm__ volatile("mov %0, %%cr3" : : "r"(pml4) : "memory");
}

void print_hex(uint64_t val)
{
    if (!g_serialWriter)
        return;

    char hex[] = "0123456789ABCDEF";
    char buf[17];
    buf[16] = '\0';
    for (int i = 15; i >= 0; i--)
    {
        buf[i] = hex[val & 0xF];
        val >>= 4;
    }
    g_serialWriter->Print(buf);
}

void print_memory_map(BootInfo *b_info)
{
    uint64_t entry_count = b_info->mmap_size / b_info->descriptor_size;

    g_serialWriter->Print("Memory Map (");
    print_hex(entry_count);
    g_serialWriter->Print(" entries):\n");
}

PSF1_Header *loadConsoleFont(BootInfo *b_info)
{
    PSF1_Header *font = (PSF1_Header *)b_info->font_address;
    if (font->magic[0] != 0x36 || font->magic[1] != 0x04)
    {
        g_serialWriter->Print("Error: Font file corrupted or invalid format.\n");
        while (1)
            ;
    }
    return font;
}

void InitializePMM(BootInfo *b_info, uint64_t kernel_pages, void *bitmap_buffer, uint64_t bitmap_size)
{
    g_serialWriter->Print("K: InitializePMM started\n");

    uint64_t memSize = 0;
    for (uint64_t i = 0; i < b_info->mmap_size; i += b_info->descriptor_size)
    {
        MemoryDescriptor *desc = (MemoryDescriptor *)((uint64_t)b_info->mmap_address + i);
        memSize += desc->NumberOfPages * 4096;
    }
    g_PMM.TotalMemory = memSize;

    memset(bitmap_buffer, 0xFF, bitmap_size);
    g_PMM.PageBitmap.Buffer = (uint8_t *)bitmap_buffer;
    g_PMM.PageBitmap.Size = bitmap_size;

    for (uint64_t i = 0; i < b_info->mmap_size; i += b_info->descriptor_size)
    {
        MemoryDescriptor *desc = (MemoryDescriptor *)((uint64_t)b_info->mmap_address + i);
        if (desc->Type == EfiConventionalMemory)
        {
            g_PMM.FreePages((void *)desc->PhysicalStart, desc->NumberOfPages);
        }
    }

    g_PMM.LockPages((void *)&_kernel_start, kernel_pages);

    uint64_t bitmap_pages = (bitmap_size / 4096) + 1;
    g_PMM.LockPages(bitmap_buffer, bitmap_pages);
}

EXTERNC __attribute__((section(".text.start"))) void start(BootInfo *b_info)
{
    uint64_t cr0;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 &= ~(1UL << 16); // Clear Bit 16 (WP - Write Protect)
    __asm__ volatile("mov %0, %%cr0" : : "r"(cr0));

    Serial earlySerial;
    earlySerial.Init(SERIAL_COM1);
    g_serialWriter = &earlySerial;

    g_serialWriter->Print("K: Kernel Starting...\n");

    if (b_info->font_address == NULL)
    {
        g_serialWriter->Print("Error: No font provided!\n");
        while (1)
            ;
    }
    PSF1_Header *font = loadConsoleFont(b_info);

    uint64_t mmap_entries = b_info->mmap_size / b_info->descriptor_size;
    uint64_t total_mem = 0;

    for (uint64_t i = 0; i < mmap_entries; i++)
    {
        MemoryDescriptor *desc = (MemoryDescriptor *)((uint8_t *)b_info->mmap_address + (i * b_info->descriptor_size));
        total_mem += desc->NumberOfPages * 4096;
    }

    uint64_t bitmap_size = (total_mem / 4096 / 8) + 1;
    uint64_t bitmap_pages = (bitmap_size / 4096) + 1;

    uint64_t kernel_start = (uint64_t)&_kernel_start;
    uint64_t kernel_end = (uint64_t)&_kernel_end;
    uint64_t kernel_size = kernel_end - kernel_start;
    uint64_t kernel_pages = (kernel_size + 4095) / 4096;

    MemoryDescriptor *largest_free_block = NULL;
    uint64_t max_size = 0;

    for (uint64_t i = 0; i < mmap_entries; i++)
    {
        MemoryDescriptor *desc = (MemoryDescriptor *)((uint8_t *)b_info->mmap_address + (i * b_info->descriptor_size));
        if (desc->Type == EfiConventionalMemory)
        {
            uint64_t size = desc->NumberOfPages * 4096;

            if ((desc->PhysicalStart + size < 0x100000000ULL) &&
                !((desc->PhysicalStart < kernel_end) && (desc->PhysicalStart + size > kernel_start)))
            {
                if (size > max_size)
                {
                    max_size = size;
                    largest_free_block = desc;
                }
            }
        }
    }

    if (!largest_free_block)
    {
        g_serialWriter->Print("Error: Not enough memory for Heap!\n");
        while (1)
            ;
    }

    void *bitmap_buffer = (void *)largest_free_block->PhysicalStart;
    uint64_t heap_phys_start = largest_free_block->PhysicalStart + (bitmap_pages * 4096);
    uint64_t heap_size = max_size - (bitmap_pages * 4096);

    g_heap.Init((void *)heap_phys_start, heap_size);
    g_serialWriter->Print("K: Heap Initialized\n");

    InitializePMM(b_info, kernel_pages, bitmap_buffer, bitmap_size);

    g_serialWriter->Print("K: Setting up Page Tables...\n");
    PageTable *main_pml4 = (PageTable *)g_PMM.RequestPage();
    memset(main_pml4, 0, 4096);

    PageTableManager manager;
    manager.Init(main_pml4);

    auto MapRegion = [&](uint64_t base, uint64_t size)
    {
        for (uint64_t i = base; i < base + size; i += 4096)
        {
            manager.MapMemory((void *)i, (void *)i);
        }
    };

    MapRegion(kernel_start, kernel_size);
    MapRegion(b_info->framebuffer.BaseAddress, b_info->framebuffer.BufferSize);
    MapRegion(0, 0x100000);
    MapRegion((uint64_t)bitmap_buffer, bitmap_pages * 4096);
    MapRegion(heap_phys_start, heap_size);

    MapRegion((uint64_t)b_info, sizeof(BootInfo) + 4096);
    if (b_info->font_address)
        MapRegion((uint64_t)b_info->font_address, 4096 * 4);

    uint64_t rsp_val;
    __asm__ volatile("mov %%rsp, %0" : "=r"(rsp_val));
    MapRegion(rsp_val & ~0xFFF, 4096 * 32);

    SwitchPageTable(main_pml4);

    InitializeConstructors();

    g_serialWriter = new Serial();
    g_serialWriter->Init(SERIAL_COM1);

    Framebuffer *fb = (Framebuffer *)&b_info->framebuffer;
    g_screenwriter = new ScreenWriter(fb, font);

    idt = new IDT();

    g_screenwriter->Clear(0x000000FF);
    g_screenwriter->Print("NeoOS Kernel Initialized Successfully!\n");
    g_serialWriter->Print("K: Initialization Complete.\n");
    idt->Init();

    GDT_Init();

    while (1)
    {
        __asm__ volatile("hlt");
    }
}