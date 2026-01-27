#pragma once
#include <stdint.h>

// ELF Magic Numbers
#define EI_MAG0     0  // e_ident[] indexes
#define EI_MAG1     1
#define EI_MAG2     2
#define EI_MAG3     3
#define EI_CLASS    4
#define EI_DATA     5
#define EI_VERSION  6
#define EI_OSABI    7
#define EI_PAD      8

#define ELFMAG0     0x7f  // Magic number bytes
#define ELFMAG1     'E'
#define ELFMAG2     'L'
#define ELFMAG3     'F'

// ELF Classes
#define ELFCLASSNONE 0  // Invalid class
#define ELFCLASS32   1  // 32-bit objects
#define ELFCLASS64   2  // 64-bit objects

// ELF Data encodings
#define ELFDATANONE  0  // Invalid data encoding
#define ELFDATA2LSB  1  // Little-endian
#define ELFDATA2MSB  2  // Big-endian

// ELF File Types
#define ET_NONE      0  // No file type
#define ET_REL       1  // Relocatable file
#define ET_EXEC      2  // Executable file
#define ET_DYN       3  // Shared object file
#define ET_CORE      4  // Core file

// ELF Machine Types
#define EM_NONE      0  // No machine
#define EM_386       3  // Intel 80386
#define EM_X86_64   62  // AMD x86-64

// ELF Versions
#define EV_NONE      0  // Invalid version
#define EV_CURRENT   1  // Current version

// Program Header Types
#define PT_NULL      0  // Unused entry
#define PT_LOAD      1  // Loadable segment
#define PT_DYNAMIC   2  // Dynamic linking info
#define PT_INTERP    3  // Interpreter path
#define PT_NOTE      4  // Auxiliary info
#define PT_SHLIB     5  // Reserved
#define PT_PHDR      6  // Program header table
#define PT_TLS       7  // Thread-Local Storage

// Program Header Flags
#define PF_X         0x1  // Execute
#define PF_W         0x2  // Write
#define PF_R         0x4  // Read

// Section Header Types
#define SHT_NULL     0  // Inactive section
#define SHT_PROGBITS 1  // Program-defined info
#define SHT_SYMTAB   2  // Symbol table
#define SHT_STRTAB   3  // String table
#define SHT_RELA     4  // Relocation entries with addends
#define SHT_HASH     5  // Symbol hash table
#define SHT_DYNAMIC  6  // Dynamic linking info
#define SHT_NOTE     7  // Note section
#define SHT_NOBITS   8  // Space with no data (BSS)
#define SHT_REL      9  // Relocation entries, no addends
#define SHT_SHLIB   10  // Reserved
#define SHT_DYNSYM  11  // Dynamic linker symbol table

// Section Header Flags
#define SHF_WRITE       0x1  // Writable
#define SHF_ALLOC       0x2  // Occupies memory
#define SHF_EXECINSTR   0x4  // Executable

// Symbol Binding
#define STB_LOCAL    0  // Local symbol
#define STB_GLOBAL   1  // Global symbol
#define STB_WEAK     2  // Weak symbol

// Symbol Types
#define STT_NOTYPE   0  // No type
#define STT_OBJECT   1  // Data object
#define STT_FUNC     2  // Function
#define STT_SECTION  3  // Section
#define STT_FILE     4  // File name

// Special Section Indexes
#define SHN_UNDEF    0  // Undefined section

// 64-bit ELF Header
typedef struct {
    unsigned char e_ident[16];  // Magic number and other info
    uint16_t      e_type;       // Object file type
    uint16_t      e_machine;    // Architecture
    uint32_t      e_version;    // Object file version
    uint64_t      e_entry;      // Entry point virtual address
    uint64_t      e_phoff;      // Program header table file offset
    uint64_t      e_shoff;      // Section header table file offset
    uint32_t      e_flags;      // Processor-specific flags
    uint16_t      e_ehsize;     // ELF header size in bytes
    uint16_t      e_phentsize;  // Program header table entry size
    uint16_t      e_phnum;      // Program header table entry count
    uint16_t      e_shentsize;  // Section header table entry size
    uint16_t      e_shnum;      // Section header table entry count
    uint16_t      e_shstrndx;   // Section header string table index
} Elf64_Ehdr;

// 64-bit Program Header
typedef struct {
    uint32_t p_type;    // Segment type
    uint32_t p_flags;   // Segment flags
    uint64_t p_offset;  // Segment file offset
    uint64_t p_vaddr;   // Segment virtual address
    uint64_t p_paddr;   // Segment physical address
    uint64_t p_filesz;  // Segment size in file
    uint64_t p_memsz;   // Segment size in memory
    uint64_t p_align;   // Segment alignment
} Elf64_Phdr;

// 64-bit Section Header
typedef struct {
    uint32_t sh_name;      // Section name (string tbl index)
    uint32_t sh_type;      // Section type
    uint64_t sh_flags;     // Section flags
    uint64_t sh_addr;      // Section virtual addr at execution
    uint64_t sh_offset;    // Section file offset
    uint64_t sh_size;      // Section size in bytes
    uint32_t sh_link;      // Link to another section
    uint32_t sh_info;      // Additional section information
    uint64_t sh_addralign; // Section alignment
    uint64_t sh_entsize;   // Entry size if section holds table
} Elf64_Shdr;

// 64-bit Symbol Table Entry
typedef struct {
    uint32_t st_name;   // Symbol name (string tbl index)
    uint8_t  st_info;   // Symbol type and binding
    uint8_t  st_other;  // Symbol visibility
    uint16_t st_shndx;  // Section index
    uint64_t st_value;  // Symbol value
    uint64_t st_size;   // Symbol size
} Elf64_Sym;

// 64-bit Relocation Entry
typedef struct {
    uint64_t r_offset;  // Address
    uint64_t r_info;    // Relocation type and symbol index
} Elf64_Rel;

// 64-bit Relocation Entry with Addend
typedef struct {
    uint64_t r_offset;  // Address
    uint64_t r_info;    // Relocation type and symbol index
    int64_t  r_addend;  // Addend
} Elf64_Rela;

// 32-bit ELF Header (for reference/compatibility)
typedef struct {
    unsigned char e_ident[16];
    uint16_t      e_type;
    uint16_t      e_machine;
    uint32_t      e_version;
    uint32_t      e_entry;
    uint32_t      e_phoff;
    uint32_t      e_shoff;
    uint32_t      e_flags;
    uint16_t      e_ehsize;
    uint16_t      e_phentsize;
    uint16_t      e_phnum;
    uint16_t      e_shentsize;
    uint16_t      e_shnum;
    uint16_t      e_shstrndx;
} Elf32_Ehdr;

// 32-bit Program Header
typedef struct {
    uint32_t p_type;
    uint32_t p_offset;
    uint32_t p_vaddr;
    uint32_t p_paddr;
    uint32_t p_filesz;
    uint32_t p_memsz;
    uint32_t p_flags;
    uint32_t p_align;
} Elf32_Phdr;

// Helper macros for symbol info
#define ELF64_ST_BIND(i)    ((i) >> 4)
#define ELF64_ST_TYPE(i)    ((i) & 0xf)
#define ELF64_ST_INFO(b,t)  (((b) << 4) + ((t) & 0xf))

// Helper macros for relocation info
#define ELF64_R_SYM(i)      ((i) >> 32)
#define ELF64_R_TYPE(i)     ((i) & 0xffffffff)
#define ELF64_R_INFO(s,t)   (((s) << 32) + ((t) & 0xffffffff))

// Helper function to validate ELF magic
inline bool IsValidELF(const unsigned char *ident) {
    return (ident[EI_MAG0] == ELFMAG0 &&
            ident[EI_MAG1] == ELFMAG1 &&
            ident[EI_MAG2] == ELFMAG2 &&
            ident[EI_MAG3] == ELFMAG3);
}

// Helper function to check if ELF is 64-bit
inline bool IsElf64(const unsigned char *ident) {
    return ident[EI_CLASS] == ELFCLASS64;
}

// Helper function to check if ELF is little-endian
inline bool IsLittleEndian(const unsigned char *ident) {
    return ident[EI_DATA] == ELFDATA2LSB;
}