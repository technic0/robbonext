;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;  RobboNext - copy a 16x16 tile into layer 2
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

SECTION code_user

PUBLIC _blit_tile

; void blit_tile(const u8* src, u8* dst)
; src: 256 bytes, 16 rows of 16 pixels
; dst: top left pixel in layer 2, rows are 256 bytes apart
_blit_tile:
    pop bc                  ; return address
    pop hl                  ; src
    pop de                  ; dst
    push de
    push hl
    push bc                 ; restore stack, the caller cleans up
    ld a, 16
blit_row:
    ldi
    ldi
    ldi
    ldi
    ldi
    ldi
    ldi
    ldi
    ldi
    ldi
    ldi
    ldi
    ldi
    ldi
    ldi
    ldi
    add de, 240             ; next row of layer 2
    dec a
    jr nz, blit_row
    ret
