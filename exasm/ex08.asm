; ex08.asm - a straight insert sort
         ORG $0800
NNx:     RMB  1
SxList:  RMB  4

         ORG $C000
Start:   ldy #UxList
         clr  NNx

Top1:    cmpy #UxListx ; check if done
         bhs  Done     ;
         lda  0,Y      ; get next number
         
         ldb  NNx      ; initialize
         ldx  #SxList  ; the inner
         abx           ; loop
         
Top2:    cmpx #SxList
         beq  Pass
         cmpa -1,X
         bhs  Pass

         ldb  ,-X
         stb  1,X
         decb
         bra  Top2

Pass:    sta  0,X
         leay 1,Y
         inc  NNx
         bra  Top1
         
Done:    bra Done
UxList:  FCB $34,$12,$78,$56
UxListx:
         ORG $FFFE
         FDB  Start
         
