%include "src/boot/stage2/arch/i686/ProtectedMode.inc"

section .text

global BIOSDiskExtensionPresent
BIOSDiskExtensionPresent:
    [bits 32]
    push ebp
    mov ebp, esp
    
    ; Save the drive parameter
    movzx eax, byte [ebp + 8]
    mov [.drive_number], al
    
    x86_EnterRealMode
    [bits 16]
    
    push bx
    mov ah, 41h
    mov bx, 0x55AA
    mov dl, [.drive_number]
    stc
    int 13h
    
    ; Save result in a global variable, not on stack
    jc .Error
    cmp bx, 0xAA55
    jne .Error
    mov byte [.result], 1
    jmp .EndIF    
.Error:
    mov byte [.result], 0
.EndIF:
    pop bx
    
    x86_EnterProtectedMode
    [bits 32]
    
    ; Load result from global variable
    movzx eax, byte [.result]
    
    mov esp, ebp
    pop ebp
    ret

section .data
    .drive_number: db 0
    .result: db 0

section .text
section .data
; Global variables for BIOSDiskExtendedRead
BIOSDiskExtendedRead.drive: db 0
BIOSDiskExtendedRead.buffer: dd 0
BIOSDiskExtendedRead.result: dd 0

; Global variables for BIOSDiskExtendedGetDriveParams
BIOSDiskExtendedGetDriveParams.drive: db 0
BIOSDiskExtendedGetDriveParams.buffer: dd 0
BIOSDiskExtendedGetDriveParams.result: dd 0

; Global variables for BIOSDiskGetDriveParams
BIOSDiskGetDriveParams.drive: db 0
BIOSDiskGetDriveParams.driveType: dd 0
BIOSDiskGetDriveParams.cylinders: dd 0
BIOSDiskGetDriveParams.sectors: dd 0
BIOSDiskGetDriveParams.heads: dd 0
BIOSDiskGetDriveParams.result: dd 0

; Global variables for BIOSDiskReset
BIOSDiskReset.drive: db 0
BIOSDiskReset.result: dd 0

; Global variables for BIOSDiskRead
BIOSDiskRead.drive: db 0
BIOSDiskRead.cylinder: db 0
BIOSDiskRead.sector: db 0
BIOSDiskRead.head: db 0
BIOSDiskRead.count: db 0
BIOSDiskRead.buffer: dd 0
BIOSDiskRead.result: dd 0

section .text

global BIOSDiskExtendedRead
BIOSDiskExtendedRead:
    [bits 32]
    push ebp
    mov ebp, esp
    
    ; Save parameters
    mov al, [ebp + 8]
    mov [BIOSDiskExtendedRead.drive], al
    mov eax, [ebp + 12]
    mov [BIOSDiskExtendedRead.buffer], eax
    
    x86_EnterRealMode
    [bits 16]
    
    push ds
    push esi
    mov ah, 42h
    mov dl, [BIOSDiskExtendedRead.drive]
    LinearToSegOffset [BIOSDiskExtendedRead.buffer], ds, esi, si
    stc
    int 13h
    mov eax, 1
    sbb eax, 0
    mov [BIOSDiskExtendedRead.result], eax
    pop esi
    pop ds
    
    x86_EnterProtectedMode
    [bits 32]
    
    mov eax, [BIOSDiskExtendedRead.result]
    mov esp, ebp
    pop ebp
    ret

global BIOSDiskExtendedGetDriveParams
BIOSDiskExtendedGetDriveParams:
    [bits 32]
    push ebp
    mov ebp, esp
    
    ; Save parameters
    mov al, [ebp + 8]
    mov [BIOSDiskExtendedGetDriveParams.drive], al
    mov eax, [ebp + 12]
    mov [BIOSDiskExtendedGetDriveParams.buffer], eax
    
    x86_EnterRealMode
    [bits 16]
    
    push ds
    push esi
    mov ah, 48h
    mov dl, [BIOSDiskExtendedGetDriveParams.drive]
    LinearToSegOffset [BIOSDiskExtendedGetDriveParams.buffer], ds, esi, si
    stc
    int 13h
    mov eax, 1
    sbb eax, 0
    mov [BIOSDiskExtendedGetDriveParams.result], eax
    pop esi
    pop ds
    
    x86_EnterProtectedMode
    [bits 32]
    
    mov eax, [BIOSDiskExtendedGetDriveParams.result]
    mov esp, ebp
    pop ebp
    ret

global BIOSDiskGetDriveParams
BIOSDiskGetDriveParams:
    [bits 32]
    push ebp
    mov ebp, esp
    
    ; Save parameters
    mov al, [ebp + 8]
    mov [BIOSDiskGetDriveParams.drive], al
    mov eax, [ebp + 12]
    mov [BIOSDiskGetDriveParams.driveType], eax
    mov eax, [ebp + 16]
    mov [BIOSDiskGetDriveParams.cylinders], eax
    mov eax, [ebp + 20]
    mov [BIOSDiskGetDriveParams.sectors], eax
    mov eax, [ebp + 24]
    mov [BIOSDiskGetDriveParams.heads], eax
    
    x86_EnterRealMode
    [bits 16]
    
    push es
    push bx
    push esi
    push di
    
    mov dl, [BIOSDiskGetDriveParams.drive]
    mov ah, 08h
    mov di, 0
    mov es, di
    stc
    int 13h
    
    mov eax, 1
    sbb eax, 0
    mov [BIOSDiskGetDriveParams.result], eax
    
    ; Drive type
    LinearToSegOffset [BIOSDiskGetDriveParams.driveType], es, esi, si
    mov [es:si], bl
    
    ; Cylinders
    mov bl, ch
    mov bh, cl
    shr bh, 6
    inc bx
    LinearToSegOffset [BIOSDiskGetDriveParams.cylinders], es, esi, si
    mov [es:si], bx
    
    ; Sectors
    xor ch, ch
    and cl, 3Fh
    LinearToSegOffset [BIOSDiskGetDriveParams.sectors], es, esi, si
    mov [es:si], cx
    
    ; Heads
    mov cl, dh
    inc cx
    LinearToSegOffset [BIOSDiskGetDriveParams.heads], es, esi, si
    mov [es:si], cx
    
    pop di
    pop esi
    pop bx
    pop es
    
    x86_EnterProtectedMode
    [bits 32]
    
    mov eax, [BIOSDiskGetDriveParams.result]
    mov esp, ebp
    pop ebp
    ret

global BIOSDiskReset
BIOSDiskReset:
    [bits 32]
    push ebp
    mov ebp, esp
    
    mov al, [ebp + 8]
    mov [BIOSDiskReset.drive], al
    
    x86_EnterRealMode
    [bits 16]
    
    mov ah, 0
    mov dl, [BIOSDiskReset.drive]
    stc
    int 13h
    mov ax, 1
    sbb ax, 0
    movzx eax, ax
    mov [BIOSDiskReset.result], eax
    
    x86_EnterProtectedMode
    [bits 32]
    
    mov eax, [BIOSDiskReset.result]
    mov esp, ebp
    pop ebp
    ret

global BIOSDiskRead
BIOSDiskRead:
    [bits 32]
    push ebp
    mov ebp, esp
    
    ; Save all parameters
    mov al, [ebp + 8]
    mov [BIOSDiskRead.drive], al
    mov al, [ebp + 12]
    mov [BIOSDiskRead.cylinder], al
    mov al, [ebp + 16]
    mov [BIOSDiskRead.sector], al
    mov al, [ebp + 20]
    mov [BIOSDiskRead.head], al
    mov al, [ebp + 24]
    mov [BIOSDiskRead.count], al
    mov eax, [ebp + 28]
    mov [BIOSDiskRead.buffer], eax
    
    x86_EnterRealMode
    [bits 16]
    
    push ebx
    push es
    
    mov dl, [BIOSDiskRead.drive]
    mov ch, [BIOSDiskRead.cylinder]
    mov cl, [BIOSDiskRead.sector]
    shl cl, 6
    mov al, [BIOSDiskRead.sector]
    and al, 3Fh
    or cl, al
    mov dh, [BIOSDiskRead.head]
    mov al, [BIOSDiskRead.count]
    
    LinearToSegOffset [BIOSDiskRead.buffer], es, ebx, bx
    mov ah, 02h
    stc
    int 13h
    mov ax, 1
    sbb ax, 0
    movzx eax, ax
    mov [BIOSDiskRead.result], eax
    
    pop es
    pop ebx
    
    x86_EnterProtectedMode
    [bits 32]
    
    mov eax, [BIOSDiskRead.result]
    mov esp, ebp
    pop ebp
    ret

