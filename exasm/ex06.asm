; ex06.asm
	ORG $C000
Start:	lda #$34
	tfr A,B
	lda #$12
	tfr A,CC
	tfr B,DP
	exg A,B
	exg CC,DP
Done:	bra Done
	ORG $FFFE
	FDB Start