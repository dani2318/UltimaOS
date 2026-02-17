%ifndef I_PROTECTED_MODE_INC__
%define I_PROTECTED_MODE_INC__

; Global variables to save protected mode state
section .data
    g_PM_GDT: dw 0
              dd 0
    g_PM_IDT: dw 0
              dd 0
    g_PM_ESP: dd 0

section .text

%macro x86_EnterRealMode 0
    [bits 32]
    
    ; Save protected mode state
    mov [g_PM_ESP], esp
    sgdt [g_PM_GDT]
    sidt [g_PM_IDT]
    
    ; Jump to 16-bit protected mode segment
    jmp word 0x18:%%pmode16
    
%%pmode16:
    [bits 16]
    ; Load 16-bit data segments
    mov ax, 0x20
    mov ds, ax
    mov ss, ax
    
    ; Disable protected mode bit in cr0
    mov eax, cr0
    and al, ~1
    mov cr0, eax
    
    ; Jump to real mode
    jmp word 0x00:%%rmode
    
%%rmode:
    ; Setup real mode segments
    xor ax, ax
    mov ds, ax
    mov ss, ax
    mov sp, 0x7C00          ; Use a safe stack location
    
    ; Enable interrupts for BIOS calls
    sti
%endmacro
%macro x86_EnterProtectedMode 0
    [bits 16]
    cli
    
    ; In real mode, we need to make sure we can access g_PM_GDT
    ; Assuming it's in the data segment that's been loaded
    push ds
    xor ax, ax
    mov ds, ax
    
    ; Reload GDT using absolute address
    lgdt [g_PM_GDT]
    
    pop ds
    
    ; Set protection enable flag in CR0
    mov eax, cr0
    or al, 1
    mov cr0, eax
    
    ; Far jump into protected mode
    jmp dword 0x08:%%pmode
    
%%pmode:
    [bits 32]
    
    ; Setup segment registers
    mov ax, 0x10
    mov ds, ax
    mov ss, ax
    
    ; Restore stack pointer
    mov esp, [g_PM_ESP]
    
    ; Reload IDT
    lidt [g_PM_IDT]
%endmacro

; Linear address to segment:offset conversion
;    1 - linear address
;    2 - (out) target segment (e.g. es)
;    3 - target 32-bit register to use (e.g. eax)
;    4 - target lower 16-bit half of #3 (e.g. ax)
%macro LinearToSegOffset 4
    mov %3, %1
    shr %3, 4
    mov %2, %4
    mov %3, %1
    and %3, 0xf
%endmacro

%endif