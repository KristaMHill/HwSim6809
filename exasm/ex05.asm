; ex05.asm
        ORG  $C000
Start:  lds  #$1000
	ldd  #$1234
	ldx  #$5678
	ldy  #$9ABC
	pshs A,B,X,Y
        ldd  #$00
	ldx  #$00
	ldy  #$00
	puls A,B,X,Y
Done:	bra  Done

	ORG  $FFFE
	FDB  Start
	