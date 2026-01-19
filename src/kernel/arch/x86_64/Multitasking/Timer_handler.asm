[extern Scheduler_Schedule_Wrapper]
[global Timer_Handler]

%define APIC_EOI_REGISTER_OFFSET 0xB0
%define LAPIC_BASE_ADDR 0xFEE00000

Timer_Handler:
    push rax
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    
    sub rsp, 8  ; Stack alignment
    
    call Scheduler_Schedule_Wrapper
    
    add rsp, 8
    
    ; Send EOI to PIC (not APIC)
    mov al, 0x20
    out 0x20, al   ; Send EOI to master PIC
    
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rax
    
    iretq