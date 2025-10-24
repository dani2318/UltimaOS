section .rodata
    file_stage2_bin:        db 'STAGE2  BIN'

section .data

    have_extensions:        db 0
    extensions_dap:
        .size:              db 10h
                            db 0
        .count:             dw 0
        .offset:            dw 0
        .segment:           dw 0
        .lba:               dq 0


    STAGE2_LOAD_SEGMENT     equ 0x0
    STAGE2_LOAD_OFFSET      equ 0x500

    PARTITION_ENTRY_SEGMENT equ 0x2000
    PARTITION_ENTRY_OFFSET  equ 0x0

section .data
    global stage2_location
    stage2_location:        times 30 db 0

section .bss
    buffer:
