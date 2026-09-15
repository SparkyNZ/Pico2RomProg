.org $0000

; Fill memory from $0000 to $7FFF with $EA (NOP)
.res $8000, $EA

;start will be at $8000 - no need for ORG
start:
    LDA #$01
    STA $C000
    LDA #$05
    STA $D000
    JMP start

; Pad the remaining space from the end of the program up to the vectors at $FFFC
.res $FFFC - *, $EA

; 6502 Reset Vector at $FFFC
.word start    ; Automatically writes low byte then high byte
.word start    ; $FFFE: IRQ/BRK vector