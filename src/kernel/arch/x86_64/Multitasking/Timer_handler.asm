[extern Scheduler_Schedule_Wrapper]
[global Timer_Handler]

Timer_Handler:
    ; 1. The CPU has already pushed SS, RSP, RFLAGS, CS, RIP onto the stack
    ; 2. We don't save registers here because switch_task will do it manually
    
    ; 3. Acknowledge the interrupt (Send EOI to PIC/APIC)
    mov al, 0x20
    out 0x20, al ; Sending EOI to Master PIC

    ; 4. Call the C++ scheduler
    call Scheduler_Schedule_Wrapper

    ; 5. Return from interrupt
    iretq