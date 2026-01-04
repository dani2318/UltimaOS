[BITS 64]

global switch_task

; void switch_task(TaskContext* old_context, TaskContext* new_context)
; RDI = old_context (where we save current state)
; RSI = new_context (where we load next state from)
switch_task:
    ; =====================================================
    ; 1. SAVE OLD CONTEXT
    ; =====================================================
    ; If old_context is NULL (first task ever), skip saving
    test rdi, rdi
    jz .load_new

    mov [rdi + 0],   r15
    mov [rdi + 8],   r14
    mov [rdi + 16],  r13
    mov [rdi + 24],  r12
    mov [rdi + 32],  r11
    mov [rdi + 40],  r10
    mov [rdi + 48],  r9
    mov [rdi + 56],  r8
    mov [rdi + 64],  rbp
    mov [rdi + 72],  rdi
    mov [rdi + 80],  rsi
    mov [rdi + 88],  rdx
    mov [rdi + 96],  rcx
    mov [rdi + 104], rbx
    mov [rdi + 112], rax
    
    ; Save Data Segment and Flags
    mov ax, ds
    mov [rdi + 120], rax
    pushfq
    pop rax
    mov [rdi + 144], rax 

    ; Save RIP (The address this function returns to)
    mov rax, [rsp]
    mov [rdi + 128], rax
    
    ; Save RSP (The stack as it was before this call, +8 to skip return addr)
    lea rax, [rsp + 8]
    mov [rdi + 152], rax
    
    mov rax, cs
    mov [rdi + 136], rax
    mov rax, ss
    mov [rdi + 160], rax

    ; =====================================================
    ; 2. LOAD NEW CONTEXT
    ; =====================================================
.load_new:
    ; IMPORTANT: We stay on the KERNEL stack to build the iretq frame.
    ; Do NOT mov rsp, [rsi + 152] yet!

    ; Update Data Segments
    mov ax, [rsi + 120] 
    mov ds, ax
    mov es, ax

    ; Build the IRETQ Stack Frame
    ; This frame tells the CPU where to jump and what stack to use
    push qword [rsi + 160] ; SS (New Stack Segment)
    push qword [rsi + 152] ; RSP (New Stack Pointer)
    push qword [rsi + 144] ; RFLAGS
    push qword [rsi + 136] ; CS (New Code Segment)
    push qword [rsi + 128] ; RIP (New Instruction Pointer)

    

    ; Load General Purpose Registers
    mov r15, [rsi + 0]
    mov r14, [rsi + 8]
    mov r13, [rsi + 16]
    mov r12, [rsi + 24]
    mov r11, [rsi + 32]
    mov r10, [rsi + 40]
    mov r9,  [rsi + 48]
    mov r8,  [rsi + 56]
    mov rbp, [rsi + 64]
    mov rdx, [rsi + 88]
    mov rcx, [rsi + 96]
    mov rbx, [rsi + 104]
    mov rax, [rsi + 112]
    
    ; Restore RSI and RDI last (since we are using RSI to find data)
    mov rdi, [rsi + 72]
    mov rsi, [rsi + 80]
    
    ; Final Step: IRETQ
    ; This pops the 5 values we pushed, changes the privilege level 
    ; (if CS/SS RPL is 3), and starts the shell.
    iretq