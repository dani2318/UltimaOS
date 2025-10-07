%include "ProtectedMode.inc"


; int ASMCALL E820GetNextBlock(E820MemoryBlock* block, uint32_t* continuationId);
E820Signature equ 0x53D4150
global E820GetNextBlock
E820GetNextBlock:
    [bits 32]

    push ebp
    mov ebp, esp

    x86_EnterRealMode

    push ebx
    push ecx
    push edx
    push esi
    push ds
    push es

    LinearToSegOffset [bp + 8], es, edi, di
    LinearToSegOffset [bp + 12], ds, esi, si

    mov ebx, ds:[si]

    mov eax, 0xE820
    mov edx, E820Signature
    mov ecx, 24

    int 0x15

    cmp eax, E820Signature
    jne .error

.ifOK:

    mov eax, ecx
    mov ds:[si], ebx

    jmp .EndIF

.error:
    mov eax, -1

.EndIF:

    pop es
    pop ds
    pop esi
    pop edx
    pop ecx
    pop ebx

    push eax

    x86_EnterProtectedMode

    pop eax

    mov esp, ebp
    pop ebp
    ret