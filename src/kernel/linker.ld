ENTRY(start)
OUTPUT_FORMAT("elf64-x86-64")

/* UEFI kernels are often loaded at higher addresses by the firmware */
/* But if your bootloader specifically moves the kernel to 1MB, this is fine */

SECTIONS
{
    . = 0x2000000;
    _kernel_start = .;
    .text : ALIGN(4K) {
        *(.text.start)
        *(.text .text.*)
    }

    .rodata : {
        *(.rodata .rodata.*)
        
        . = ALIGN(8);
        __init_array_start = .;  /* Two underscores */
        KEEP(*(SORT_BY_INIT_PRIORITY(.init_array.*) SORT_BY_INIT_PRIORITY(.ctors.*)))
        KEEP(*(.init_array .ctors))
        __init_array_end = .;    /* Two underscores */
    }
    
    .data : ALIGN(4K) {
        *(.data .data.*)
    }

    .bss : ALIGN(4K) {
        *(COMMON)
        *(.bss .bss.*)
    }
    _kernel_end = .;
}