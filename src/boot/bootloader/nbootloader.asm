bits 16

section .entry

extern __bss_start
extern __end
extern _init

extern Start
global entry

entry:
    cli

    ; save boot drive
    mov [g_BootDrive], dl
    mov [g_BootPartitionOff], si
    mov [g_BootPartitionSeg], di

    ; setup stack
    mov ax, ds
    mov ss, ax
    mov sp, 0xFFF0
    mov bp, sp

    ; switch to protected mode
    call EnableA20          ; 2 - Enable A20 gate
    call LoadGDT            ; 3 - Load GDT

    ; 4 - set protection enable flag in CR0
    mov eax, cr0
    or al, 1
    mov cr0, eax

    ; 5 - far jump into protected mode
    jmp dword 08h:.pmode

.pmode:
    ; we are now in protected mode!
    [bits 32]
    
    ; 6 - setup segment registers
    mov ax, 0x10
    mov ds, ax
    mov ss, ax
   
    ; clear bss (uninitialized data)
    mov edi, __bss_start
    mov ecx, __end
    sub ecx, edi
    mov al, 0
    cld
    rep stosb

    ; call global constructors
    call _init

    ; expect boot drive in dl, send it as argument to cstart function
    mov dx, [g_BootPartitionSeg]
    shl edx, 16
    mov dx, [g_BootPartitionOff]
    push edx

    xor edx, edx
    mov dl, [g_BootDrive]
    push edx
    call Start

    cli
    hlt


%include "src/boot/bootloader/include/GDT.inc"

section .data
g_BootDrive: db 0
g_BootPartitionSeg: dw 0
g_BootPartitionOff: dw 0