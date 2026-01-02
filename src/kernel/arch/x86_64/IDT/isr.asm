bits 64
global isr0
extern isr_handler
section .text

isr0:
    cli
    push 0          ; fake error code
    push 0          ; interrupt number
    jmp isr_common

isr_common:
    ; Save all registers
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
    
    cld             ; Clear direction flag (SysV ABI requirement)
    
    mov rdi, rsp    ; Pass stack pointer as first argument
    call isr_handler
    
    ; Restore all registers
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
    
    add rsp, 16     ; Remove error code and interrupt number
    ; DON'T use sti here - iretq restores interrupt flag automatically
    iretq