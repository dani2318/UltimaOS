; kernel_entry.asm (Assemble with: nasm -f elf64)
[bits 64]
global start
extern kernel_main

section .text.start
start:
    cli                 ; Disable interrupts
    mov rbp, rsp        ; Setup stack frame
    call kernel_main    ; Jump to your C code
    hlt                 ; Halt if C code returns