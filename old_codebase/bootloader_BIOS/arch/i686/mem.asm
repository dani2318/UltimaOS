%include "src/boot/bootloader/arch/i686/ProtectedMode.inc"

section .data
; Global variables for E820GetNextBlock
E820GetNextBlock.block: dd 0
E820GetNextBlock.continuationId: dd 0
E820GetNextBlock.result: dd 0
E820GetNextBlock.contId: dd 0

section .text

; int ASMCALL E820GetNextBlock(E820MemoryBlock* block, uint32_t* continuationId);
E820Signature equ 0x534D4150

global E820GetNextBlock
E820GetNextBlock:
    [bits 32]
    push ebp
    mov ebp, esp
    
    ; Save parameters
    mov eax, [ebp + 8]
    mov [E820GetNextBlock.block], eax
    mov eax, [ebp + 12]
    mov [E820GetNextBlock.continuationId], eax
    
    x86_EnterRealMode
    [bits 16]
    
    push ebx
    push ecx
    push edx
    push esi
    push ds
    push es
    push di
    
    ; Load continuation ID
    LinearToSegOffset [E820GetNextBlock.continuationId], ds, esi, si
    mov ebx, [ds:si]
    mov [E820GetNextBlock.contId], ebx
    
    ; Set up destination for E820 block
    LinearToSegOffset [E820GetNextBlock.block], es, edi, di
    
    mov eax, 0xE820
    mov edx, E820Signature
    mov ecx, 24
    int 0x15
    
    ; Check if successful
    cmp eax, E820Signature
    jne .error
    
.ifOK:
    ; Save continuation ID back
    mov [E820GetNextBlock.contId], ebx
    LinearToSegOffset [E820GetNextBlock.continuationId], ds, esi, si
    mov ebx, [E820GetNextBlock.contId]
    mov [ds:si], ebx
    
    ; Return number of bytes
    movzx eax, cx
    mov [E820GetNextBlock.result], eax
    jmp .EndIF
    
.error:
    mov dword [E820GetNextBlock.result], -1
    
.EndIF:
    pop di
    pop es
    pop ds
    pop esi
    pop edx
    pop ecx
    pop ebx
    
    x86_EnterProtectedMode
    [bits 32]
    
    mov eax, [E820GetNextBlock.result]
    mov esp, ebp
    pop ebp
    ret