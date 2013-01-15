; ex07.asm
        ORG  $0800
Sum:    RMB  1
	
        ORG  $C000
Start:  clr  Sum
        ldx  #List
Top:    lda  0,X+
        beq  Done
        adda Sum
        sta  Sum
        bra  Top
Done:   bra  Done
	
List:   FCB $12,$08,$23,$00
        ORG $FFFE
	FDB Start
