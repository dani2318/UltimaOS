ENTRY(start)
OUTPUT_FORMAT("elf64-x86-64")

PHDRS
{
    text    PT_LOAD FLAGS(5); /* R + X */
    rodata  PT_LOAD FLAGS(4); /* R */
    data    PT_LOAD FLAGS(6); /* R + W */
}

SECTIONS
{
    . = 0x2000000;
    _kernel_start = .;

    .text : ALIGN(4K)
    {
        *(.text.start)
        *(.text .text.*)
    } :text

    .rodata : ALIGN(4K)
    {
        *(.rodata .rodata.*)

        . = ALIGN(8);
        __init_array_start = .;
        KEEP(*(SORT_BY_INIT_PRIORITY(.init_array.*) SORT_BY_INIT_PRIORITY(.ctors.*)))
        KEEP(*(.init_array .ctors))
        __init_array_end = .;
    } :rodata

    .data : ALIGN(4K)
    {
        *(.data .data.*)
    } :data

    .bss : ALIGN(4K)
    {
        *(COMMON)
        *(.bss .bss.*)
    } :data

    _kernel_end = .;
}
