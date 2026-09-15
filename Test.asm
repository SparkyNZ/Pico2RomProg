.org $0000    ; Relative offset within the 32KB ROM image ($8000 physical)
physical_start = $8000

; --- 1. Character ROM (0x0000 - 0x0FFF in file -> $8000 CPU) ---
.incbin "characters.901225-01.bin"    ; 4,096 bytes ($1000 hex)

; --- 2. Pad from 0x1000 up to 0x2000 (CPU $A000) ---
.res $2000 - *, $EA

; --- 3. Code Start (0x2000 in file -> $A000 CPU) ---
start:
    LDA #$01
    STA $C000
    LDA #$05
    STA $D000
    JMP start

; --- 4. Pad from current location up to 0x7FFC (CPU $FFFC) ---
.res $7FFC - *, $EA

; --- 5. 6502 Reset Vectors (0x7FFC - 0x7FFF in file -> $FFFC CPU) ---
.word start + physical_start    ; $7FFC/$7FFD: Reset vector points to 'start' label
.word start + physical_start    ; $7FFE/$7FFF: IRQ/BRK vector