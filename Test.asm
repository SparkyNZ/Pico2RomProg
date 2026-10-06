.org $0000    ; Relative offset within the 32KB ROM image ($8000 physical)

physical_start = $8000

; --- 1. Character ROM (0x0000 - 0x0FFF in file -> $8000 CPU) ---
.incbin "characters.901225-01.bin"    ; 4,096 bytes ($1000 hex)

; --- 2. Pad from 0x1000 up to 0x2000 (CPU $A000) ---
;.res $2000 - *, $EA
.res $2000 - *, $FF

.org $A000		

black  = $00
white  = $01
red    = $02
cyan   = $03
purple = $04
green  = $05
blue   = $06
yellow = $07

; --- 3. Code Start (0x2000 in file -> $A000 CPU) ---
start:
    ; Clear C000-CFFF (limited by D0xx)
    LDA #$00
    STA FILLPTR_L       ; Set pointer low byte to $00
    LDA #$C0
    STA FILLPTR_H       ; Set pointer high byte to $C0
    LDA #' '            ; Fill value
    LDX #$D0            ; Hi byte limit, so D000-1 (CFFF)
    JSR FillMemory

    LDA #$00
    STA FILLPTR_L       ; Set pointer low byte to $00
    LDA #$D0
    STA FILLPTR_H       ; Set pointer high byte to $D0
    LDA #$01            ; Fill value
    LDX #$E0            ; Hi byte limit, so E000-1 (DFFF)
    JSR FillMemory

; Skip the TEXT print..
;    JMP loop

; Print a null terminated string    
DoString1:
    LDA #yellow
    STA CHAR_COLOUR

    LDA #<PaulString
    STA <STRINGPTR_L
    LDA #>PaulString
    STA <STRINGPTR_H
    
    LDA #<$C000
    STA <SCREENPTR_L
    LDA #>$C000
    STA <SCREENPTR_H
    
    LDA #<$D000
    STA <COLOURPTR_L
    LDA #>$D000
    STA <COLOURPTR_H
    
    JSR PrintStringZ
    
DoString2:
    LDA #green
    STA CHAR_COLOUR

    ; Print alternate string
    LDA #<PaulString2
    STA <STRINGPTR_L
    LDA #>PaulString2
    STA <STRINGPTR_H
    
    LDA #<$C000
    STA <SCREENPTR_L
    LDA #>$C000
    STA <SCREENPTR_H
    
    LDA #<$D000
    STA <COLOURPTR_L
    LDA #>$D000
    STA <COLOURPTR_H
    
    JSR PrintStringZ
    
    JMP DoString1
    
loop:
    JMP loop

TEMP_REG1   = $DF

CHAR_COLOUR = $E0

SCREENPTR_L = $FE
SCREENPTR_H = $FF

STRINGPTR_L = $FC
STRINGPTR_H = $FD

COLOURPTR_L = $FA
COLOURPTR_H = $FB

PaulString:   .asciiz "HELLO PAUL"
;PaulString:   .asciiz "HEXXO PAUL"
PaulString2:   .asciiz "GOODBYE ME"
  
; Print a null terminated string
PrintStringZ:
    LDY #$00
ReadChar:    
    LDA (STRINGPTR_L),Y
    CMP #$00
    BEQ EndString
    
    CMP #' '
    BEQ NoSub
    ; Convert ASCII 'A' to CBM 'A'
    SEC
    SBC #'A'-1
NoSub:    
    
    JSR Delay1Sec
    
    ; Write char into screen
    STA (SCREENPTR_L),Y
    
    ;JSR Delay1Sec
    
    LDA CHAR_COLOUR
    STA (COLOURPTR_L),Y
    INY
    
    ; Safety - if string is 255 long or more, stop
    CPY #$00
    BEQ EndString
    
    JMP ReadChar
EndString:
    RTS
  
; Define zero-page pointer locations
FILLPTR_L = $FA
FILLPTR_H = $FB

;-------------------------------------------------------------------------------------------------------------
; FillMemory
; Inputs:
;   A = Fill byte value
;   X = End page high byte (stops when FILLPTR_H reaches this value)
;   FILLPTR_L / FILLPTR_H = Starting address
;-------------------------------------------------------------------------------------------------------------
FillMemory:
    PHA                 ; Save fill byte to stack
fill_loop:
    LDY #$00            ; Reset Y for page iteration
fill_page_loop:
    PLA                 ; Retrieve fill byte
    PHA                 ; Keep a copy on stack
    STA (FILLPTR_L),Y   ; Write fill byte to memory
    INY                 ; Advance index
    BNE fill_page_loop  ; Loop 256 bytes per page

    INC FILLPTR_H       ; Step to next page
    CPX FILLPTR_H       ; Compare target end page with current high byte
    BNE fill_loop       ; Continue if end page not reached

    PLA                 ; Clean up stack
    RTS

;-------------------------------------------------------------------------------------------------------------
; Delay1Sec
; 3-stage nested loop delay for a 1 MHz 6502 CPU (~1,000,000 cycles)
; Preserves A, X, and Y registers
;-------------------------------------------------------------------------------------------------------------
Delay1Sec:
    PHA                     ; Save registers on stack
    TXA
    PHA
    TYA
    PHA

    ;LDA #13                 ; Outer loop counter (~1,000,000 total cycles)
    ;LDA #6                 ; Outer loop counter 
    LDA #4
    STA TEMP_REG1

OuterLoop:
    LDX #200                ; Middle loop counter
MiddleLoop:
    LDY #127                ; Inner loop counter
InnerLoop:
    DEY                     ; 2 cycles
    BNE InnerLoop           ; 3 cycles (2 when no branch)
                            ; Inner loop takes (127 * 5) - 1 = 634 cycles

    DEX                     ; 2 cycles
    BNE MiddleLoop          ; 3 cycles
                            ; Middle loop takes 200 * (634 + 5) - 1 = 127,799 cycles

    DEC TEMP_REG1           ; 5 cycles (Zero Page)
    BNE OuterLoop           ; 3 cycles
                            ; Outer loop takes 13 * (127,799 + 8) - 1 = 1,661,490 -> tweak values below

    PLA                     ; Restore registers
    TAY
    PLA
    TAX
    PLA
    RTS
    
; --- 4. Pad from current location up to 0x7FFC (CPU $FFFC) ---
;.res $7FFC - *, $EA
;.res $FFFC - *, $EA
.res $FFFA - *, $FF

; --- 5. 6502 Reset Vectors (0x7FFC - 0x7FFF in file -> $FFFC CPU) ---
;.word start + physical_start    ; $7FFC/$7FFD: Reset vector points to 'start' label
;.word start + physical_start    ; $7FFE/$7FFF: IRQ/BRK vector

.word start ; $7FFA/$7FFB: Reset vector points to 'start' label
.word start ; $7FFC/$7FFD: Reset vector points to 'start' label
.word start; $7FFE/$7FFF: IRQ/BRK vector
