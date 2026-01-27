[bits 64]
global IDTLoad
IDTLoad:
    lidt [rdi]
    ret