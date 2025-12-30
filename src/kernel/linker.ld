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
        __ctors_start = .;
        *(.init_array)
        *(.ctors)
        __ctors_end = .;
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