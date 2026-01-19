#pragma once
#include <stdint.h>
#include <stddef.h>
#include "Drivers/HAL/HAL.hpp"
#include <Drivers/ELF/ELFHeaders.hpp>
#include <arch/x86_64/Paging/Paging.hpp>
#include <arch/x86_64/Serial.hpp>
#include <memory/memory.hpp>
#include <Drivers/Storage/Filesystem/Filesystem.hpp>
#include <cpp/String.hpp>
#include <arch/x86_64/ScreenWriter.hpp>

#define PT_LOAD 1
#define PT_DYNAMIC 2
#define PF_X 1
#define PF_W 2
#define PF_R 4

#define R_X86_64_64 1

#pragma once
#include <stdint.h>
#include <stddef.h>
#include <Drivers/Storage/Filesystem/GPT.hpp>
#include <Drivers/Storage/Filesystem/FAT32.hpp>
#include <Drivers/ELF/ELFHeaders.hpp>
#include <arch/x86_64/Paging/Paging.hpp>
#include <memory/memory.hpp>
#include <globals.hpp>

class ELFLoader
{
private:
    FAT32Filesystem *fat32;
    PageTableManager *ptm;

    // Step 1: Initialize disk access (independent)
    bool InitializeDisk()
    {
        gScreenwriter->print("Step 1: Initializing disk...\n", true);

        if (!DiskDevice::Initialize())
        {
            gScreenwriter->print("ERROR: Disk initialization failed\n", true);
            return false;
        }

        gScreenwriter->print("Disk ready\n", true);
        return true;
    }

    // Step 2: Read GPT and find FAT32 partition (independent of Step 1 result)
    bool FindFAT32Partition(uint64_t *out_lba)
    {
        gScreenwriter->print("Step 2: Finding FAT32 partition...\n", true);

        // Try partition index 0 first (usually the data partition)
        GPTManager::PartitionInfo part = GPTManager::GetPartition(0);

        if (!part.found)
        {
            gScreenwriter->print("ERROR: Partition 0 not found\n", true);
            return false;
        }

        *out_lba = part.start_lba;

        char buf[32];
        gScreenwriter->print("Found partition at LBA: ", true);
        gScreenwriter->print(itoa(*out_lba, buf, 10), true);
        gScreenwriter->print("\n", true);

        return true;
    }

    // Step 3: Mount FAT32 filesystem (independent)
    bool MountFAT32(uint64_t partition_lba)
    {
        gScreenwriter->print("Step 3: Mounting FAT32...\n", true);

        fat32 = new FAT32Filesystem(partition_lba);

        if (!fat32)
        {
            gScreenwriter->print("ERROR: Failed to create FAT32 instance\n", true);
            return false;
        }

        gScreenwriter->print("FAT32 mounted\n", true);
        return true;
    }

    // Step 4: Read ELF file into memory (independent)
    bool ReadELFFile(const char *filepath, void **out_data, size_t *out_size)
    {
        gScreenwriter->print("Step 4: Reading ELF file: ", true);
        gScreenwriter->print(filepath, true);
        gScreenwriter->print("\n", true);

        uint32_t file_size_32 = 0; // Temporary 32-bit variable
        void *data = fat32->ReadFile(filepath, &file_size_32);

        if (data != nullptr && out_size != nullptr)
        {
            *out_size = (size_t)file_size_32; // Convert back to size_t
        }

        char buf[32];
        gScreenwriter->print("File read successfully, size: ", true);
        gScreenwriter->print(itoa(*out_size, buf, 10), true);
        gScreenwriter->print(" bytes\n", true);

        *out_data = data;
        return true;
    }

    // Step 5: Validate ELF header (independent)
    bool ValidateELF(void *elf_data)
    {
        gScreenwriter->print("Step 5: Validating ELF header...\n", true);

        ELF64_Ehdr *ehdr = (ELF64_Ehdr *)elf_data;

        // Check magic number
        if (memcmp(ehdr->e_ident, "\x7F"
                                  "ELF",
                   4) != 0)
        {
            gScreenwriter->print("ERROR: Invalid ELF magic\n", true);
            return false;
        }

        // Check 64-bit
        if (ehdr->e_ident[4] != 2) // ELFCLASS64
        {
            gScreenwriter->print("ERROR: Not a 64-bit ELF\n", true);
            return false;
        }

        // Check x86-64
        if (ehdr->e_machine != 0x3E) // EM_X86_64
        {
            gScreenwriter->print("ERROR: Not an x86-64 ELF\n", true);
            return false;
        }

        char buf[32];
        gScreenwriter->print("Valid ELF64, entry point: 0x", true);
        gScreenwriter->printHex(ehdr->e_entry, true);
        gScreenwriter->print("\n", true);

        return true;
    }

    bool CalculateMemoryLayout(void *elf_data, uint64_t *out_base, uint64_t *out_size)
    {
        ELF64_Ehdr *ehdr = (ELF64_Ehdr *)elf_data;
        ELF64_Phdr *phdrs = (ELF64_Phdr *)((uint8_t *)elf_data + ehdr->e_phoff);

        uint64_t lowest = 0xFFFFFFFFFFFFFFFF;
        uint64_t highest = 0;
        bool found_loadable = false;

        for (uint16_t i = 0; i < ehdr->e_phnum; i++)
        {
            if (phdrs[i].p_type != 1) continue; // PT_LOAD

            found_loadable = true;
            if (phdrs[i].p_vaddr < lowest) lowest = phdrs[i].p_vaddr;
            if (phdrs[i].p_vaddr + phdrs[i].p_memsz > highest) highest = phdrs[i].p_vaddr + phdrs[i].p_memsz;
        }

        if (!found_loadable) return false;

        *out_base = lowest;
        *out_size = highest - lowest;
        return true;
    }



    // Step 7: Allocate memory (independent)
    bool AllocateMemory(uint64_t size, void **out_mem)
    {
        gScreenwriter->print("Step 7: Allocating memory...\n", true);

        void *mem = gHeap.alloc(size);

        if (!mem)
        {
            gScreenwriter->print("ERROR: Memory allocation failed\n", true);
            return false;
        }

        // Zero the memory
        memset(mem, 0, size);

        gScreenwriter->print("Memory allocated at: 0x", true);
        gScreenwriter->printHex((uint64_t)mem, true);
        gScreenwriter->print("\n", true);

        *out_mem = mem;
        return true;
    }

    // Step 8: Load segments into memory (independent)
    bool LoadSegments(void *elf_data, void *base_mem, uint64_t base_vaddr)
    {
        gScreenwriter->print("Step 8: Loading segments...\n", true);

        ELF64_Ehdr *ehdr = (ELF64_Ehdr *)elf_data;
        ELF64_Phdr *phdrs = (ELF64_Phdr *)((uint8_t *)elf_data + ehdr->e_phoff);

        for (uint16_t i = 0; i < ehdr->e_phnum; i++)
        {
            ELF64_Phdr *ph = &phdrs[i];

            if (ph->p_type != PT_LOAD)
                continue;

            // Calculate destination in allocated memory
            void *dest = (uint8_t *)base_mem + (ph->p_vaddr - base_vaddr);
            uint8_t* check = (uint8_t*)dest;
            gScreenwriter->print("First byte of segment: ", true);
            gScreenwriter->printHex(check[0], true);
            // Copy file content
            memcpy(dest, (uint8_t *)elf_data + ph->p_offset, ph->p_filesz);

            // Zero BSS if needed
            if (ph->p_memsz > ph->p_filesz)
            {
                memset((uint8_t *)dest + ph->p_filesz, 0, ph->p_memsz - ph->p_filesz);
            }

            char buf[32];
            gScreenwriter->print("  Segment ", true);
            gScreenwriter->print(itoa(i, buf, 10), true);
            gScreenwriter->print(": vaddr=0x", true);
            gScreenwriter->printHex(ph->p_vaddr, true);
            gScreenwriter->print(" filesz=", true);
            gScreenwriter->print(itoa(ph->p_filesz, buf, 10), true);
            gScreenwriter->print(" memsz=", true);
            gScreenwriter->print(itoa(ph->p_memsz, buf, 10), true);
            gScreenwriter->print("\n", true);
        }

        return true;
    }

    // Step 9: Map pages with correct permissions (independent)
    bool MapPages(void *elf_data, void *base_mem, uint64_t base_vaddr)
    {
        gScreenwriter->print("Step 9: Mapping pages...\n", true);

        ELF64_Ehdr *ehdr = (ELF64_Ehdr *)elf_data;
        ELF64_Phdr *phdrs = (ELF64_Phdr *)((uint8_t *)elf_data + ehdr->e_phoff);

        for (uint16_t i = 0; i < ehdr->e_phnum; i++)
        {
            ELF64_Phdr *ph = &phdrs[i];

            if (ph->p_type != PT_LOAD)
                continue;

            // Determine permissions
            bool writable = (ph->p_flags & 0x2) != 0;   // PF_W
            bool executable = (ph->p_flags & 0x1) != 0; // PF_X

            // Calculate physical address in our allocated memory
            uint64_t phys_start = (uint64_t)base_mem + (ph->p_vaddr - base_vaddr);

            // Align to page boundaries
            uint64_t virt_start = ph->p_vaddr & ~0xFFFULL;
            uint64_t virt_end = (ph->p_vaddr + ph->p_memsz + 0xFFF) & ~0xFFFULL;

            // Map each page
            for (uint64_t virt = virt_start; virt < virt_end; virt += 4096)
            {
                uint64_t page_offset = virt - virt_start;
                uint64_t phys = (phys_start - (ph->p_vaddr & 0xFFF)) + page_offset;

                ptm->mapPage(virt, phys, writable, false, executable);
            }

            char buf[32];
            gScreenwriter->print("  Mapped segment ", true);
            gScreenwriter->print(itoa(i, buf, 10), true);
            gScreenwriter->print(": ", true);
            gScreenwriter->print(writable ? "RW" : "R-", true);
            gScreenwriter->print(executable ? "X" : "-", true);
            gScreenwriter->print("\n", true);
        }

        return true;
    }

    // Step 10: Calculate final entry point (independent)
// Step 10: Calculate final entry point (independent)
uint64_t CalculateEntryPoint(void *elf_data, void *base_mem, uint64_t base_vaddr)
{
    gScreenwriter->print("Step 10: Calculating entry point...\n", true);

    ELF64_Ehdr *ehdr = (ELF64_Ehdr *)elf_data;

    // Entry point is a virtual address in the ELF
    uint64_t entry_vaddr = ehdr->e_entry;

    gScreenwriter->print("ELF e_entry:     0x", true);
    gScreenwriter->printHex(entry_vaddr, true);
    gScreenwriter->print("\n", true);

    gScreenwriter->print("Base vaddr:      0x", true);
    gScreenwriter->printHex(base_vaddr, true);
    gScreenwriter->print("\n", true);

    gScreenwriter->print("Base mem:        0x", true);
    gScreenwriter->printHex((uint64_t)base_mem, true);
    gScreenwriter->print("\n", true);

    // Validate entry point is within a loaded segment
    ELF64_Phdr *phdr = (ELF64_Phdr *)((uint8_t *)elf_data + ehdr->e_phoff);
    bool entry_valid = false;
    uint64_t entry_actual = 0;

    for (int i = 0; i < ehdr->e_phnum; i++)
    {
        if (phdr[i].p_type == PT_LOAD)
        {
            uint64_t seg_start = phdr[i].p_vaddr;
            uint64_t seg_end = seg_start + phdr[i].p_memsz;

            // Check if entry point falls within this segment
            if (entry_vaddr >= seg_start && entry_vaddr < seg_end)
            {
                // Calculate offset within this segment
                uint64_t offset_in_seg = entry_vaddr - seg_start;

                // Map to actual memory location
                uint64_t seg_base_actual = (uint64_t)base_mem + (seg_start - base_vaddr);
                entry_actual = seg_base_actual + offset_in_seg;

                entry_valid = true;

                char buf[32];
                char* out = itoa(i, buf, 10);

                gScreenwriter->print("Entry in segment ", true);
                gScreenwriter->print(buf);
                gScreenwriter->print(", offset: 0x", true);
                gScreenwriter->printHex(offset_in_seg, true);
                gScreenwriter->print("\n", true);
                break;
            }
        }
    }

    if (!entry_valid)
    {
        gScreenwriter->print("ERROR: Entry point not in any loaded segment!\n", true);
        return 0;
    }

    gScreenwriter->print("Entry point (actual): 0x", true);
    gScreenwriter->printHex(entry_actual, true);
    gScreenwriter->print("\n", true);

    // Verify the entry point looks like valid code
    uint8_t *code = (uint8_t *)entry_actual;
    gScreenwriter->print("First bytes at entry: ", true);
    for (int i = 0; i < 8; i++)
    {
        gScreenwriter->printHex(code[i], true);
        gScreenwriter->print(" ", true);
    }
    gScreenwriter->print("\n", true);

    return entry_actual;
}

public:
    ELFLoader(PageTableManager *page_table_manager)
        : fat32(nullptr), ptm(page_table_manager)
    {
    }

    ~ELFLoader()
    {
        if (fat32)
            delete fat32;
    }

    // Main loading function - orchestrates all steps
    uint64_t LoadELFFromDisk(const char *filepath)
    {
        gScreenwriter->print("\n=== ELF Loader Started ===\n", true);
        gScreenwriter->print("Target file: ", true);
        gScreenwriter->print(filepath, true);
        gScreenwriter->print("\n\n", true);

        // Step 1: Initialize disk
        if (!InitializeDisk())
            return 0;

        // Step 2: Find partition
        uint64_t partition_lba = 0;
        if (!FindFAT32Partition(&partition_lba))
            return 0;

        // Step 3: Mount FAT32
        if (!MountFAT32(partition_lba))
            return 0;

        // Step 4: Read ELF file
        void *elf_data = nullptr;
        size_t elf_size = 0;
        if (!ReadELFFile(filepath, &elf_data, &elf_size))
            return 0;

        // Step 5: Validate ELF
        if (!ValidateELF(elf_data))
        {
            gHeap.free(elf_data);
            return 0;
        }

        // Step 6: Calculate layout
        uint64_t base_vaddr = 0;
        uint64_t total_size = 0;
        if (!CalculateMemoryLayout(elf_data, &base_vaddr, &total_size))
        {
            gHeap.free(elf_data);
            return 0;
        }

        // Step 7: Allocate memory
        void *base_mem = nullptr;
        if (!AllocateMemory(total_size, &base_mem))
        {
            gHeap.free(elf_data);
            return 0;
        }

        // Step 8: Load segments
        if (!LoadSegments(elf_data, base_mem, base_vaddr))
        {
            gHeap.free(base_mem);
            gHeap.free(elf_data);
            return 0;
        }

        // Step 9: Map pages
        if (!MapPages(elf_data, base_mem, base_vaddr))
        {
            gHeap.free(base_mem);
            gHeap.free(elf_data);
            return 0;
        }

        // Step 10: Get entry point
        uint64_t entry = CalculateEntryPoint(elf_data, base_mem, base_vaddr);

        // Cleanup ELF data (no longer needed)
        gHeap.free(elf_data);

        gScreenwriter->print("\n=== ELF Loader Complete ===\n", true);
        gScreenwriter->print("Ready to execute at: 0x", true);
        gScreenwriter->printHex(entry, true);
        gScreenwriter->print("\n\n", true);

        return entry;
    }

};
