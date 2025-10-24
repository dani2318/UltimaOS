;
; Convert LBA address to CHS address
;   Parameters:
;       -   ax: LBA address
;   Returns:
;       -   cx [bits 0-5]:  sector number
;       -   cx [bits 6-15]: cylinders
;       -   dh :            head
lba_to_chs:

    push ax
    push dx

    xor dx, dx                              ; dx = 0
    div word [bdb_sectors_per_track]        ; ax = LBA / bdb_sectors_per_track

    inc dx                                  ; dx = (LBA % bdb_sectors_per_track) + 1 = Sector
    mov cx, dx                              ; cx = sector

    xor dx, dx                              ; dx = 0
    div word [bdb_heads]                    ; ax = (LBA / bdb_sectors_per_track) / heads = Cylinder
                                            ; dx = (LBA / bdb_sectors_per_track) % heads = Head

    mov dh, dl                              ; dh = head
    mov ch, al                              ; hc = cylinder (lower 8 bits)
    shl ah, 6
    or cl, ah                               ; put upper 2 bits of cylinder in cl

    pop ax
    mov dl, al                              ; restore DL
    pop ax
    ret
