#include <stdint.h>
#include <stdbool.h>

#include <memory/memory.hpp>
#include <libs/core/UEFI.h>
#include <arch/x86_64/Serial.hpp>
#include <arch/x86_64/ScreenWriter.hpp>
#include <arch/x86_64/Paging.hpp>
#include <globals.hpp>
#include <cpp/cppSupport.hpp>
#include <cpp/String.hpp>

#include <arch/x86_64/Interrupts/PIT/PIT.hpp>
#include <arch/x86_64/Drivers/Keyboard.hpp>
#include <arch/x86_64/Drivers/HAL/HAL.hpp>
#include <arch/x86_64/Multitasking/Scheduler.hpp>
#include <arch/x86_64/Interrupts/GDT/GDT.hpp>
#include <arch/x86_64/Interrupts/IDT/IDT.hpp>
#include <arch/x86_64/Filesystem/GPT.hpp>
#include <arch/x86_64/Filesystem/FAT32.hpp>
#include <arch/x86_64/Drivers/ELF/ELFHeaders.hpp>

PageTable *kernelPLM4;

PSF1_Header *LoadConsoleFont(BootInfo *b_info)
{
    PSF1_Header *font = (PSF1_Header *)b_info->font_address;
    if (font->magic[0] != 0x36 || font->magic[1] != 0x04)
    {
        g_serialWriter->Print("ERROR: Invalid font format\n");
        while (1) __asm__ volatile("hlt");
    }
    return font;
}

void InitializePMM(BootInfo *b_info, uint64_t kernel_pages, void *bitmap_buffer, uint64_t bitmap_size)
{
    g_serialWriter->Print("K: Initializing PMM\n");

    // Calculate total memory
    uint64_t total_mem = 0;
    uint64_t mmap_entries = b_info->mmap_size / b_info->descriptor_size;
    
    for (uint64_t i = 0; i < mmap_entries; i++)
    {
        MemoryDescriptor *desc = (MemoryDescriptor *)((uint64_t)b_info->mmap_address + i * b_info->descriptor_size);
        total_mem += desc->NumberOfPages * 4096;
    }
    
    g_PMM.TotalMemory = total_mem;

    // Initialize bitmap (all locked)
    memset(bitmap_buffer, 0xFF, bitmap_size);
    g_PMM.PageBitmap.Buffer = (uint8_t *)bitmap_buffer;
    g_PMM.PageBitmap.Size = bitmap_size;

    // Free usable memory regions
    for (uint64_t i = 0; i < mmap_entries; i++)
    {
        MemoryDescriptor *desc = (MemoryDescriptor *)((uint64_t)b_info->mmap_address + i * b_info->descriptor_size);
        if (desc->Type == EfiConventionalMemory)
        {
            g_PMM.FreePages((void *)desc->PhysicalStart, desc->NumberOfPages);
        }
    }

    // Lock kernel and bitmap
    g_PMM.LockPages((void *)&_kernel_start, kernel_pages);
    g_PMM.LockPages(bitmap_buffer, (bitmap_size + 4095) / 4096);
}

PageTable* CreateUserAddressSpace()
{
    PageTable *user_pml4 = CreatePageTable();
    if (!user_pml4) {
        g_serialWriter->Print("ERROR: Failed to allocate user PML4\n");
        return nullptr;
    }
    
    // Copy kernel mappings (upper half - entries 256-511)
    for (int i = 256; i < 512; i++) {
        user_pml4->entries[i] = kernelPLM4->entries[i];
    }
    
    return user_pml4;
}

void* LoadELFToUserSpace(void *elf_data, size_t elf_size, PageTable *user_pml4, uint64_t *out_entry)
{
    Elf64_Ehdr *ehdr = (Elf64_Ehdr *)elf_data;
    
    // Validate ELF
    if (!IsValidELF(ehdr->e_ident)) {
        g_serialWriter->Print("ERROR: Invalid ELF magic\n");
        return nullptr;
    }
    
    if (!IsElf64(ehdr->e_ident)) {
        g_serialWriter->Print("ERROR: Not 64-bit ELF\n");
        return nullptr;
    }
    
    if (ehdr->e_type != ET_EXEC && ehdr->e_type != ET_DYN) {
        g_serialWriter->Print("ERROR: Not executable ELF\n");
        return nullptr;
    }
    
    // Create page table managers
    PageTableManager user_manager;
    user_manager.Init(user_pml4);
    
    PageTableManager kernel_manager;
    kernel_manager.Init(kernelPLM4);
    
    // Load program headers
    Elf64_Phdr *phdr = (Elf64_Phdr *)((uint8_t *)elf_data + ehdr->e_phoff);
    
    for (int i = 0; i < ehdr->e_phnum; i++) {
        if (phdr[i].p_type != PT_LOAD) continue;
        
        // Calculate addresses
        uint64_t vaddr_start = phdr[i].p_vaddr & ~0xFFF;
        uint64_t vaddr_end = (phdr[i].p_vaddr + phdr[i].p_memsz + 4095) & ~0xFFF;
        uint64_t num_pages = (vaddr_end - vaddr_start) / 4096;
        
        // Allocate and map pages
        for (uint64_t j = 0; j < num_pages; j++) {
            void *phys_page = g_PMM.RequestPage();
            if (!phys_page) {
                g_serialWriter->Print("ERROR: Out of memory loading ELF\n");
                return nullptr;
            }
            
            uint64_t vaddr = vaddr_start + j * 4096;
            
            // Map in user space
            user_manager.MapMemory((void *)vaddr, phys_page, true, true, true);
            
            // Also map in kernel temporarily
            kernel_manager.MapMemory((void *)vaddr, phys_page, false, true, true);
        }
        
        // Copy data
        uint8_t *src = (uint8_t *)elf_data + phdr[i].p_offset;
        uint8_t *dst = (uint8_t *)phdr[i].p_vaddr;
        
        memcpy(dst, src, phdr[i].p_filesz);
        
        // Zero BSS
        if (phdr[i].p_memsz > phdr[i].p_filesz) {
            memset(dst + phdr[i].p_filesz, 0, phdr[i].p_memsz - phdr[i].p_filesz);
        }
        
        // Unmap from kernel
        kernel_manager.UnmapRange((void *)vaddr_start, vaddr_end - vaddr_start);
    }
    
    *out_entry = ehdr->e_entry;
    return (void *)ehdr->e_entry;
}

void* AllocateUserStack(PageTable *user_pml4, size_t stack_size)
{
    const uint64_t USER_STACK_TOP = 0x00007FFFFFFFE000;
    
    PageTableManager user_manager;
    user_manager.Init(user_pml4);
    
    uint64_t num_pages = (stack_size + 4095) / 4096;
    uint64_t stack_bottom = USER_STACK_TOP - (num_pages * 4096);
    
    for (uint64_t i = 0; i < num_pages; i++) {
        void *phys_page = g_PMM.RequestPage();
        if (!phys_page) {
            g_serialWriter->Print("ERROR: Failed to allocate stack\n");
            return nullptr;
        }
        
        uint64_t vaddr = stack_bottom + (i * 4096);
        // Stack: user accessible, writable, non-executable (NX)
        user_manager.MapMemory((void *)vaddr, phys_page, true, true, false);
    }
    
    return (void *)USER_STACK_TOP;
}

EXTERNC __attribute__((section(".text.start"))) 
void start(BootInfo *b_info, EFI_SYSTEM_TABLE *system_table, EFI_HANDLE *ImageHandle)
{

    gImageHandle = ImageHandle;
    g_uefi_system_table = system_table;
    g_hal = new HAL();

    g_hal->DisableInterrupts();
    g_hal->InitializeCPU();

    // Early serial console
    Serial earlySerial;
    earlySerial.Init(SERIAL_COM1);
    g_serialWriter = &earlySerial;
    g_serialWriter->Print("\n=== NeoOS Kernel Starting ===\n");

    // Validate boot info
    if (!b_info->font_address) {
        g_serialWriter->Print("ERROR: No font provided\n");
        while (true) g_hal->Halt();
    }

    PSF1_Header *font = LoadConsoleFont(b_info);

    g_serialWriter->Print("K: Setting up memory management\n");

    // Calculate memory map info
    uint64_t mmap_entries = b_info->mmap_size / b_info->descriptor_size;
    uint64_t total_mem = 0;

    for (uint64_t i = 0; i < mmap_entries; i++) {
        auto *desc = (MemoryDescriptor *)((uint8_t *)b_info->mmap_address + i * b_info->descriptor_size);
        total_mem += desc->NumberOfPages * 4096;
    }

    // Calculate sizes
    uint64_t bitmap_size = (total_mem / 4096 / 8) + 1;
    uint64_t bitmap_pages = (bitmap_size + 4095) / 4096;

    uint64_t kernel_start = (uint64_t)&_kernel_start;
    uint64_t kernel_end = (uint64_t)&_kernel_end;
    uint64_t kernel_size = kernel_end - kernel_start;
    uint64_t kernel_pages = (kernel_size + 4095) / 4096;

    // Find largest usable memory region for heap
    MemoryDescriptor *largest = nullptr;
    uint64_t max_size = 0;

    for (uint64_t i = 0; i < mmap_entries; i++) {
        auto *desc = (MemoryDescriptor *)((uint8_t *)b_info->mmap_address + i * b_info->descriptor_size);

        if (desc->Type != EfiConventionalMemory) continue;
        
        uint64_t size = desc->NumberOfPages * 4096;

        // Skip high memory (above 4GB)
        if (desc->PhysicalStart + size >= 0x100000000ULL) continue;
        
        // Skip kernel region
        if (desc->PhysicalStart < kernel_end && desc->PhysicalStart + size > kernel_start) continue;

        if (size > max_size) {
            max_size = size;
            largest = desc;
        }
    }

    if (!largest) {
        g_serialWriter->Print("ERROR: No usable memory found\n");
        while (true) g_hal->Halt();
    }

    // Setup heap
    void *bitmap_buffer = (void *)largest->PhysicalStart;
    uint64_t heap_phys = largest->PhysicalStart + bitmap_pages * 4096;
    uint64_t heap_size = max_size - bitmap_pages * 4096;

    g_heap.Init((void *)heap_phys, heap_size);
    g_serialWriter->Print("K: Heap initialized\n");

    // Initialize PMM
    InitializePMM(b_info, kernel_pages, bitmap_buffer, bitmap_size);

    g_serialWriter->Print("K: Setting up paging\n");

    // We're still on the bootloader's page table at this point.
    // The bootloader should have identity-mapped physical memory.
    kernelPLM4 = (PageTable *)g_PMM.RequestPage();
    if (!kernelPLM4) {
        g_serialWriter->Print("ERROR: Failed to allocate PML4\n");
        while (true) g_hal->Halt();
    }
    
    g_serialWriter->Print("K: PML4 allocated at 0x");
    g_serialWriter->PrintHex((uint64_t)kernelPLM4);
    g_serialWriter->Print("\n");
    
    // Zero the PML4
    memset(kernelPLM4, 0, PAGE_SIZE);
    
    PageTableManager manager;
    manager.Init(kernelPLM4);

    // Initialize platform (ACPI, APIC, etc.)
    g_hal->InitializePlatform(b_info, manager);

    // Map kernel regions
    manager.MapRange((void *)(kernel_start & ~0xFFF), (void *)(kernel_start & ~0xFFF), 
                     kernel_size, false, true, true);
    
    manager.MapRange((void *)b_info->framebuffer.BaseAddress, 
                     (void *)b_info->framebuffer.BaseAddress, 
                     b_info->framebuffer.BufferSize, false, true, false);
    
    manager.MapRange((void *)0, (void *)0, 0x100000, false, true, true);
    
    manager.MapRange((void *)bitmap_buffer, (void *)bitmap_buffer, 
                     bitmap_pages * 4096, false, true, true);
    
    manager.MapRange((void *)heap_phys, (void *)heap_phys, 
                     heap_size, false, true, true);
    
    manager.MapRange((void *)b_info, (void *)b_info, 
                     (sizeof(BootInfo) + 4095) & ~0xFFF, false, true, true);

    if (b_info->font_address) {
        manager.MapRange((void *)b_info->font_address, (void *)b_info->font_address, 
                         4096 * 4, false, true, true);
    }

    uint64_t rsp;
    asm volatile("mov %%rsp, %0" : "=r"(rsp));
    manager.MapRange((void *)(rsp & ~0xFFF), (void *)(rsp & ~0xFFF), 
                     4096 * 32, false, true, true);

    // Switch to new page table
    g_hal->SwitchPageTable(kernelPLM4);
    InitializeConstructors();

    g_serialWriter = new Serial();
    g_serialWriter->Init(SERIAL_COM1);

    g_screenwriter = new ScreenWriter((Framebuffer *)&b_info->framebuffer, font);
    g_screenwriter->Clear(NORMAL_CLEAR_COLOR);
    g_screenwriter->Print("NeoOS Kernel\n\r");
    g_screenwriter->Print("============\n\r\n\r");

    g_gdt = new GDT();
    g_idt = new IDT();
    g_gdt->Initialize();
    g_idt->Initialize();

    // Keep interrupts disabled during setup
    g_hal->DisableInterrupts();

    // Initialize hardware
    g_screenwriter->Print("Initializing hardware...\n\r");
    Timer::Initialize(1000);  // 1ms tick for CFS
    Keyboard::Initialize();

    // Initialize scheduler
    g_screenwriter->Print("Initializing scheduler...\n\r");
    Scheduler::Initialize();

    // Initialize disk
    g_screenwriter->Print("Initializing storage...\n\r");
    if (!DiskDevice::Initialize()) {
        g_screenwriter->Print("ERROR: Disk init failed\n\r");
        while (true) g_hal->Halt();
    }

    g_screenwriter->Print("Loading filesystem...\n\r");
    
    GPTManager::PartitionInfo partition = GPTManager::GetPartition(0);
    if (!partition.found) {
        g_screenwriter->Print("ERROR: No partition found\n\r");
        while(true) {};
    }

    FAT32Filesystem *fs = new FAT32Filesystem(partition.start_lba);
    if (!fs->IsInitialized()) {
        g_screenwriter->Print("ERROR: FAT32 init failed\n\r");
        while(true) {};
    }

    // Load shell
    g_screenwriter->Print("Loading shell...\n\r");
    
    size_t shell_size = 0;
    void *shell_buffer = fs->ReadFile("PROGRAMS/SHELL.ELF", &shell_size);
    
    if (!shell_buffer || shell_size == 0) {
        g_screenwriter->Print("ERROR: Shell not found\n\r");
        while(true) {};
    }

    // Validate ELF
    uint8_t *elf_magic = (uint8_t *)shell_buffer;
    if (elf_magic[0] != 0x7F || elf_magic[1] != 'E' || 
        elf_magic[2] != 'L' || elf_magic[3] != 'F') {
        g_screenwriter->Print("ERROR: Invalid shell ELF\n\r");
        g_heap.Free(shell_buffer);
        while(true) {};
    }

    // Create user address space
    PageTable *shell_pml4 = CreateUserAddressSpace();
    if (!shell_pml4) {
        g_screenwriter->Print("ERROR: Failed to create user space\n\r");
        g_heap.Free(shell_buffer);
        while(true) {};
    }

    // Load ELF
    uint64_t shell_entry = 0;
    void *entry_point = LoadELFToUserSpace(shell_buffer, shell_size, shell_pml4, &shell_entry);
    g_heap.Free(shell_buffer);
    
    if (!entry_point) {
        g_screenwriter->Print("ERROR: Failed to load shell ELF\n\r");
        while(true) {};
    }

    // Allocate stack
    void *shell_stack = AllocateUserStack(shell_pml4, 1024 * 1024);
    if (!shell_stack) {
        g_screenwriter->Print("ERROR: Failed to allocate shell stack\n\r");
        while(true) {};
    }


    //! DISABLED SHELL CODE SINCE: it does a Page fault exception at that point with 02 as err_code and 0E as int_no
    //TODO: Fix crash.
    //? Note that: The codebase works with the task creation disabled

    // Create shell task
    // Task *shell_task = Scheduler::CreateTask(
    //     (void(*)())shell_entry,
    //     "shell",
    //     -5,  // High priority for responsiveness
    //     shell_pml4,
    //     shell_stack
    // );
    
    // if (!shell_task) {
    //     g_screenwriter->Print("ERROR: Failed to create shell task\n\r");
    //     while(true) {};
    // }

    // g_screenwriter->Print("Shell loaded successfully\n\r");

    g_screenwriter->Clear(NORMAL_CLEAR_COLOR);
    g_screenwriter->Print("=========================\n\r");
    g_screenwriter->Print("   NeoOS Ready!\n\r");
    g_screenwriter->Print("=========================\n\r");
    g_screenwriter->Print("Scheduler: CFS\n\r");
    g_screenwriter->Print("Multitasking: Enabled\n\r");
    g_screenwriter->Print("=========================\n\r\n\r");
    
    g_serialWriter->Print("\nK: Enabling interrupts\n");
    g_hal->EnableInterrupts();
    
    g_serialWriter->Print("K: Entering idle loop\n\n");

    // Idle loop
    while (true) {
        g_hal->Halt();
    }
}