%include "src/boot/stage2/arch/i686/ProtectedMode.inc"


global BIOSDiskExtensionPresent
BIOSDiskExtensionPresent:
    [bits 32]

    push ebp
    mov ebp, esp
    
    x86_EnterRealMode

    [bits 16]

    push bx

    mov ah, 41h
    mov bx, 0x55AA
    mov dl, [bp + 8]            ; Drive number

    stc
    int 13h

    jc .Error

    cmp bx, 0xAA55
    jne .Error

    mov eax, 1
    jmp .EndIF    

.Error:
    mov eax, 0

.EndIF:
    pop bx

    push eax

    x86_EnterProtectedMode

    [bits 32]

    pop eax

    mov esp, ebp
    pop ebp
    ret

global BIOSDiskExtendedRead
BIOSDiskExtendedRead:
    [bits 32]

    push ebp
    mov ebp, esp
    
    x86_EnterRealMode

    [bits 16]

    push ds
    push esi

    mov ah, 42h
    mov dl, [bp + 8]            ; Drive number
    LinearToSegOffset [bp + 12], ds, esi, si

    stc
    int 13h

    mov eax, 1
    sbb eax, 0

    pop esi
    pop ds

    push eax

    x86_EnterProtectedMode

    [bits 32]

    pop eax

    mov esp, ebp
    pop ebp
    ret



global BIOSDiskExtendedGetDriveParams
BIOSDiskExtendedGetDriveParams:
    [bits 32]

    push ebp
    mov ebp, esp
    
    x86_EnterRealMode

    [bits 16]

    push ds
    push esi

    mov ah, 48h
    mov dl, [bp + 8]            ; Drive number
    LinearToSegOffset [bp + 12], ds, esi, si

    stc
    int 13h

    mov eax, 1
    sbb eax, 0

    pop esi
    pop ds

    push eax

    x86_EnterProtectedMode

    [bits 32]

    pop eax

    mov esp, ebp
    pop ebp
    ret



global BIOSDiskGetDriveParams
BIOSDiskGetDriveParams:
    [bits 32]

    ; make new call frame
    push ebp             ; save old call frame
    mov ebp, esp         ; initialize new call frame

    x86_EnterRealMode

    [bits 16]

    ; save regs
    push es
    push bx
    push esi
    push di

    ; call int13h
    mov dl, [bp + 8]    ; dl - disk drive
    mov ah, 08h
    mov di, 0           ; es:di - 0000:0000
    mov es, di
    stc
    int 13h

    ; out params
    mov eax, 1
    sbb eax, 0

    ; drive type from bl
    LinearToSegOffset [bp + 12], es, esi, si
    mov [es:si], bl

    ; cylinders
    mov bl, ch          ; cylinders - lower bits in ch
    mov bh, cl          ; cylinders - upper bits in cl (6-7)
    shr bh, 6
    inc bx

    LinearToSegOffset [bp + 16], es, esi, si
    mov [es:si], bx

    ; sectors
    xor ch, ch          ; sectors - lower 5 bits in cl
    and cl, 3Fh
    
    LinearToSegOffset [bp + 20], es, esi, si
    mov [es:si], cx

    ; heads
    mov cl, dh          ; heads - dh
    inc cx

    LinearToSegOffset [bp + 24], es, esi, si
    mov [es:si], cx

    ; restore regs
    pop di
    pop esi
    pop bx
    pop es

    ; return

    push eax

    x86_EnterProtectedMode

    [bits 32]

    pop eax

    ; restore old call frame
    mov esp, ebp
    pop ebp
    ret

global BIOSDiskReset
BIOSDiskReset:
    [bits 32]
    push ebp
    mov ebp, esp

    x86_EnterRealMode

    [bits 16]

    mov ah, 0
    mov dl, [bp + 8]
    stc
    int 13h

    mov ax, 1
    sbb ax, 0

    push eax

    x86_EnterProtectedMode

    [bits 32]

    pop eax

    mov esp, ebp
    pop ebp
    ret



global BIOSDiskRead
BIOSDiskRead:
    [bits 32]

    push ebp
    mov ebp, esp
    
    x86_EnterRealMode

    [bits 16]

    push ebx
    push es

    mov dl, [bp + 8]

    mov ch, [bp + 12]
    mov cl, [bp + 13]
    shl cl, 6

    mov al, [bp + 16]
    and al, 3Fh
    or cl, al
    
    mov dh, [bp + 20]

    mov al, [bp + 24]

    LinearToSegOffset [bp + 28], es, ebx, bx

    mov ah, 02h
    stc
    int 13h

    mov ax, 1
    sbb ax, 0
    
    pop es
    pop ebx

    push eax

    x86_EnterProtectedMode

    [bits 32]

    pop eax

    mov esp, ebp
    pop ebp
    ret


