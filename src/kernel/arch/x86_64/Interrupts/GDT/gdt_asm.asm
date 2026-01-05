[BITS 64]
global gdt_flush
gdt_flush:
    lgdt [rdi]          ; Load the new GDT pointer

    ; 1. Reload Data Segments to 0x10 (Kernel Data)
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; 2. Reload Code Segment to 0x08 using Far Return
    lea rax, [rel .reload_cs]
    push 0x08               ; New CS (0x08)
    push rax                ; New RIP (.reload_cs)
    retfq                   ; Far Return: pops RAX into RIP and 0x08 into CS
.reload_cs:
    ret