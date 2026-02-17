[bits 16]

section .entry

extern __bss_start
extern __end
extern _init
extern Start
global entry

entry:
    cli

    mov [g_BootDrive], dl
    mov [g_BootPartitionOff], si
    mov [g_BootPartitionSeg], di

    mov ax, ds
    mov ss, ax
    mov sp, 0xFFF0
    mov bp, sp

    ; switch to protected mode
    call EnableA20
    call LoadGDT

    ; set protection enable flag in CR0
    mov eax, cr0
    or al, 1
    mov cr0, eax

    ; far jmp in pmode
    jmp dword 08h:.pmode

.pmode:
    ; we are now in protected mode!
    [bits 32]
    
    ; setup segment registers
    mov ax, 0x10
    mov ds, ax
    mov ss, ax
    
    ; Set up a proper stack with alignment
    mov esp, 0x90000        ; Stack at 576KB (safe location)
    mov ebp, esp
    and esp, 0xFFFFFFF0     ; Align stack to 16-byte boundary
   
    ; clear bss (uninitialized data) FIRST - before setting up IDT
    mov edi, __bss_start
    mov ecx, __end
    sub ecx, edi
    mov al, 0
    cld
    rep stosb

    ; Setup IDT AFTER clearing BSS so it doesn't get wiped
    call SetupIDT

    ; call global constructors
    call _init

    ; Pass partition address to Start function
    xor edx, edx
    mov dx, [g_BootPartitionSeg]
    shl edx, 16
    mov dx, [g_BootPartitionOff]
    push edx

    ; Pass boot drive to Start function
    xor edx, edx
    mov dl, [g_BootDrive]
    push edx
    
    call Start

    add esp, 8              ; Clean up stack
    cli
    hlt

; Setup a basic IDT to catch exceptions
SetupIDT:
    [bits 32]
    ; Fill IDT with exception handlers (0-31)
    mov edi, IDT_BASE
    xor ecx, ecx            ; Exception number
    
.fill_exceptions:
    cmp ecx, 32
    jge .fill_rest
    
    ; Calculate handler address for this exception
    mov eax, ExceptionHandlers
    mov ebx, ecx
    shl ebx, 4              ; Each handler stub is 16 bytes (multiply by 16)
    add eax, ebx
    
    mov word [edi], ax              ; Offset bits 0-15
    mov word [edi+2], 0x08          ; Code segment selector
    mov byte [edi+4], 0             ; Reserved
    mov byte [edi+5], 0x8E          ; Type: 32-bit interrupt gate, present, DPL=0
    shr eax, 16
    mov word [edi+6], ax            ; Offset bits 16-31
    add edi, 8
    inc ecx
    jmp .fill_exceptions
    
.fill_rest:
    ; Fill rest with generic handler
    mov ecx, 224            ; 256 - 32 remaining entries
.fill_loop:
    mov eax, DummyExceptionHandler
    mov word [edi], ax
    mov word [edi+2], 0x08
    mov byte [edi+4], 0
    mov byte [edi+5], 0x8E
    shr eax, 16
    mov word [edi+6], ax
    add edi, 8
    loop .fill_loop
    
    ; Load IDT
    lidt [IDTDescriptor]
    ret

; Exception handler stubs - one for each exception 0-31
; Each stub is exactly 16 bytes
ExceptionHandlers:
%assign i 0
%rep 32
    push dword i
    jmp CommonExceptionHandler
    nop
    nop
    nop
    nop
    nop
    nop
%assign i i+1
%endrep

CommonExceptionHandler:
    [bits 32]
    cli
    
    ; Stack: [exception_num], [error_code (maybe)], EIP, CS, EFLAGS
    pop eax                 ; Get exception number
    
    ; Display exception number on screen
    mov edi, 0xB8000
    mov byte [edi], 'E'
    mov byte [edi+1], 0x4F  ; White on red
    mov byte [edi+2], 'X'
    mov byte [edi+3], 0x4F
    mov byte [edi+4], ':'
    mov byte [edi+5], 0x4F
    
    ; Convert exception number to hex and display
    mov ebx, eax
    shr ebx, 4
    and ebx, 0x0F
    add ebx, '0'
    cmp ebx, '9'
    jle .first_digit
    add ebx, 7
.first_digit:
    mov [edi+6], bl
    mov byte [edi+7], 0x4F
    
    mov ebx, eax
    and ebx, 0x0F
    add ebx, '0'
    cmp ebx, '9'
    jle .second_digit
    add ebx, 7
.second_digit:
    mov [edi+8], bl
    mov byte [edi+9], 0x4F
    
    ; Display " @" separator
    mov byte [edi+10], ' '
    mov byte [edi+11], 0x4F
    mov byte [edi+12], '@'
    mov byte [edi+13], 0x4F
    
    ; Display EIP
    mov eax, [esp]          ; EIP is now at esp (we popped exception num)
    mov edi, 0xB8000 + 28
    mov ecx, 8
.print_eip:
    rol eax, 4
    mov ebx, eax
    and ebx, 0x0F
    add ebx, '0'
    cmp ebx, '9'
    jle .eip_digit
    add ebx, 7
.eip_digit:
    mov [edi], bl
    mov byte [edi+1], 0x4F
    add edi, 2
    loop .print_eip
    
    hlt
    jmp $

DummyExceptionHandler:
    [bits 32]
    cli
    mov byte [0xB8000], '?'
    mov byte [0xB8001], 0x4F
    mov byte [0xB8002], '?'
    mov byte [0xB8003], 0x4F
    hlt
    jmp $

EnableA20:
    [bits 16]
    ; disable keyboard
    call A20WaitInput
    mov al, KbdControllerDisableKeyboard
    out KbdControllerCommandPort, al

    ; read control output port
    call A20WaitInput
    mov al, KbdControllerReadCtrlOutputPort
    out KbdControllerCommandPort, al

    call A20WaitOutput
    in al, KbdControllerDataPort
    push eax

    ; write control output port
    call A20WaitInput
    mov al, KbdControllerWriteCtrlOutputPort
    out KbdControllerCommandPort, al
    
    call A20WaitInput
    pop eax
    or al, 2                                    ; bit 2 = A20 bit
    out KbdControllerDataPort, al

    ; enable keyboard
    call A20WaitInput
    mov al, KbdControllerEnableKeyboard
    out KbdControllerCommandPort, al

    call A20WaitInput
    ret

A20WaitInput:
    [bits 16]
    ; wait until status bit 2 (input buffer) is 0
    ; by reading from command port, we read status byte
    in al, KbdControllerCommandPort
    test al, 2
    jnz A20WaitInput
    ret

A20WaitOutput:
    [bits 16]
    ; wait until status bit 1 (output buffer) is 1 so it can be read
    in al, KbdControllerCommandPort
    test al, 1
    jz A20WaitOutput
    ret

LoadGDT:
    [bits 16]
    lgdt [g_GDTDesc]
    ret

KbdControllerDataPort               equ 0x60
KbdControllerCommandPort            equ 0x64
KbdControllerDisableKeyboard        equ 0xAD
KbdControllerEnableKeyboard         equ 0xAE
KbdControllerReadCtrlOutputPort     equ 0xD0
KbdControllerWriteCtrlOutputPort    equ 0xD1

ScreenBuffer                        equ 0xB8000
IDT_BASE                            equ 0x7E00    ; Safe location after Stage1

g_GDT:      ; NULL descriptor
            dq 0

            ; 32-bit code segment
            dw 0FFFFh                   ; limit (bits 0-15) = 0xFFFFF for full 32-bit range
            dw 0                        ; base (bits 0-15) = 0x0
            db 0                        ; base (bits 16-23)
            db 10011010b                ; access (present, ring 0, code segment, executable, direction 0, readable)
            db 11001111b                ; granularity (4k pages, 32-bit pmode) + limit (bits 16-19)
            db 0                        ; base high

            ; 32-bit data segment
            dw 0FFFFh                   ; limit (bits 0-15) = 0xFFFFF for full 32-bit range
            dw 0                        ; base (bits 0-15) = 0x0
            db 0                        ; base (bits 16-23)
            db 10010010b                ; access (present, ring 0, data segment, executable, direction 0, writable)
            db 11001111b                ; granularity (4k pages, 32-bit pmode) + limit (bits 16-19)
            db 0                        ; base high

            ; 16-bit code segment
            dw 0FFFFh                   ; limit (bits 0-15) = 0xFFFFF
            dw 0                        ; base (bits 0-15) = 0x0
            db 0                        ; base (bits 16-23)
            db 10011010b                ; access (present, ring 0, code segment, executable, direction 0, readable)
            db 00001111b                ; granularity (1b pages, 16-bit pmode) + limit (bits 16-19)
            db 0                        ; base high

            ; 16-bit data segment
            dw 0FFFFh                   ; limit (bits 0-15) = 0xFFFFF
            dw 0                        ; base (bits 0-15) = 0x0
            db 0                        ; base (bits 16-23)
            db 10010010b                ; access (present, ring 0, data segment, executable, direction 0, writable)
            db 00001111b                ; granularity (1b pages, 16-bit pmode) + limit (bits 16-19)
            db 0                        ; base high

g_GDTDesc:  dw g_GDTDesc - g_GDT - 1    ; limit = size of GDT
            dd g_GDT                    ; address of GDT

align 8
IDTDescriptor:
            dw 256*8-1                  ; limit
            dd IDT_BASE                 ; base

g_BootDrive: db 0
g_BootPartitionSeg: dw 0
g_BootPartitionOff: dw 0