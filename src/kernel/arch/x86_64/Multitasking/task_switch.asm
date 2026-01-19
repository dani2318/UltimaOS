[bits 64]
global switch_context
global user_task_trampoline

; RDI = &old_task->context (Task_Registers*)
; RSI = &new_task->context (Task_Registers*)

switch_context:
    ; Save current context to old_task->context (RDI)
    
    ; Save general purpose registers (32-bit as per Task_Registers struct)
    mov [rdi + 0],  eax     ; eax offset 0
    mov [rdi + 4],  ebx     ; ebx offset 4
    mov [rdi + 8],  ecx     ; ecx offset 8
    mov [rdi + 12], edx     ; edx offset 12
    mov [rdi + 16], edi     ; edi offset 16
    mov [rdi + 20], esp     ; esp offset 20
    mov [rdi + 24], ebp     ; ebp offset 24
    
    ; Save EIP (return address) - offset 28
    mov rax, [rsp]
    mov [rdi + 28], eax
    
    ; Save EFLAGS - offset 32
    pushfq
    pop rax
    mov [rdi + 32], eax
    
    ; Save CR3 - offset 36
    mov rax, cr3
    mov [rdi + 36], eax
    
    ; Load new context from new_task->context (RSI)
    
    ; Load CR3 first (switch page tables)
    mov eax, [rsi + 36]
    mov cr3, rax
    
    ; Load EFLAGS
    mov eax, [rsi + 32]
    push rax
    popfq
    
    ; Load general purpose registers
    mov eax, [rsi + 0]
    mov ebx, [rsi + 4]
    mov ecx, [rsi + 8]
    mov edx, [rsi + 12]
    mov ebp, [rsi + 24]
    mov esp, [rsi + 20]
    
    ; Load EIP and jump
    mov edi, [rsi + 28]     ; Load EIP into EDI temporarily
    push rdi                 ; Push as return address
    mov edi, [rsi + 16]     ; Restore EDI
    ret                      ; Jump to new EIP

user_task_trampoline:
    ; Set data segments to User Data (0x23)
    mov ax, 0x23
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; Clear general purpose registers (Security: don't leak kernel data)
    xor rax, rax
    xor rbx, rbx
    xor rcx, rcx
    xor rdx, rdx
    xor rsi, rsi
    xor rdi, rdi
    xor rbp, rbp
    xor r8, r8
    xor r9, r9
    xor r10, r10
    xor r11, r11
    xor r12, r12
    xor r13, r13
    xor r14, r14
    xor r15, r15

    ; The IRET frame is already on the stack from Scheduler::CreateTask
    ; [RIP, CS, RFLAGS, RSP, SS]
    iretq