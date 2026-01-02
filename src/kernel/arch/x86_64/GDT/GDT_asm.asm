[bits 64]

global x86_64_GDT_Load

x86_64_GDT_Load:
    lgdt [rdi]
    push rsi
    lea rax, [rel .reload_cs]
    push rax
    retfq

.reload_cs:
    mov ds, dx
    mov es, dx
    mov ss, dx

    mov ax, 0x00
    mov fs, ax
    mov gs, ax
    ret