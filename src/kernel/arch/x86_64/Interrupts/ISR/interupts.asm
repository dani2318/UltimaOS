[BITS 64]

extern isr_handler

global isr_common
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
    
    ; Pass pointer to register structure as argument
    mov rdi, rsp
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
    
    ; Remove error code and interrupt number from stack
    add rsp, 16
    
    iretq

global isr0
isr0:
    push qword 0    ; Dummy error code
    push qword 0    ; Interrupt number
    jmp isr_common

global isr13
isr13:
    ; CPU pushes error code automatically
    push qword 13   ; Interrupt number
    jmp isr_common

global isr14
isr14:
    ; CPU pushes error code automatically
    push qword 14   ; Interrupt number
    jmp isr_common


extern irq_handler

global irq0, irq1, irq2, irq3, irq4, irq5, irq6, irq7
global irq8, irq9, irq10, irq11, irq12, irq13, irq14, irq15

; IRQ 0 - Timer
irq0:
    push qword 0
    push qword 32
    jmp irq_common

; IRQ 1 - Keyboard
irq1:
    push qword 0
    push qword 33
    jmp irq_common

; IRQ 2 - Cascade (never raised)
irq2:
    push qword 0
    push qword 34
    jmp irq_common

; IRQ 3 - COM2
irq3:
    push qword 0
    push qword 35
    jmp irq_common

; IRQ 4 - COM1
irq4:
    push qword 0
    push qword 36
    jmp irq_common

; IRQ 5 - LPT2
irq5:
    push qword 0
    push qword 37
    jmp irq_common

; IRQ 6 - Floppy disk
irq6:
    push qword 0
    push qword 38
    jmp irq_common

; IRQ 7 - LPT1
irq7:
    push qword 0
    push qword 39
    jmp irq_common

; IRQ 8 - CMOS real-time clock
irq8:
    push qword 0
    push qword 40
    jmp irq_common

; IRQ 9 - Free for peripherals / legacy SCSI / NIC
irq9:
    push qword 0
    push qword 41
    jmp irq_common

; IRQ 10 - Free for peripherals / SCSI / NIC
irq10:
    push qword 0
    push qword 42
    jmp irq_common

; IRQ 11 - Free for peripherals / SCSI / NIC
irq11:
    push qword 0
    push qword 43
    jmp irq_common

; IRQ 12 - PS2 Mouse
irq12:
    push qword 0
    push qword 44
    jmp irq_common

; IRQ 13 - FPU / Coprocessor / Inter-processor
irq13:
    push qword 0
    push qword 45
    jmp irq_common

; IRQ 14 - Primary ATA Hard Disk
irq14:
    push qword 0
    push qword 46
    jmp irq_common

; IRQ 15 - Secondary ATA Hard Disk
irq15:
    push qword 0
    push qword 47
    jmp irq_common

; Common IRQ handler
irq_common:
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
    
    ; Pass pointer to register structure as argument
    mov rdi, rsp
    call irq_handler
    
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
    
    ; Remove error code and interrupt number from stack
    add rsp, 16
    
    iretq