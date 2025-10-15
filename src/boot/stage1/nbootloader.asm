bits 16

%define ENDL 0x0D, 0x0A

%define fat12 1
%define fat16 2
%define fat32 3
%define ext2  4                                     ; Unimplemented

;
; FAT12 header
;

section .fsjump

jmp short start
nop

%include "src/boot/stage1/includes/fsheaders.inc"

section .entry
global start
start:

    ; move partition entry from MBR to a different location so we
    ; don't overwrite it (which is passed through DS:SI)
    mov ax, PARTITION_ENTRY_SEGMENT
    mov es, ax
    mov di, PARTITION_ENTRY_OFFSET
    mov cx, 16
    rep movsb

    ; setup data segments
    mov ax, 0           ; can't set ds/es directly
    mov ds, ax
    mov es, ax

    ; setup stack
    mov ss, ax
    mov sp, 0x7C00              ; stack grows downwards from where we are loaded in memory

    ; some BIOSes might start us at 07C0:0000 instead of 0000:7C00, make sure we are in the
    ; expected location
    push es
    push word .after
    retf

.after:

    ; read something from floppy disk
    ; BIOS should set DL to drive number
    mov [ebr_drive_number], dl

    ; check extensions present
    mov ah, 41h
    mov bx, 0x55AA
    stc
    int 13h

    jc .no_disk_extensions
    cmp bx, 0xAA55
    jne .no_disk_extensions

    mov byte [have_extensions], 1
    jmp .after_disk_extensions_check


.no_disk_extensions:
    mov byte [have_extensions], 0

.after_disk_extensions_check:
    ;load stage2
    mov si, stage2_location

    mov ax, STAGE2_LOAD_SEGMENT         ; set segment registers
    mov es, ax

    mov bx, STAGE2_LOAD_OFFSET

.loop:
    mov eax, [si]
    add si, 4
    mov cl, [si]
    inc si

    cmp eax, 0
    je .read_finish

    call disk_read

    xor ch, ch
    shl cx, 5
    mov di, es
    add di, cx
    mov es, di

    jmp .loop


.read_finish:

    ; jump to our stage2
    mov dl, [ebr_drive_number]          ; boot device in dl
    mov si, PARTITION_ENTRY_OFFSET
    mov di, PARTITION_ENTRY_SEGMENT

    mov ax, STAGE2_LOAD_SEGMENT         ; set segment registers
    mov ds, ax
    mov es, ax

    jmp STAGE2_LOAD_SEGMENT:STAGE2_LOAD_OFFSET

    jmp 0FFFFh:0                        ; jump to beginning of BIOS, should reboot

    cli                                 ; disable interrupts, this way CPU can't get out of "halt" state
    hlt

%include "src/boot/stage1/includes/algo.inc"

%include "src/boot/stage1/includes/disk.inc"

section .text
;
; Error handlers
;

floppy_error:
    jmp 0FFFFh:0                ; jump to beginning of BIOS, should reboot


%include "src/boot/stage1/includes/data.inc"

