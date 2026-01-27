[bits 64]
[global syscall_entry]
[extern syscall_handler]
[extern g_tss]

syscall_entry:
    ; Save user RSP in TSS temporary location
    mov [g_tss + 104], rsp  ; offset 104 = user_rsp_temp
    
    ; Load kernel stack from TSS.rsp0
    mov rsp, [g_tss + 4]    ; offset 4 = rsp0
    
    ; Save registers
    push rcx               ; RIP to return to
    push r11               ; RFLAGS
    push rbp
    push rbx
    push r12
    push r13
    push r14
    push r15
    
    ; Syscall convention: rax=syscall#, rdi=arg1, rsi=arg2, rdx=arg3
    ; C calling convention needs: rdi=arg1, rsi=arg2, rdx=arg3, rcx=syscall#
    mov rcx, rax           ; Move syscall number to 4th argument
    
    ; Call handler
    call syscall_handler
    
    ; Restore registers
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
    pop r11                ; RFLAGS
    pop rcx                ; Return RIP
    
    ; Restore user stack
    mov rsp, [g_tss + 104]
    
    ; Return to userspace
    sysretq