bits 64

global lidt
section .text

lidt:
    lidt [rdi]
    ret