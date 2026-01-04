#pragma once
#include <stdint.h>
#include <stddef.h>
#include <arch/x86_64/Drivers/HAL/HAL.hpp>
#include <arch/x86_64/Paging.hpp>
#include <arch/x86_64/Serial.hpp>
#include <memory/memory.hpp>
#include <arch/x86_64/Filesystem/Filesystem.hpp>
#include <cpp/String.hpp>

struct ELF64_Ehdr
{
    uint8_t e_ident[16];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint64_t e_entry;
    uint64_t e_phoff;
    uint64_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
};

struct ELF64_Phdr
{
    uint32_t p_type;
    uint32_t p_flags;
    uint64_t p_offset;
    uint64_t p_vaddr;
    uint64_t p_paddr;
    uint64_t p_filesz;
    uint64_t p_memsz;
    uint64_t p_align;
};

struct ELF64_Shdr
{
    uint32_t sh_name;
    uint32_t sh_type;
    uint64_t sh_flags;
    uint64_t sh_addr;
    uint64_t sh_offset;
    uint64_t sh_size;
    uint32_t sh_link;
    uint32_t sh_info;
    uint64_t sh_addralign;
    uint64_t sh_entsize;
};

struct ELF64_Rela
{
    uint64_t r_offset;
    uint64_t r_info;
    int64_t r_addend;
};

#define PT_LOAD 1
#define PT_DYNAMIC 2
#define PF_X 1
#define PF_W 2
#define PF_R 4

#define R_X86_64_64 1

class ELFDriver
{
public:
    const char *filename;
    void *base_address;
    uint64_t size;
    uint64_t entry;

    using DriverInitFunc = void (*)(HAL *, PageTableManager &);
    DriverInitFunc init_func;
    uint64_t GetEntryPoint() { return entry; }
    ELFDriver() : filename(nullptr), base_address(nullptr), size(0), entry(0), init_func(nullptr) {}

    bool Load(const char *fname, HAL *hal, PageTableManager &manager)
    {
        filename = fname;
        g_serialWriter->Print("ELFDriver: Loading file: ");
        g_serialWriter->Print(fname);
        g_serialWriter->Print("\n");

        // 1. Read ELF file into memory
        void *elf_data = FileSystem::ReadAll(fname, &size);
        if (!elf_data)
        {
            g_serialWriter->Print("ELFDriver: Failed to read file\n");
            return false;
        }

        ELF64_Ehdr *ehdr = (ELF64_Ehdr *)elf_data;

        // 2. Verify ELF header
        if (ehdr->e_ident[0] != 0x7F || ehdr->e_ident[1] != 'E' ||
            ehdr->e_ident[2] != 'L' || ehdr->e_ident[3] != 'F')
        {
            g_serialWriter->Print("ELFDriver: Invalid ELF\n");
            return false;
        }
        if (ehdr->e_ident[4] != 2)
        {
            g_serialWriter->Print("ELFDriver: Not 64-bit\n");
            return false;
        }

        // 3. Determine memory layout
        ELF64_Phdr *phdrs = (ELF64_Phdr *)((uint8_t *)elf_data + ehdr->e_phoff);
        uint64_t lowest_vaddr = UINT64_MAX;
        uint64_t highest_vaddr = 0;
        for (uint16_t i = 0; i < ehdr->e_phnum; i++)
        {
            ELF64_Phdr *ph = &phdrs[i];
            if (ph->p_type != PT_LOAD)
                continue;
            if (ph->p_vaddr < lowest_vaddr)
                lowest_vaddr = ph->p_vaddr;
            if (ph->p_vaddr + ph->p_memsz > highest_vaddr)
                highest_vaddr = ph->p_vaddr + ph->p_memsz;
        }

        uint64_t total_size = highest_vaddr - lowest_vaddr;
        void *mem = g_heap.Alloc(total_size);
        if (!mem)
        {
            g_serialWriter->Print("ELFDriver: Failed to allocate memory\n");
            return false;
        }
        base_address = mem;

        // 4. Load segments
        for (uint16_t i = 0; i < ehdr->e_phnum; i++)
        {
            ELF64_Phdr *ph = &phdrs[i];
            if (ph->p_type != PT_LOAD)
                continue;

            void *seg_addr = (uint8_t *)mem + (ph->p_vaddr - lowest_vaddr);
            memcpy(seg_addr, (uint8_t *)elf_data + ph->p_offset, ph->p_filesz);
            if (ph->p_memsz > ph->p_filesz)
                memset((uint8_t *)seg_addr + ph->p_filesz, 0, ph->p_memsz - ph->p_filesz);

            // Map pages
            uint64_t start = (uint64_t)seg_addr & ~0xFFFULL;
            uint64_t end = ((uint64_t)seg_addr + ph->p_memsz + 0xFFF) & ~0xFFFULL;
            bool exec = (ph->p_flags & PF_X) != 0;
            for (uint64_t addr = start; addr < end; addr += 4096)
            {
                uint64_t phys_addr = (uint64_t)seg_addr; // This is the memory from g_heap.Alloc
                uint64_t virt_addr = ph->p_vaddr;        // This is where the ELF wants to live (e.g., 0x200000)

                for (uint64_t offset = 0; offset < ph->p_memsz; offset += 4096)
                {
                    manager.MapMemory((void *)(virt_addr + offset), (void *)(phys_addr + offset), true);
                }
            }
        }

        entry = ehdr->e_entry - lowest_vaddr + (uint64_t)mem;

        // 5. Apply relocations (simplified: R_X86_64_64)
        for (uint16_t i = 0; i < ehdr->e_shnum; i++)
        {
            ELF64_Shdr *sh = (ELF64_Shdr *)((uint8_t *)elf_data + ehdr->e_shoff + i * ehdr->e_shentsize);
            if (sh->sh_type != 4 /* SHT_RELA */)
                continue;

            ELF64_Rela *rela = (ELF64_Rela *)((uint8_t *)elf_data + sh->sh_offset);
            uint64_t count = sh->sh_size / sizeof(ELF64_Rela);
            for (uint64_t r = 0; r < count; r++)
            {
                ELF64_Rela *rec = &rela[r];
                uint32_t type = rec->r_info & 0xFFFFFFFF;
                if (type == R_X86_64_64)
                {
                    uint64_t *reloc = (uint64_t *)((uint8_t *)mem + (rec->r_offset - lowest_vaddr));
                    *reloc = (uint64_t)mem + rec->r_addend;
                }
            }
        }

        // 6. Initialize C++ static constructors
        for (uint16_t i = 0; i < ehdr->e_shnum; i++)
        {
            ELF64_Shdr *sh = (ELF64_Shdr *)((uint8_t *)elf_data + ehdr->e_shoff + i * ehdr->e_shentsize);
            if (sh->sh_type != 14 /* SHT_INIT_ARRAY */)
                continue;
            void (**ctors)() = (void (**)())((uint8_t *)mem + (sh->sh_addr - lowest_vaddr));
            uint64_t count = sh->sh_size / sizeof(void *);
            for (uint64_t c = 0; c < count; c++)
                ctors[c]();
        }

        // 7. Lookup DriverInit entry
        init_func = (DriverInitFunc)((uint8_t *)mem + (ehdr->e_entry - lowest_vaddr));

        g_serialWriter->Print("ELFDriver: Loaded at 0x");

        return true;
    }

    bool LoadFromBuffer(void *elf_data, size_t elf_size, PageTableManager &manager)
    {
        ELF64_Ehdr *ehdr = (ELF64_Ehdr *)elf_data;

        // 1. Verify ELF Header
        if (memcmp(ehdr->e_ident, "\x7f\x45\x4c\x46", 4) != 0)
            return false;

        // 2. Parse Program Headers
        ELF64_Phdr *phdrs = (ELF64_Phdr *)((uint8_t *)elf_data + ehdr->e_phoff);

        for (uint16_t i = 0; i < ehdr->e_phnum; i++)
        {
            ELF64_Phdr *ph = &phdrs[i];
            if (ph->p_type != PT_LOAD)
                continue;

            // Allocate PHYSICAL pages from PMM
            uint64_t pages = (ph->p_memsz + 4095) / 4096;
            void *physical_mem = g_PMM.RequestPages(pages);

            // Zero out the physical memory first (important for .bss)
            memset(physical_mem, 0, pages * 4096);

            // Copy data from the ELF buffer to the PHYSICAL memory
            memcpy(physical_mem, (uint8_t *)elf_data + ph->p_offset, ph->p_filesz);

            // Map the VIRTUAL address the ELF expects to the PHYSICAL pages we allocated
            for (uint64_t j = 0; j < pages; j++)
            {
                uint64_t v_addr = ph->p_vaddr + (j * 4096);
                uint64_t p_addr = (uint64_t)physical_mem + (j * 4096);

                // Flags: Present, Writable, User (if applicable)
                manager.MapMemory((void *)v_addr, (void *)p_addr, true, true);
            }
        }

        // 3. Set the entry point (Virtual Address)
        entry = ehdr->e_entry;
        return true;
    }

    void Initialize(HAL *hal, PageTableManager &manager)
    {
        if (init_func)
            init_func(hal, manager);
    }
};
