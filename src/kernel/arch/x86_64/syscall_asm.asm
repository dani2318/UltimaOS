[bits 64]
[global syscall_entry]
[extern syscall_handler]

syscall_entry:
    ; 1. Swap from User GS to Kernel GS (to find kernel stack)
    swapgs
    mov [gs:0x10], rsp     ; Save User RSP in a safe place
    mov rsp, [gs:0x00]     ; Load Kernel RSP

    ; 2. Save registers to the stack (Syscall ABI: R11=RFLAGS, RCX=RIP)
    push r11
    push rcx
    push rbp
    push rdi
    push rsi
    push rdx
    push r8
    push r9
    push r10
    push r12
    push r13
    push r14
    push r15

    ; 3. Call the C++ handler (RDI, RSI, RDX are already syscall args)
    call syscall_handler

    ; 4. Restore registers
    pop r15
    pop r14
    pop r13
    pop r12
    pop r10  ; <--- This matches the push order now
    pop r9
    pop r8
    pop rdx
    pop rsi
    pop rdi
    pop rbp
    pop rcx 
    pop r11

    mov rsp, [gs:0x10]     ; Restore User RSP

    jmp $

    swapgs
    sysretq