/* =============================================================================
 * UEFI Bootloader Linker Script
 * =============================================================================
 * This linker script is for GNU-EFI UEFI applications.
 * UEFI bootloaders are ELF shared objects, NOT flat binaries like kernels.
 * 
 * Key differences from kernel linker scripts:
 * - Must define ImageBase symbol (required by gnu-efi crt0)
 * - Must be position independent (PIC/PIE)
 * - Must include dynamic linking sections (.dynamic, .dynsym, .rela, etc.)
 * - Must have proper relocation sections (.reloc)
 * - Sections must be 4K aligned for UEFI page protection
 * ============================================================================= */

OUTPUT_FORMAT("elf64-x86-64")
OUTPUT_ARCH(i386:x86-64)
ENTRY(_start)

SECTIONS
{
  /* ImageBase MUST be defined - required by gnu-efi's crt0 */
  . = 0;
  ImageBase = .;
  
  /* Text section - executable code */
  . = ALIGN(4096);
  .text : {
    _text = .;
    *(.text .text.* .gnu.linkonce.t.*)
    _etext = .;
  }
  
  /* Relocation section - CRITICAL for UEFI */
  . = ALIGN(4096);
  .reloc : {
    *(.reloc)
  }
  
  /* Data sections - initialized data */
  . = ALIGN(4096);
  .data : {
    _data = .;
    *(.rodata .rodata.* .gnu.linkonce.r.*)
    *(.data .data.* .gnu.linkonce.d.*)
    *(.sdata)
    *(.got.plt)
    *(.got)
    _edata = .;
  }
  
  /* Dynamic linking section - required for shared objects */
  . = ALIGN(4096);
  .dynamic : {
    _dynamic = .;
    *(.dynamic)
  }
  
  /* Relocation with addend sections */
  . = ALIGN(4096);
  .rela : {
    *(.rela.text .rela.text.*)
    *(.rela.data .rela.data.*)
    *(.rela.rodata .rela.rodata.*)
    *(.rela.got)
    *(.rela.dyn)
    *(.rela.stab)
    *(.rela.*)
  }
  
  /* Dynamic symbol table */
  . = ALIGN(4096);
  .dynsym : {
    *(.dynsym)
  }
  
  /* Dynamic string table */
  . = ALIGN(4096);
  .dynstr : {
    *(.dynstr)
  }
  
  /* Symbol hash table */
  . = ALIGN(4096);
  .hash : {
    *(.hash)
  }
  
  /* BSS section - uninitialized data */
  . = ALIGN(4096);
  .bss : {
    _bss = .;
    *(.sbss)
    *(.scommon)
    *(.bss .bss.*)
    *(COMMON)
    _ebss = .;
  }
  
  /* Discard sections we don't need */
  /DISCARD/ : {
    *(.note.GNU-stack)
    *(.gnu_debuglink)
    *(.interp)
    *(.eh_frame)
    *(.comment)
  }
}