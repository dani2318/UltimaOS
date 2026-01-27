/* src/programs/shell/linker.ld */
OUTPUT_FORMAT(elf64-x86-64)
ENTRY(_start)

SECTIONS
{
    /* Link address at 2MB */
    . = 0x200000;

    .text : { *(.text) }
    .data : { *(.data) }
    .rodata : { *(.rodata) }
    .bss  : { *(.bss) }
}