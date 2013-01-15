; ex4.asm
        ORG  $0800  ; Start of RAM
Sum:    RMB  2

        ORG  $C000  ; Start of ROM
Start:  lds  #$1000
        ldx  #$00
        ldb  #$04
        
Top:    abx
        decb
        bne  Top
        stx  Sum
        
Done:   bra  Done
        ORG  $FFFE
        FDB  Start
