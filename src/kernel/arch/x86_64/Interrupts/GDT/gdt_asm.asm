[BITS 64]
global gdt_flush
gdt_flush:
    lgdt [rdi]          ; Load GDT
    mov ax, 0x10        ; Data segment selector
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    ; Far return to reload CS
    pop rdi             ; Get return address
    mov rax, 0x08       ; Code segment selector
    push rax            ; Push new CS
    push rdi            ; Push return address
    retfq               ; Far return