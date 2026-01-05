[extern Scheduler_Schedule_Wrapper]
[global Timer_Handler]

; Define the APIC EOI Register offset
%define APIC_EOI_REGISTER_OFFSET 0xB0
; Replace this with your actual mapped LAPIC address
; If you are identity mapping, 0xFEE00000 is the standard physical base
%define LAPIC_BASE_ADDR 0xFEE00000

Timer_Handler:
    ; Save ALL registers that aren't saved by switch_task
    push rax
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    
    call Scheduler_Schedule_Wrapper

    ; Send EOI to PIC
    ; APIC EOI: Write 0 to Local APIC Base + 0xB0
    ; You'll need to get your LAPIC_BASE (e.g., 0xFEE00000)
    mov rax, LAPIC_BASE_ADDR
    mov dword [rax + APIC_EOI_REGISTER_OFFSET], 0
    
    ; Call the scheduler
    
    ; Restore ALL registers
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rax
    
    ; Return from interrupt (CPU pops RIP, CS, RFLAGS, RSP, SS)
    iretq