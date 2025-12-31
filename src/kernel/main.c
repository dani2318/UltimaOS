#include <stdint.h>
#include <stdbool.h>

#include <memory/memory.h>
#include <arch/x86_64/UEFI.h>
#include <arch/x86_64/Serial.h>
#include <arch/x86_64/ScreenWriter.h>
#include <arch/x86_64/panic.h>
#include <arch/x86_64/Paging.h>
#include <arch/x86_64/GDT/GDT.h>
#include <arch/x86_64/IDT/IDT.h>

PhysicalMemoryManager g_PMM;
Serial g_serialWriter;
ScreenWriter g_screenwriter;
BumpAllocator g_allocator;
PSF1_Header *font;

void SwitchPageTable(PageTable *pml4)
{
    __asm__ volatile("mov %0, %%cr3" : : "r"(pml4) : "memory");
}

void print_hex(uint64_t val)
{
    char hex[] = "0123456789ABCDEF";
    char buf[17];
    buf[16] = '\0';
    for (int i = 15; i >= 0; i--)
    {
        buf[i] = hex[val & 0xF];
        val >>= 4;
    }
    Serial_Print(&g_serialWriter, buf);
}

void print_memory_map(BootInfo *b_info)
{
    uint64_t entry_count = b_info->mmap_size / b_info->descriptor_size;

    Serial_Print(&g_serialWriter, "Memory Map (");
    print_hex(entry_count);
    Serial_Print(&g_serialWriter, " entries):\n");
    Serial_Print(&g_serialWriter, "Type Base             Pages\n");

    for (uint64_t i = 0; i < entry_count; i++)
    {
        MemoryDescriptor *desc = (MemoryDescriptor *)((uint8_t *)b_info->mmap_address + (i * b_info->descriptor_size));

        char t = (desc->Type < 10) ? (desc->Type + '0') : '?';

        char buf[2] = {t, '\0'};

        Serial_WriteChar(&g_serialWriter, t);

        Serial_Print(&g_serialWriter, "    ");

        print_hex(desc->PhysicalStart);

        Serial_Print(&g_serialWriter, "  ");

        print_hex(desc->NumberOfPages);

        Serial_Print(&g_serialWriter, "\n");
    }
    Serial_Print(&g_serialWriter, "\n");
}

PSF1_Header *loadConsoleFont(BootInfo *b_info)
{

    Serial_Print(&g_serialWriter, "Font addr: ");
    print_hex((uint64_t)b_info->font_address);
    Serial_Print(&g_serialWriter, "\n");

    PSF1_Header *font = (PSF1_Header *)b_info->font_address;
    Serial_Print(&g_serialWriter, "Font magic: ");
    print_hex(font->magic[0]);
    print_hex(font->magic[1]);
    Serial_Print(&g_serialWriter, "\n");

    if (font->magic[0] != 0x36 || font->magic[1] != 0x04)
    {
        Serial_Print(&g_serialWriter, "Error: Font file corrupted or invalid format.\n");
        while (1)
            ;
    }
    Serial_Print(&g_serialWriter, "K: Font loaded successfully\n");
    return font;
}

void InitializePMM(BootInfo *b_info, uint64_t kernel_pages, void *bitmap_buffer, uint64_t bitmap_size)
{
    Serial_Print(&g_serialWriter, "K: InitializePMM started\n");

    uint64_t memSize = 0;
    for (uint64_t i = 0; i < b_info->mmap_size; i += b_info->descriptor_size)
    {
        MemoryDescriptor *desc = (MemoryDescriptor *)((uint64_t)b_info->mmap_address + i);
        memSize += desc->NumberOfPages * 4096;
    }
    g_PMM.TotalMemory = memSize;

    Serial_Print(&g_serialWriter, "K: Total memory: ");
    print_hex(memSize);
    Serial_Print(&g_serialWriter, " bytes\n");

    memset(bitmap_buffer, 0xFF, bitmap_size);
    g_PMM.PageBitmap.Buffer = (uint8_t *)bitmap_buffer;
    g_PMM.PageBitmap.Size = bitmap_size;

    // Free all conventional memory regions
    uint64_t freed_pages = 0;
    for (uint64_t i = 0; i < b_info->mmap_size; i += b_info->descriptor_size)
    {
        MemoryDescriptor *desc = (MemoryDescriptor *)((uint64_t)b_info->mmap_address + i);
        if (desc->Type == EfiConventionalMemory)
        {
            PMM_FreePages(&g_PMM, (void *)desc->PhysicalStart, desc->NumberOfPages);
            freed_pages += desc->NumberOfPages;
        }
    }

    Serial_Print(&g_serialWriter, "K: Freed pages: ");
    print_hex(freed_pages);
    Serial_Print(&g_serialWriter, "\n");

    // Lock kernel pages
    PMM_LockPages(&g_PMM, (void *)&_kernel_start, kernel_pages);
    Serial_Print(&g_serialWriter, "K: Locked kernel (");
    print_hex(kernel_pages);
    Serial_Print(&g_serialWriter, " pages)\n");

    // Lock bitmap pages
    uint64_t bitmap_pages = (bitmap_size / 4096) + 1;
    PMM_LockPages(&g_PMM, bitmap_buffer, bitmap_pages);
    Serial_Print(&g_serialWriter, "K: Locked bitmap (");
    print_hex(bitmap_pages);
    Serial_Print(&g_serialWriter, " pages)\n");
}

__attribute__((section(".text.start"))) void start(BootInfo *b_info)
{
    Serial_Init(&g_serialWriter, SERIAL_COM1);
    Serial_Print(&g_serialWriter, "K: Kernel Starting...\n");

    if (b_info->font_address == NULL)
    {
        Serial_Print(&g_serialWriter, "Error: No font provided!\n");
        while (1)
            ;
    }

    PSF1_Header *font = loadConsoleFont(b_info);

    Serial_Print(&g_serialWriter, "K: Initializing screen writer...\n");
    screen_writer_init(&g_screenwriter, &b_info->framebuffer, font);
    screen_writer_clear(&g_screenwriter, 0x000000FF);
    Serial_Print(&g_serialWriter, "K: Screen cleared to blue\n");

    print_memory_map(b_info);

    Serial_Print(&g_serialWriter, "K: Calculating memory requirements...\n");

    uint64_t mmap_entries = b_info->mmap_size / b_info->descriptor_size;

    // Calculate total memory size for bitmap calculation
    uint64_t total_mem = 0;
    for (uint64_t i = 0; i < mmap_entries; i++)
    {
        MemoryDescriptor *desc = (MemoryDescriptor *)((uint8_t *)b_info->mmap_address + (i * b_info->descriptor_size));
        total_mem += desc->NumberOfPages * 4096;
    }

    uint64_t bitmap_size = (total_mem / 4096 / 8) + 1;
    uint64_t bitmap_pages = (bitmap_size / 4096) + 1;

    Serial_Print(&g_serialWriter, "K: Bitmap size: ");
    print_hex(bitmap_size);
    Serial_Print(&g_serialWriter, " bytes (");
    print_hex(bitmap_pages);
    Serial_Print(&g_serialWriter, " pages)\n");

    // Get kernel boundaries
    uint64_t kernel_start = (uint64_t)&_kernel_start;
    uint64_t kernel_end = (uint64_t)&_kernel_end;

    // Find largest free block that can fit both bitmap and heap
    // Prefer blocks below 4GB and away from kernel
    MemoryDescriptor *largest_free_block = NULL;
    uint64_t max_size = 0;

    Serial_Print(&g_serialWriter, "K: Kernel at ");
    print_hex(kernel_start);
    Serial_Print(&g_serialWriter, " to ");
    print_hex(kernel_end);
    Serial_Print(&g_serialWriter, "\n");

    Serial_Print(&g_serialWriter, "K: Searching for suitable memory block...\n");
    for (uint64_t i = 0; i < mmap_entries; i++)
    {
        MemoryDescriptor *desc = (MemoryDescriptor *)((uint8_t *)b_info->mmap_address + (i * b_info->descriptor_size));
        if (desc->Type == EfiConventionalMemory)
        {
            uint64_t size = desc->NumberOfPages * 4096;
            uint64_t block_start = desc->PhysicalStart;
            uint64_t block_end = block_start + size;

            Serial_Print(&g_serialWriter, "K:   Found type 7 block: ");
            print_hex(size);
            Serial_Print(&g_serialWriter, " bytes at ");
            print_hex(block_start);

            // Skip blocks that are too small
            uint64_t required_size = bitmap_size + (1024 * 1024);
            if (size < required_size)
            {
                Serial_Print(&g_serialWriter, " [too small]\n");
                continue;
            }

            // Skip blocks above 4GB
            if (block_end > 0x100000000ULL)
            {
                Serial_Print(&g_serialWriter, " [above 4GB]\n");
                continue;
            }

            // Skip blocks that overlap or are too close to kernel (within 16MB)
            bool near_kernel = (block_start < kernel_end + 0x1000000) &&
                               (block_end > kernel_start - 0x1000000);
            if (near_kernel)
            {
                Serial_Print(&g_serialWriter, " [too close to kernel]\n");
                continue;
            }

            Serial_Print(&g_serialWriter, " [suitable]\n");

            if (size > max_size)
            {
                max_size = size;
                largest_free_block = desc;
            }
        }
    }

    if (largest_free_block == NULL)
    {
        Serial_Print(&g_serialWriter, "Error: No suitable memory block found!\n");
        while (1)
            ;
    }

    Serial_Print(&g_serialWriter, "K: Selected block at ");
    print_hex(largest_free_block->PhysicalStart);
    Serial_Print(&g_serialWriter, " size ");
    print_hex(max_size);
    Serial_Print(&g_serialWriter, "\n");

    // Place bitmap at start of largest block
    void *bitmap_buffer = (void *)largest_free_block->PhysicalStart;

    // Place heap after bitmap (aligned)
    uint64_t heap_phys_start = largest_free_block->PhysicalStart + (bitmap_pages * 4096);
    uint64_t heap_size = max_size - (bitmap_pages * 4096);

    Serial_Print(&g_serialWriter, "K: Heap at ");
    print_hex(heap_phys_start);
    Serial_Print(&g_serialWriter, " size ");
    print_hex(heap_size);
    Serial_Print(&g_serialWriter, "\n");

    // Verify the bitmap buffer is accessible before using it
    Serial_Print(&g_serialWriter, "K: Testing bitmap buffer access...\n");
    volatile uint8_t *test_ptr = (volatile uint8_t *)bitmap_buffer;
    *test_ptr = 0xAA; // Write test
    if (*test_ptr != 0xAA)
    {
        Serial_Print(&g_serialWriter, "Error: Bitmap buffer not writable!\n");
        while (1)
            ;
    }
    Serial_Print(&g_serialWriter, "K: Bitmap buffer is accessible\n");

    BumpAllocator_Init(&g_allocator, heap_phys_start, heap_size);
    Serial_Print(&g_serialWriter, "K: Bump allocator initialized\n");

    uint64_t kernel_size = (uint64_t)&_kernel_end - (uint64_t)&_kernel_start;
    uint64_t kernel_pages = (kernel_size + 4095) / 4096;

    Serial_Print(&g_serialWriter, "K: Kernel size: ");
    print_hex(kernel_size);
    Serial_Print(&g_serialWriter, " bytes (");
    print_hex(kernel_pages);
    Serial_Print(&g_serialWriter, " pages)\n");

    InitializePMM(b_info, kernel_pages, bitmap_buffer, bitmap_size);
    Serial_Print(&g_serialWriter, "K: PMM Initialized\n");

    Serial_Print(&g_serialWriter, "K: Requesting PML4 page...\n");
    PageTable *main_pml4 = (PageTable *)PMM_RequestPage(&g_PMM);
    if (main_pml4 == NULL)
    {
        Serial_Print(&g_serialWriter, "Error: Failed to allocate PML4!\n");
        while (1)
            ;
    }
    Serial_Print(&g_serialWriter, "K: PML4 at ");
    print_hex((uint64_t)main_pml4);
    Serial_Print(&g_serialWriter, "\n");

    memset(main_pml4, 0, 4096);

    PageTableManager manager;
    PTM_Init(&manager, main_pml4);
    Serial_Print(&g_serialWriter, "K: Page table manager initialized\n");

    Serial_Print(&g_serialWriter, "K: Mapping kernel...\n");
    // Map kernel
    for (uint64_t i = (uint64_t)&_kernel_start; i < (uint64_t)&_kernel_end; i += 4096)
    {
        PTM_MapMemory(&manager, (void *)i, (void *)i);
    }

    Serial_Print(&g_serialWriter, "K: Mapping framebuffer...\n");
    // Save framebuffer info before switching page tables
    uint64_t fb_base = b_info->framebuffer.BaseAddress;
    uint64_t fb_size = b_info->framebuffer.BufferSize;
    uint32_t fb_width = b_info->framebuffer.Width;
    uint32_t fb_height = b_info->framebuffer.Height;

    Serial_Print(&g_serialWriter, "K: FB Info - Base: ");
    print_hex(fb_base);
    Serial_Print(&g_serialWriter, " Size: ");
    print_hex(fb_size);
    Serial_Print(&g_serialWriter, "\n");

    // Map framebuffer
    for (uint64_t i = fb_base; i < fb_base + fb_size; i += 4096)
    {
        PTM_MapMemory(&manager, (void *)i, (void *)i);
    }

    Serial_Print(&g_serialWriter, "K: Mapping low 1MB...\n");
    // Map low 1MB for legacy devices
    for (uint64_t i = 0; i < 0x100000; i += 4096)
    {
        PTM_MapMemory(&manager, (void *)i, (void *)i);
    }

    Serial_Print(&g_serialWriter, "K: Mapping bitmap...\n");
    // Map bitmap region
    for (uint64_t i = 0; i < bitmap_pages; i++)
    {
        uint64_t addr = (uint64_t)bitmap_buffer + (i * 4096);
        PTM_MapMemory(&manager, (void *)addr, (void *)addr);
    }

    Serial_Print(&g_serialWriter, "K: Mapping heap...\n");
    // Map heap region
    for (uint64_t i = heap_phys_start; i < heap_phys_start + heap_size; i += 4096)
    {
        PTM_MapMemory(&manager, (void *)i, (void *)i);
    }

    // Map memory map itself
    uint64_t mmap_addr = (uint64_t)b_info->mmap_address;
    uint64_t mmap_size = b_info->mmap_size;
    uint64_t mmap_start = mmap_addr & ~0xFFF;
    uint64_t mmap_end = (mmap_addr + mmap_size + 4095) & ~0xFFF;
    for (uint64_t i = mmap_start; i < mmap_end; i += 4096)
    {
        PTM_MapMemory(&manager, (void *)i, (void *)i);
    }

    Serial_Print(&g_serialWriter, "K: Mapping BootInfo structure...\n");
    // Map BootInfo structure itself
    uint64_t bootinfo_addr = (uint64_t)b_info;
    uint64_t bootinfo_start = bootinfo_addr & ~0xFFF;
    uint64_t bootinfo_end = (bootinfo_addr + sizeof(BootInfo) + 4095) & ~0xFFF;
    for (uint64_t i = bootinfo_start; i < bootinfo_end; i += 4096)
    {
        PTM_MapMemory(&manager, (void *)i, (void *)i);
    }

    // Map font data
    Serial_Print(&g_serialWriter, "K: Mapping font data...\n");
    uint64_t font_addr = (uint64_t)b_info->font_address;
    if (font_addr != 0)
    {
        uint64_t font_start = font_addr & ~0xFFF;
        // PSF1 fonts are typically 4KB max
        uint64_t font_end = font_start + 4096;
        for (uint64_t i = font_start; i < font_end; i += 4096)
        {
            PTM_MapMemory(&manager, (void *)i, (void *)i);
        }
    }

    Serial_Print(&g_serialWriter, "K: Mapping stack...\n");
    uint64_t rsp_val;
    __asm__ volatile("mov %%rsp, %0" : "=r"(rsp_val));
    uint64_t stack_base = rsp_val & ~0xFFF;
    Serial_Print(&g_serialWriter, "K: Stack pointer at ");
    print_hex(rsp_val);
    Serial_Print(&g_serialWriter, "\n");

    for (uint64_t i = stack_base - (32 * 4096); i <= stack_base + 4096; i += 4096)
    {
        PTM_MapMemory(&manager, (void *)i, (void *)i);
    }

    Serial_Print(&g_serialWriter, "K: Switching to new page tables...\n");
    SwitchPageTable(main_pml4);
    Serial_Print(&g_serialWriter, "K: Paging Active!\n");

    // Verify we can still write to screen
    Serial_Print(&g_serialWriter, "K: Framebuffer: \n");
    Serial_Print(&g_serialWriter, "K: Saved FB Base: ");
    print_hex(fb_base);
    Serial_Print(&g_serialWriter, "\nK: Saved FB Size: ");
    print_hex(fb_size);
    Serial_Print(&g_serialWriter, "\nK: Saved FB Width: ");
    print_hex(fb_width);
    Serial_Print(&g_serialWriter, "\nK: Saved FB Height: ");
    print_hex(fb_height);
    Serial_Print(&g_serialWriter, "\n");

    screen_writer_clear(&g_screenwriter, 0x000000FF);
    Serial_Print(&g_serialWriter, "K: Screen cleared successfully\n");

    // Test text output
    Serial_Print(&g_serialWriter, "K: Testing text output...\n");
    g_screenwriter.color = 0xFFFFFFFF;

    screen_writer_print(&g_screenwriter, "Hello from NeoOS!\n");

    Serial_Print(&g_serialWriter, "K: Kernel initialization complete!\n");

    // Lock kernel pages
    PMM_LockPages(&g_PMM, (void *)&_kernel_start, kernel_pages);

    GDT_Init();

    while (1){} ;
}