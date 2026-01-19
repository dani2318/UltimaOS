[BITS 64]
global gdt_flush
gdt_flush:
    lgdt [rdi]          ; Load the new GDT pointer

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    lea rax, [rel .reload_cs]
    push 0x08               ; New CS (0x08)
    push rax                ; New RIP (.reload_cs)
    retfq                   ; Far Return: pops RAX into RIP and 0x08 into CS
.reload_cs:
    ret