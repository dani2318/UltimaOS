#include <stdint.h>
#include <stdbool.h>

#include <memory/memory.hpp>
#include <libs/core/UEFI.h>
#include <arch/x86_64/Serial.hpp>
#include <arch/x86_64/Syscall.hpp>
#include <arch/x86_64/ScreenWriter.hpp>
#include <arch/x86_64/panic.hpp>
#include <arch/x86_64/Paging.hpp>
#include <globals.hpp>
#include <cpp/cppSupport.hpp>
#include <cpp/String.hpp>

#include <arch/x86_64/Interrupts/PIT/PIT.hpp>
#include <arch/x86_64/Drivers/Keyboard.hpp>
#include <arch/x86_64/Drivers/ACPI/ACPI.hpp>
#include <arch/x86_64/Drivers/HAL/HAL.hpp>
#include <arch/x86_64/Drivers/ELF/ELFLoader.hpp>
#include <arch/x86_64/Filesystem/Filesystem.hpp>
#include <arch/x86_64/Multitasking/Scheduler.hpp>

#include <arch/x86_64/Interrupts/GDT/GDT.hpp>
#include <arch/x86_64/Interrupts/IDT/IDT.hpp>
#include "arch/x86_64/Filesystem/GPT.hpp"
#include "arch/x86_64/Filesystem/FAT32.hpp"
#include <arch/x86_64/Drivers/ELF/ELFHeaders.hpp>

struct KernelAPI
{
    void (*Print)(const char *);
    char (*GetChar)();
    void (*Exit)();
    void (*Clear)(uint32_t);
};

// Static wrappers to match the function pointer signature
void api_print(const char *s) { g_screenwriter->Print(s); }
char api_get_char() { return Keyboard::HasChar() ? Keyboard::GetChar() : 0; }
void api_clear(uint32_t col)
{
    g_screenwriter->Clear(col);
    g_screenwriter->SetCursor(0, 0);
}

PageTable *kernelPLM4;

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
        {
            __asm__ volatile("hlt");
        }
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
    memset(b_info->mmap_address, 0, bitmap_size);

    g_PMM.LockPages((void *)&_kernel_start, kernel_pages);

    uint64_t bitmap_pages = (bitmap_size + 4095) / 4096;
    g_PMM.LockPages(bitmap_buffer, bitmap_pages);
}

void ProcessCommand(const char *cmd)
{
    if (strcmp(cmd, "help") == 0)
    {
        g_screenwriter->Print("Available commands:\n\r");
        g_screenwriter->Print("  help - Show this help\n\r");
        g_screenwriter->Print("  clear - Clear screen\n\r");
        g_screenwriter->Print("  uptime - Show system uptime\n\r");
        g_screenwriter->Print("  shutdown - Shutdown the system\n\r");
        g_screenwriter->Print("  reboot - Reboot the system\n\r");
        return;
    }
    else if (strcmp(cmd, "clear") == 0)
    {
        g_screenwriter->Clear(NORMAL_CLEAR_COLOR);
        g_screenwriter->SetCursor(0, 0);
        return;
    }
    else if (strcmp(cmd, "uptime") == 0)
    {
        g_screenwriter->Uptime();
        g_screenwriter->Print("\n\r");
        return;
    }
    if (strcmp(cmd, "shutdown") == 0)
    {
        g_screenwriter->Print("Shutting down...\n\r");
        g_acpi->Shutdown();
        return;
    }
    if (strcmp(cmd, "reboot") == 0)
    {
        g_screenwriter->Print("Shutting down...\n\r");
        g_acpi->Reboot();
        return;
    }
    else
    {
        g_screenwriter->Print("Unknown command: ");
        g_screenwriter->Print(cmd);
        g_screenwriter->Print("\n\r");
        return;
    }
}

void shell_task()
{
    char command_buffer[256];
    int cmd_index = 0;
    g_screenwriter->Print("> ");

    while (true) // 1. Task must stay in an infinite loop
    {
        if (Keyboard::HasChar())
        {
            char c = Keyboard::GetChar();

            if (c == '\n')
            {
                g_screenwriter->Print("\n\r");
                command_buffer[cmd_index] = '\0';

                if (cmd_index > 0)
                {
                    ProcessCommand(command_buffer);
                }

                cmd_index = 0;
                g_screenwriter->Print("> ");
            }
            else if (c == '\b')
            {
                if (cmd_index > 0)
                {
                    cmd_index--;
                    g_screenwriter->Backspace();
                }
            }
            else if (cmd_index < 255)
            {
                command_buffer[cmd_index++] = c;
                char str[2] = {c, '\0'};
                g_screenwriter->Print(str);
            }
        }
        else
        {
            // 2. No key? Give up the CPU so other tasks can work
            Scheduler::Yield();
        }
    }
}



EXTERNC __attribute__((section(".text.start"))) void start(BootInfo *b_info, EFI_SYSTEM_TABLE *system_table, EFI_HANDLE *ImageHandle)
{
    gImageHandle = ImageHandle;
    g_uefi_system_table = system_table;
    g_hal = new HAL();

    // =====================================================
    // Early CPU + Interrupt State
    // =====================================================
    g_hal->DisableInterrupts();
    g_hal->InitializeCPU();

    // =====================================================
    // Early Serial Output
    // =====================================================
    Serial earlySerial;
    earlySerial.Init(SERIAL_COM1);
    g_serialWriter = &earlySerial;
    g_serialWriter->Print("K: Kernel Starting...\n");

    // =====================================================
    // Font Validation
    // =====================================================
    if (!b_info->font_address)
    {
        g_serialWriter->Print("Error: No font provided!\n");
        while (true)
            g_hal->Halt();
    }

    PSF1_Header *font = loadConsoleFont(b_info);

    // =====================================================
    // Memory Map Processing
    // =====================================================
    uint64_t mmap_entries = b_info->mmap_size / b_info->descriptor_size;
    uint64_t total_mem = 0;

    for (uint64_t i = 0; i < mmap_entries; i++)
    {
        auto *desc = (MemoryDescriptor *)((uint8_t *)b_info->mmap_address + i * b_info->descriptor_size);
        total_mem += desc->NumberOfPages * 4096;
    }

    uint64_t bitmap_size = (total_mem / 4096 / 8) + 1;
    uint64_t bitmap_pages = (bitmap_size + 4095) / 4096;

    uint64_t kernel_start = (uint64_t)&_kernel_start;
    uint64_t kernel_end = (uint64_t)&_kernel_end;
    uint64_t kernel_size = kernel_end - kernel_start;
    uint64_t kernel_pages = (kernel_size + 4095) / 4096;

    // =====================================================
    // Locate Largest Usable Memory Block
    // =====================================================
    MemoryDescriptor *largest = nullptr;
    uint64_t max_size = 0;

    for (uint64_t i = 0; i < mmap_entries; i++)
    {
        auto *desc = (MemoryDescriptor *)((uint8_t *)b_info->mmap_address + i * b_info->descriptor_size);

        if (desc->Type != EfiConventionalMemory)
            continue;

        uint64_t size = desc->NumberOfPages * 4096;

        if (desc->PhysicalStart + size >= 0x100000000ULL)
            continue;

        if (desc->PhysicalStart < kernel_end && desc->PhysicalStart + size > kernel_start)
            continue;

        if (size > max_size)
        {
            max_size = size;
            largest = desc;
        }
    }

    if (!largest)
    {
        g_serialWriter->Print("Error: No heap memory!\n");
        while (true)
            g_hal->Halt();
    }

    // =====================================================
    // Heap + Physical Memory Manager
    // =====================================================
    void *bitmap_buffer = (void *)largest->PhysicalStart;
    uint64_t heap_phys = largest->PhysicalStart + bitmap_pages * 4096;
    uint64_t heap_size = max_size - bitmap_pages * 4096;

    g_heap.Init((void *)heap_phys, heap_size);
    g_serialWriter->Print("K: Heap Initialized\n");

    InitializePMM(b_info, kernel_pages, bitmap_buffer, bitmap_size);

    // =====================================================
    // Paging Setup
    // =====================================================
    kernelPLM4 = (PageTable *)g_PMM.RequestPage();
    memset(kernelPLM4, 0, 4096);

    PageTableManager manager;
    manager.Init(kernelPLM4);

    // HAL handles ACPI + PIC + LAPIC
    g_hal->InitializePlatform(b_info, manager);

    auto MapRegion = [&](uint64_t base, uint64_t size)
    {
        size = (size + 4095) & ~0xFFFULL;
        for (uint64_t addr = base & ~0xFFFULL; addr < (base & ~0xFFFULL) + size; addr += 4096)
        {
            manager.MapMemory((void *)addr, (void *)addr);
        }
    };

    MapRegion(kernel_start, kernel_size);
    MapRegion(b_info->framebuffer.BaseAddress, b_info->framebuffer.BufferSize);
    MapRegion(0, 0x100000);
    MapRegion((uint64_t)bitmap_buffer, bitmap_pages * 4096);
    MapRegion(heap_phys, heap_size);
    MapRegion((uint64_t)b_info, (sizeof(BootInfo) + 4095) & ~0xFFF);

    if (b_info->font_address)
        MapRegion((uint64_t)b_info->font_address, 4096 * 4);

    uint64_t rsp;
    asm volatile("mov %%rsp, %0" : "=r"(rsp));
    MapRegion(rsp & ~0xFFF, 4096 * 32);

    // =====================================================
    // Enable Paging + Constructors
    // =====================================================
    g_hal->SwitchPageTable(kernelPLM4);
    InitializeConstructors();

    // =====================================================
    // Late Kernel Subsystems
    // =====================================================
    g_serialWriter = new Serial();
    g_serialWriter->Init(SERIAL_COM1);

    g_screenwriter = new ScreenWriter((Framebuffer *)&b_info->framebuffer, font);
    g_screenwriter->Clear(NORMAL_CLEAR_COLOR);
    g_screenwriter->Print("NeoOS Kernel Initialized Successfully!\n\r");

    g_gdt = new GDT();
    g_idt = new IDT();
    g_gdt->Initialize();
    g_idt->Initialize();

    // ============ FIX #3: Proper initialization order ============
    // Make absolutely sure interrupts are disabled during setup
    g_hal->DisableInterrupts();

    // Initialize timer (but interrupts are still disabled)
    Timer::Initialize(100);
    Keyboard::Initialize();

    // Initialize scheduler
    Scheduler::Initialize();

    g_screenwriter->Clear(NORMAL_CLEAR_COLOR);
    g_serialWriter->Print("K: Scheduler initialized\n");

    // Initialize syscalls
    Syscall::Initialize();
    g_serialWriter->Print("K: Syscalls initialized\n");

    // NOW it's safe to enable interrupts
    g_hal->EnableInterrupts();
    g_serialWriter->Print("K: Interrupts enabled\n");
    // ============ END OF FIX #3 ============

    // =====================================================
    // Disk Initialization
    // =====================================================
    g_screenwriter->Print("Initializing disk...\n\r");
    if (!DiskDevice::Initialize())
    {
        g_screenwriter->Print("ERROR: Failed to initialize disk\n\r");
        g_hal->EnableInterrupts();
        while (true)
            g_hal->Halt();
    }
    g_serialWriter->Print("K: Disk ready\n");

    // =====================================================
    // Find and Read Shell from Disk
    // =====================================================
    g_screenwriter->Print("Looking for partition...\n\r");
    GPTManager::PartitionInfo esp = GPTManager::GetPartition(0);

    if (!esp.found)
    {
        g_screenwriter->Print("ERROR: Partition not found!\n\r");
        g_hal->EnableInterrupts();
        while (true)
            g_hal->Halt();
    }

    g_screenwriter->Print("Partition found at LBA: ");
    g_screenwriter->PrintHex(esp.start_lba);
    g_screenwriter->Print("\n\r");




    FAT32Filesystem fs(esp.start_lba);
    g_serialWriter->Print("K: FAT32 ready\n");

    fs.ListRootDirectory();

    g_screenwriter->Print("Reading SHELL.ELF...\n\r");
    size_t shellSize = 0;
    asm volatile("cli");
    char shellPath[] = "PROGRAMS/SHELL.ELF";
    void *shellBuffer = fs.ReadFile(shellPath, &shellSize);
    asm volatile("sti");

    if (!shellBuffer || shellSize == 0)
    {
        g_screenwriter->Print("ERROR: Failed to load shell\n\r");
        g_hal->EnableInterrupts();
        while (true)
            g_hal->Halt();
    }

    g_screenwriter->Print("Loaded ");
    g_screenwriter->PrintHex(shellSize);
    g_screenwriter->Print(" bytes\n\r");

    // Validate ELF magic
    uint8_t *elf_magic = (uint8_t *)shellBuffer;
    if (elf_magic[0] != 0x7F || elf_magic[1] != 'E' || elf_magic[2] != 'L' || elf_magic[3] != 'F')
    {
        g_screenwriter->Print("ERROR: Invalid ELF file\n\r");
        g_heap.Free(shellBuffer);
        g_hal->EnableInterrupts();
        while (true)
            g_hal->Halt();
    }

    // =====================================================
    // Create Shell Process
    // =====================================================
    g_screenwriter->Print("Creating shell process...\n\r");

    // Create page table for shell
    PageTable *shellPML4 = (PageTable *)g_PMM.RequestPage();
    memset(shellPML4, 0, 4096);

    // Copy kernel mappings (upper half)
    for (int i = 256; i < 512; i++)
    {
        shellPML4->entries[i] = kernelPLM4->entries[i];
    }

    PageTableManager shellManager;
    shellManager.Init(shellPML4);

    // Map critical kernel structures into shell's page table
    g_serialWriter->Print("K: Mapping kernel structures\n");

    // Map the page table itself
    shellManager.MapMemory((void *)shellPML4, (void *)shellPML4, false);
    shellManager.MapMemory((void*)0xFEE00000, (void*)0xFEE00000, false);

    // Map framebuffer
    uint64_t fb_base = b_info->framebuffer.BaseAddress;
    uint64_t fb_size = b_info->framebuffer.BufferSize;
    for (uint64_t addr = fb_base; addr < fb_base + fb_size; addr += 4096)
    {
        shellManager.MapMemory((void *)addr, (void *)addr, false);
    }

    // Map kernel code
    for (uint64_t addr = kernel_start; addr < kernel_end; addr += 4096)
    {
        shellManager.MapMemory((void *)addr, (void *)addr, false);
    }

    // ============ FIX #2: Map TSS and kernel structures ============
    // Map the TSS (required for syscalls)
    shellManager.MapMemory((void*)&g_tss, (void*)&g_tss, false);
    
    // Map kernel globals that syscalls will use (map entire pages)
    uint64_t sw_addr = (uint64_t)g_screenwriter;
    shellManager.MapMemory((void*)(sw_addr & ~0xFFFULL), (void*)(sw_addr & ~0xFFFULL), false);
    
    uint64_t ser_addr = (uint64_t)g_serialWriter;
    shellManager.MapMemory((void*)(ser_addr & ~0xFFFULL), (void*)(ser_addr & ~0xFFFULL), false);
    
    // Map heap area (keyboard uses heap allocations)
    for (uint64_t addr = heap_phys; addr < heap_phys + heap_size; addr += 4096)
    {
        shellManager.MapMemory((void*)addr, (void*)addr, false);
    }
    
    g_serialWriter->Print("K: Mapped TSS and kernel data structures\n");
    // ============ END OF FIX #2 ============

    // Allocate and map user stack
    const uint64_t USER_STACK_BASE = 0x0000700000000000;
    const size_t STACK_PAGES = 4;

    void *userStackPhys = g_PMM.RequestPages(STACK_PAGES);
    if (!userStackPhys)
    {
        g_screenwriter->Print("ERROR: Stack allocation failed\n\r");
        g_PMM.FreePage(shellPML4);
        g_heap.Free(shellBuffer);
        g_hal->EnableInterrupts();
        while (true)
            g_hal->Halt();
    }

    for (size_t i = 0; i < STACK_PAGES; i++)
    {
        void *virtAddr = (void *)(USER_STACK_BASE + i * 4096);
        void *physAddr = (void *)((uint64_t)userStackPhys + i * 4096);
        shellManager.MapMemory(virtAddr, physAddr, true);
    }

    uint64_t userStackTop = USER_STACK_BASE + (STACK_PAGES * 4096);

    // Load ELF into shell's address space
    g_screenwriter->Print("Loading ELF...\n\r");
    ELFDriver loader;

    if (!loader.LoadFromBuffer(shellBuffer, shellSize, shellManager))
    {
        g_screenwriter->Print("ERROR: ELF loading failed\n\r");
        for (size_t i = 0; i < STACK_PAGES; i++)
        {
            g_PMM.FreePage((void *)((uint64_t)userStackPhys + i * 4096));
        }
        g_PMM.FreePage(shellPML4);
        g_heap.Free(shellBuffer);
        g_hal->EnableInterrupts();
        while (true)
            g_hal->Halt();
    }

    g_screenwriter->Print("Entry point: ");
    g_screenwriter->PrintHex(loader.GetEntryPoint());
    g_screenwriter->Print("\n\r");

    // ============ FIX #1: Map shell segments ============
    g_screenwriter->Print("Mapping shell segments...\n\r");
    
    // Get the program headers and verify mappings
    ELF64_Ehdr* ehdr = (ELF64_Ehdr*)shellBuffer;
    ELF64_Phdr* phdr = (ELF64_Phdr*)((uint8_t*)shellBuffer + ehdr->e_phoff);
    
    for (int i = 0; i < ehdr->e_phnum; i++) {
        if (phdr[i].p_type == PT_LOAD) {
            uint64_t vaddr = phdr[i].p_vaddr;
            uint64_t memsz = phdr[i].p_memsz;
            
            // Align to page boundaries
            uint64_t vaddr_aligned = vaddr & ~0xFFFULL;
            uint64_t memsz_aligned = (memsz + (vaddr & 0xFFF) + 0xFFF) & ~0xFFFULL;
            
            g_screenwriter->Print("  Segment at 0x");
            g_screenwriter->PrintHex(vaddr_aligned);
            g_screenwriter->Print(" size 0x");
            g_screenwriter->PrintHex(memsz_aligned);
            g_screenwriter->Print("\n\r");
        }
    }
    
    g_serialWriter->Print("K: Shell segments mapped\n");
    // ============ END OF FIX #1 ============

    // Create shell task
    Scheduler::CreateTask(
        (void (*)())loader.GetEntryPoint(),
        "shell",
        1,
        shellPML4,
        (void *)userStackTop);

    g_screenwriter->Print("Shell task created\n\r");
    g_serialWriter->Print("K: Shell task ready\n");

    // Free the ELF buffer (already loaded into memory)
    g_heap.Free(shellBuffer);

    // =====================================================
    // Enter Kernel Idle Loop
    // =====================================================
    g_screenwriter->Print("System ready!\n\r");
    g_serialWriter->Print("K: Entering idle loop\n");

    g_hal->EnableInterrupts();

    while (true)
        g_hal->Halt();
}