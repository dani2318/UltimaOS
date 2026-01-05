[bits 64]
global switch_task

switch_task:
    ; RDI = &old_task->context
    ; RSI = &new_task->context
    ; RDX = next_task->page_table (New CR3)

    ; 1. Save old context
    mov [rdi + 0], rsp
    mov [rdi + 8], rbp
    mov [rdi + 16], rbx
    mov [rdi + 24], r12
    mov [rdi + 32], r13
    mov [rdi + 40], r14
    mov [rdi + 48], r15
    pushfq
    pop qword [rdi + 56]

    ; 2. Switch Page Table (CR3)
    ; Important: We do this AFTER saving the old RSP but BEFORE loading the new RSP
    ; to ensure we are using the new address space for the new task's stack.
    mov rax, cr3
    cmp rax, rdx
    je .skip_cr3
    mov cr3, rdx    ; Atomic swap of address space
.skip_cr3:

    ; 3. Load new context
    mov rsp, [rsi + 0]
    mov rbp, [rsi + 8]
    mov rbx, [rsi + 16]
    mov r12, [rsi + 24]
    mov r13, [rsi + 32]
    mov r14, [rsi + 40]
    mov r15, [rsi + 48]
    push qword [rsi + 56]
    popfq

    ret  ; Pops the trampoline address (or return RIP) and jumps to it

global user_task_trampoline
user_task_trampoline:
    ; 1. Set data segments to User Data (0x23)
    mov ax, 0x23
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; 2. Clear general purpose registers (Security: don't leak kernel data)
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

    ; 3. The IRET frame is already on the stack from Scheduler::CreateTask
    ; [RIP, CS, RFLAGS, RSP, SS]
    iretq