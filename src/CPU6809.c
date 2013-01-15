/*************************************************************************
 * CPU6809.c - Part of HwSim6809 project
 * This module models the CPU proper for such a system
 * Krista Hill - kmhill@hartford.edu - 01/15/2013 - version 0.1.0
 * Krista Hill - kmhill@hartford.edu - 12/31/2012 - File is created
 * The following will build and test this code.  The gcc compilier and
 * lua 5.1 interpreter were used
 * $ gcc -c MemSys.c -Wall -I "/usr/include/lua5.1/"
 * $ gcc -c S19FileRead.c -Wall -I "/usr/include/lua5.1/"
 * $ gcc -shared -o simcore00.so MemSys.o S19FileRead.o CPU6809.o
 * $ lua test03.lua
 * Comments:
 * Not sure of H flag in subtraction
 * Check the branch instructions
 * Finish the branch instructions
 ************************************************************************/
#include "includes.h"
#define BSE(arg)          ((arg < 0x80)? (arg) : 0xFF00+(arg))
#define BRT(val)          ((BSE(val)+addx)&0xFFFF)
#define IncPCreg(nn)      (PCreg = (PCreg+(nn))&0xFFFF)
#define IncAddx(nn)       (addx  = (addx+(nn))&0xFFFF)
#define AddEx(nd)         AddExFnct(PCreg,nd)
#define RdByte(addx)      RdByteFnct(L,addx)
#define RdWord(addx)      RdWordFnct(L,addx)
#define StByte(addx,bval) StByteFnct(L,addx,bval)
#define StWord(addx,bval) StWordFnct(L,addx,bval)
#define FxByte()          RdByteFnct(L,PCreg); IncPCreg(1)
#define DxByte()          MemsReadByte(addx,&err); if (err){goto DxErr; }IncAddx(1)
#define FxWord()          FxWordFnct(L)
#define DxWord()          DxWordFnct(&addx,&err); if (err){goto DxErr;}
#define CAT(aval,bval)    (((aval)<<8)+(bval))



// Prototypes for functions in this module
UWORD AddBytesCx(UWORD aval, UWORD bval, UWORD flag);
UWORD AddExFnct(UWORD addx,UWORD nd);
UWORD AddWords(UWORD aval, UWORD bval);
UWORD ASLfnct(UWORD aval);
UWORD ASRfnct(UWORD aval);
void  AssignFlagChar(UWORD flag, char *pntr, char AssertFlag);
UWORD DAAfunct(UWORD aval);
UWORD DecByteFnct(UWORD bval);
UWORD DxWordFnct(UWORD *paddx, UWORD *err);
void  EXGfnct(lua_State *L,UWORD sxdx);
void  Flag2NZ0(UWORD aval, UWORD bval);
void  FlagBNZ0(UWORD bval);
void  FlagBNZ01(UWORD bval);
void  FlagClr(void);
void  FlagWNZ0(UWORD bval);
UWORD FxWordFnct(lua_State *L);
UWORD GetCreg(void);
UWORD GetDreg(void);
ULONG GetRx(UWORD sx);
UWORD IncByteFnct(UWORD bval);
char *IndPickRx(UWORD nd);
UWORD INDX(lua_State *L);
UWORD IndxGetReg(UWORD nd);
void  IndxPutReg(UWORD nd, UWORD rval);
char *INDY(lua_State *L, UWORD *paddx);
void  Intfnct(lua_State *L, UWORD SwiType);
UWORD LSRfnct(UWORD aval);
UWORD NegByteFnct(UWORD bval);
char *PickTEchar(UWORD rx);
char *PshFlags(char *bx, UWORD flags, char MNchar, char SUchar);
void  PSHfnct(lua_State *L, ULONG *pstk, UWORD ostk, UWORD flags);
char *PulFlags(char *bx, UWORD flags, char MNchar, char SUchar);
void  PULfnct(lua_State *L, ULONG *pstk, ULONG *ostk, UWORD flags);
void  PutCreg(UWORD cval);
void  PutDreg(UWORD bval);
void  PutRx(ULONG val, UWORD dx);
UWORD RdByteFnct(lua_State *L,UWORD addx);
UWORD RdWordFnct(lua_State *L,UWORD addx);
UWORD ROLfnct(UWORD aval);
UWORD RORfnct(UWORD aval);
void  RTIfnct(lua_State *L);
void  StByteFnct(lua_State *L,UWORD addx, UWORD bval);
void  StoreError(lua_State *L,UWORD addx,UWORD bval);
void  StWordFnct(lua_State *L,UWORD addx, UWORD bval);
UWORD SubBytesCx(UWORD aval, UWORD bval, UWORD flag);
UWORD SubWords(UWORD aval, UWORD bval);
void  TFRfnct(lua_State *L,UWORD sxdx);

// Global variables for this module
static ULONG Areg, Breg;
static ULONG Sreg, Ureg;
static ULONG Xreg, Yreg;
static ULONG PCreg,PCtmp;
static ULONG DPreg;
static UWORD CC7Eflag, CC6Fflag, CC5Hflag, CC4Iflag;
static UWORD CC3Nflag, CC2Zflag, CC1Vflag, CC0Cflag;
static UWORD NMI_ARMED_FLAG;

/*
int main()
{
  int ix;
  CPU_Reset(NULL);
  for (ix = 0; ix < 8; ix++){
    CPU_ReportRegs();
  }
  CPU_ReportRegs();
  return 0;
}


static UWORD memx[64] = {
  0x86,0xa5,0xC6,0x55, 0x10,0xCE,0x10,0x00,  // 0xFFC0
  0xCC,0x12,0xAB,0x8E, 0x00,0x00,0x43,0x53,  // 0xFFC8
  0x20,0xfe,0x00,0x00, 0x00,0x00,0x00,0x00,  // 0xFFD0
  0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,  // 0xFFD8
  0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,  // 0xFFE0
  0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,  // 0xFFE8
  0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,  // 0xFFF0
  0x00,0x00,0x00,0x00, 0x00,0x00,0xff,0xc0   // 0xFFF8
};

UWORD MemsReadByte(UWORD addx,UWORD *perr)
{
  int offs = addx - 0xFFC0;
  return memx[offs];
}
void lua_pushstring(lua_State *L, const char *s)
{ printf("%s\n",s); }
int lua_error(lua_State *L)
{ //exit(0); 
  return 0; }
*/

/*****************************************************************
 * CPU_GetPCreg
 * Return with PCreg value
 ***************************************************************/
int CPU_GetPCreg_c(lua_State *L)
{
  lua_settop(L,0);
  lua_checkstack (L,1);
  lua_pushinteger(L,PCreg);
  return 1;
}


/*****************************************************************
 * CPU_Reset
 * Collection of required actions that happen folling reset
 ***************************************************************/
int CPU_Reset_c(lua_State *L)
{
  UWORD ByteHi, ByteLo, err;
  
  // Clear direct page
  DPreg = 0;
  
  // Disarm NMI
  NMI_ARMED_FLAG = 0x0;
  
  // Obtain reset vector
  ByteHi = MemsReadByte(INTVECT_RESET,&err);
  ByteLo = MemsReadByte(INTVECT_RESET+1,&err);
  if (ByteHi & BYTE_FLAGS_MASK || ByteLo & BYTE_FLAGS_MASK){
    lua_pushstring(L, "CPU: Reset vector not valid\n");
    lua_error(L);
  }
  
  // Assign reset vector to PC
  PCreg = 0xFFFF & ((ByteHi << 8) + (0xFF & ByteLo));
  Areg  = BYTE_NOT_INIT;
  Breg  = BYTE_NOT_INIT;
  DPreg = 0x00<<8;
  Sreg  = WORD_NOT_INIT;
  Ureg  = WORD_NOT_INIT;
  Xreg  = WORD_NOT_INIT;
  Yreg  = WORD_NOT_INIT;
  CC7Eflag = BYTE_NOT_INIT;
  CC6Fflag = CCREG_F_FLAG;
  CC5Hflag = BYTE_NOT_INIT;
  CC4Iflag = CCREG_I_FLAG;
  CC3Nflag = BYTE_NOT_INIT;
  CC2Zflag = BYTE_NOT_INIT;
  CC1Vflag = BYTE_NOT_INIT;
  CC0Cflag = BYTE_NOT_INIT;
  
  // Return value in PC
  //lua_pushinteger(L,PCreg);
  lua_settop(L,0);
  lua_checkstack (L,0);
  return 0;
}

/****************************************************************
 * CPU_DisOpcode
 * Disassemble opcodes
 ***************************************************************/
int CPU_DisOpCode_c(lua_State *L)
{
  static char buff[64];
  UWORD op, nd, tx, addx, err;

  // Obtain the address
  if (!lua_isnumber(L, 1)){
      lua_pushstring(L, "CPU_DisOpcode expects address");
      lua_error(L);
  }
  addx  = 0xFFFF & lua_tointeger(L,1);
  
  op = DxByte();
  
  
  // Consider the page switch
  switch(op){ 
  case 0x10: op = (op << 8) + DxByte(); break;
  case 0x11: op = (op << 8) + DxByte(); break;
  }

  // Interpret the instruction
  switch(op){
    // ABX
  case 0x3A:   sprintf(buff,"abx"); break;
    // ADC-IMM
  case 0x89:   nd = DxByte(); sprintf(buff,"adca  #$%.2X",nd); break;
  case 0xC9:   nd = DxByte(); sprintf(buff,"adcb  #$%.2X",nd); break;
    // ADC-DIR
  case 0x99:   nd = DxByte(); sprintf(buff,"adca   $%.2X",nd); break;
  case 0xD9:   nd = DxByte(); sprintf(buff,"addb   $%.2X",nd); break;
    // ADC-IDX
  case 0xA9:   sprintf(buff,"adca   %s",INDY(L,&addx)); break;
  case 0xE9:   sprintf(buff,"adcb   %s",INDY(L,&addx)); break;
    // ADC-EXT
  case 0xB9:   nd = DxWord(); sprintf(buff,"adca   $%.4X",nd); break;
  case 0xF9:   nd = DxWord(); sprintf(buff,"addb   $%.4X",nd); break;
    // ADD-IMM
  case 0x8B:   nd = DxByte(); sprintf(buff,"adda  #$%.2X",nd); break;
  case 0xCB:   nd = DxByte(); sprintf(buff,"addb  #$%.2X",nd); break;
  case 0xC3:   nd = DxWord(); sprintf(buff,"addd  #$%.4X",nd); break;
    // ADD-DIR
  case 0x9B:   nd = DxByte(); sprintf(buff,"adda   $%.2X",nd); break;
  case 0xDB:   nd = DxByte(); sprintf(buff,"addb   $%.2X",nd); break;  
  case 0xD3:   nd = DxByte(); sprintf(buff,"addd   $%.2X",nd); break;
    // ADD-IDX
  case 0xAB:   sprintf(buff,"adda   %s",INDY(L,&addx)); break;
  case 0xEB:   sprintf(buff,"addb   %s",INDY(L,&addx)); break;
  case 0xE3:   sprintf(buff,"addd   %s",INDY(L,&addx)); break;
    // ADD-EXT
  case 0xBB:   nd = DxWord(); sprintf(buff,"adda   $%.4X",nd); break;
  case 0xFB:   nd = DxWord(); sprintf(buff,"addb   $%.4X",nd); break;  
  case 0xF3:   nd = DxWord(); sprintf(buff,"addd   $%.4X",nd); break;
    // AND-IMM
  case 0x84:   nd = DxByte(); sprintf(buff,"anda  #$%.2X",nd); break;
  case 0xC4:   nd = DxByte(); sprintf(buff,"andb  #$%.2X",nd); break;
  case 0x1C:   nd = DxByte(); sprintf(buff,"andcc  #$%.2X",nd); break;
    // AND-DIR
  case 0x94:   nd = DxByte(); sprintf(buff,"anda    $%.2X",nd); break;
  case 0xD4:   nd = DxByte(); sprintf(buff,"anda    $%.2X",nd); break;
    // AND-IDX
  case 0xA4:   sprintf(buff,"anda   %s",INDY(L,&addx)); break;
  case 0xE4:   sprintf(buff,"addb   %s",INDY(L,&addx)); break;
    // AND-EXT
  case 0xB4:   nd = DxWord(); sprintf(buff,"anda    $%.4X",nd); break;
  case 0xF4:   nd = DxWord(); sprintf(buff,"anda    $%.4X",nd); break;
    // ASL/LSL-DIR
  case 0x08:   nd = DxByte(); sprintf(buff,"asl/lsl $%.2X",nd); break;
    // ASL/LSL-IDX
  case 0x68:   sprintf(buff,"asl/lsl %s",INDY(L,&addx)); break;
    // ASL/LSL-EXT
  case 0x78:   nd = DxWord(); sprintf(buff,"asl/lsl $%.4X",nd); break;
    // ASL/LSL-INH
  case 0x48:   sprintf(buff,"asla/lsla"); break;
  case 0x58:   sprintf(buff,"aslb/lslb"); break;
    // ASR-DIR
  case 0x07:   nd = DxWord(); sprintf(buff,"asr     $%.4X",nd); break;
    // ASR-IDX
  case 0x67:   sprintf(buff,"asr     %s",INDY(L,&addx)); break;
    // ASR-EXT
  case 0x77:   nd = DxWord(); sprintf(buff,"asr     $%.4X",nd); break;
    // ASR-INH
  case 0x47:   sprintf(buff,"asra"); break;
  case 0x57:   sprintf(buff,"asrb"); break;
    // BIT-IMM
  case 0x85:   nd = DxByte(); sprintf(buff,"bita  #$%.2X",nd); break;
  case 0xC5:   nd = DxByte(); sprintf(buff,"bitb  #$%.2X",nd); break;
    // BIT-DIR
  case 0x95:   nd = DxByte(); sprintf(buff,"bita   $%.2X",nd); break;
  case 0xD5:   nd = DxByte(); sprintf(buff,"bitb   $%.2X",nd); break;
    // BIT-IDX
  case 0xA5:   sprintf(buff,"bita    %s",INDY(L,&addx)); break;
  case 0xE5:   sprintf(buff,"bitb    %s",INDY(L,&addx)); break;
    // BIT-EXT
  case 0xB5:   nd = DxWord(); sprintf(buff,"bita   $%.4X",nd); break;
  case 0xF5:   nd = DxWord(); sprintf(buff,"bitb   $%.4X",nd); break;
    // CLR-DIR
  case 0x0F:   nd = DxByte(); sprintf(buff,"clr    $%.2X",nd); break;
    // CLR-IDX
  case 0x6F:   sprintf(buff,"clr     %s",INDY(L,&addx)); break;
    // CLR-EXT
  case 0x7F:   nd = DxWord(); sprintf(buff,"clr    $%.4X",nd); break;
    // CLR-INH
  case 0x4F:   sprintf(buff,"clra"); break;
  case 0x5F:   sprintf(buff,"clrb"); break;
    // CMP-IMM
  case 0x81:   nd = DxByte(); sprintf(buff,"cmpa     #$%.2X",nd); break;
  case 0xC1:   nd = DxByte(); sprintf(buff,"cmpb     #$%.2X",nd); break; 
  case 0x1083: nd = DxWord(); sprintf(buff,"cmpd     #$%.4X",nd); break;
  case 0x118C: nd = DxWord(); sprintf(buff,"cmps     #$%.4X",nd); break;
  case 0x1183: nd = DxWord(); sprintf(buff,"cmpu     #$%.4X",nd); break;
  case 0x8C:   nd = DxWord(); sprintf(buff,"cmpx     #$%.4X",nd); break;
  case 0x108C: nd = DxWord(); sprintf(buff,"cmpy     #$%.4X",nd); break;
    // CMP-DIR
  case 0x91:   nd = DxByte(); sprintf(buff,"cmpa      $%.2X",nd); break;
  case 0xD1:   nd = DxByte(); sprintf(buff,"cmpb      $%.2X",nd); break;
  case 0x1093: nd = DxByte(); sprintf(buff,"cmpd      $%.2X",nd); break;
  case 0x119C: nd = DxByte(); sprintf(buff,"cmps      $%.2X",nd); break;
  case 0x1193: nd = DxByte(); sprintf(buff,"cmpu      $%.2X",nd); break;
  case 0x9C:   nd = DxByte(); sprintf(buff,"cmpx      $%.2X",nd); break;
  case 0x109C: nd = DxByte(); sprintf(buff,"cmpy      $%.2X",nd); break;
    // CMP-IDX
  case 0xA1:   sprintf(buff,"cmpa      %s",INDY(L,&addx)); break;
  case 0xE1:   sprintf(buff,"cmpb      %s",INDY(L,&addx)); break;
  case 0x10A3: sprintf(buff,"cmpd      %s",INDY(L,&addx)); break; 
  case 0x11AC: sprintf(buff,"cmps      %s",INDY(L,&addx)); break;
  case 0x11A3: sprintf(buff,"cmpu      %s",INDY(L,&addx)); break;  
  case 0xAC:   sprintf(buff,"cmpx      %s",INDY(L,&addx)); break;  
  case 0x10AC: sprintf(buff,"cmpy      %s",INDY(L,&addx)); break;
    // CMP-EXT
  case 0xB1:   nd = DxWord(); sprintf(buff,"cmpa      $%.4X",nd); break;
  case 0xF1:   nd = DxWord(); sprintf(buff,"cmpb      $%.4X",nd); break;
  case 0x10B3: nd = DxWord(); sprintf(buff,"cmpd      $%.4X",nd); break;
  case 0x11BC: nd = DxWord(); sprintf(buff,"cmps      $%.4X",nd); break;
  case 0x11B3: nd = DxWord(); sprintf(buff,"cmpu      $%.4X",nd); break;
  case 0xBC:   nd = DxWord(); sprintf(buff,"cmpx      $%.4X",nd); break;
  case 0x10BC: nd = DxWord(); sprintf(buff,"cmpy     $%.4X",nd); break;
    // COM-DIR
  case 0x03:   nd = DxByte(); sprintf(buff,"com    $%.2X",nd); break;
    // COM-IDX
  case 0x63:   sprintf(buff,"com     %s",INDY(L,&addx)); break;
    // COM-EXT
  case 0x73:   nd = DxWord(); sprintf(buff,"com    $%.4X",nd); break;
    // COM-INH
  case 0x43:   sprintf(buff,"coma"); break;
  case 0x53:   sprintf(buff,"comb"); break;
    // CWAI-IMM
  case 0x3C:   nd   = FxByte(); sprintf(buff,"cwai #$%.2X",nd); break; 
    // DAA-INH
  case 0x19:   sprintf(buff,"daa"); break;
    // DEC-DIR
  case 0x0A:   nd = DxByte(); sprintf(buff,"dec    $%.2X",nd); break;
    // DEC-IDX
  case 0x6A:   sprintf(buff,"dec     %s",INDY(L,&addx)); break;
    // DEC-EXT
  case 0x7A:   nd = DxWord(); sprintf(buff,"dec    $%.4X",nd); break;
    // DEC-INH
  case 0x4A:   sprintf(buff,"deca"); break;
  case 0x5A:   sprintf(buff,"decb"); break;
    // EOR-IMM
  case 0x88:   nd = DxByte(); sprintf(buff,"eora  #$%.2X",nd); break;
  case 0xC8:   nd = DxByte(); sprintf(buff,"eorb  #$%.2X",nd); break;
    // EOR-DIR
  case 0x98:   nd = DxByte(); sprintf(buff,"eora   $%.2X",nd); break;
  case 0xD8:   nd = DxByte(); sprintf(buff,"eorb   $%.2X",nd); break;
    // EOR-IDX
  case 0xA8:   sprintf(buff,"eora     %s",INDY(L,&addx)); break;
  case 0xE8:   sprintf(buff,"eorb     %s",INDY(L,&addx)); break;
    // EOR-EXT
  case 0xB8:   nd = DxWord(); sprintf(buff,"eora   $%.4X",nd); break;
  case 0xF8:   nd = DxWord(); sprintf(buff,"eorb   $%.4X",nd); break;
    // EXG
  case 0x1E:   nd = DxByte(); sprintf(buff,"exg %s,%s",
               PickTEchar(nd>>4),PickTEchar(nd&0xF));break;
    // INC-DIR
  case 0x0C:   nd = DxByte(); sprintf(buff,"inc    $%.2X",nd); break;
    // INC-IDX
  case 0x6C:   sprintf(buff,"inc   %s",INDY(L,&addx)); break;
    // INC-EXT
  case 0x7C:   nd = DxWord(); sprintf(buff,"inc   $%.4X",nd); break;
    // INC-INH
  case 0x4C:   sprintf(buff,"inca"); break;
  case 0x5C:   sprintf(buff,"incb"); break;
    // JMP-DIR
  case 0x0E:   nd = DxByte(); sprintf(buff,"jmp    $%.2X",nd); break;
    // JMP-IDX
  case 0x6E:   sprintf(buff,"jmp   %s",INDY(L,&addx)); break;
    // JMP-EXT
  case 0x7E:   nd = DxWord(); sprintf(buff,"jmp   $%.4X",nd); break;
    // JSR-DIR
  case 0x9D:   nd = DxByte(); sprintf(buff,"jsr    $%.2X",nd); break;
    // JSR-IDX
  case 0xAD:   sprintf(buff,"jsr   %s",INDY(L,&addx)); break;
    // JSR-EXT
  case 0xBD:   nd = DxWord(); sprintf(buff,"jsr   $%.4X",nd); break;
    // LD-IMM
  case 0x86:   nd = DxByte(); sprintf(buff,"lda    #$%.2X",nd); break;
  case 0xC6:   nd = DxByte(); sprintf(buff,"ldb    #$%.2X",nd); break;
  case 0xCC:   nd = DxWord(); sprintf(buff,"ldd    #$%.4X",nd); break;
  case 0x10CE: nd = DxWord(); sprintf(buff,"lds    #$%.4X",nd); break; 
  case 0xCE:   nd = DxWord(); sprintf(buff,"ldu    #$%.4X",nd); break; 
  case 0x8E:   nd = DxWord(); sprintf(buff,"ldx    #$%.4X",nd); break;
  case 0x108E: nd = DxWord(); sprintf(buff,"ldy    #$%.4X",nd); break;
      // LD-DIR
  case 0x96:   nd = DxWord(); sprintf(buff,"lda     $%.4X",nd); break;
  case 0xD6:   nd = DxWord(); sprintf(buff,"ldb     $%.4X",nd); break;
  case 0xDC:   nd = DxWord(); sprintf(buff,"ldd     $%.4X",nd); break;
  case 0x10DE: nd = DxWord(); sprintf(buff,"lds     $%.4X",nd); break; 
  case 0xDE:   nd = DxWord(); sprintf(buff,"ldu     $%.4X",nd); break; 
  case 0x9E:   nd = DxWord(); sprintf(buff,"ldx     $%.4X",nd); break;
  case 0x109E: nd = DxWord(); sprintf(buff,"ldy     $%.4X",nd); break;
    // LD-IDX
  case 0xA6:   sprintf(buff,"lda  %s",INDY(L,&addx)); break;
  case 0xE6:   sprintf(buff,"ldb  %s",INDY(L,&addx)); break;
  case 0xEC:   sprintf(buff,"ldd  %s",INDY(L,&addx)); break;
  case 0x10EE: sprintf(buff,"lds  %s",INDY(L,&addx)); break;  
  case 0xEE:   sprintf(buff,"ldu  %s",INDY(L,&addx)); break;
  case 0xAE:   sprintf(buff,"ldx  %s",INDY(L,&addx)); break;
  case 0x10AE: sprintf(buff,"ldy  %s",INDY(L,&addx)); break;
    // LD-EXT
  case 0xB6:   nd = DxWord(); sprintf(buff,"lda   $%.4X",nd); break;
  case 0xF6:   nd = DxWord(); sprintf(buff,"ldb   $%.4X",nd); break;
  case 0xFC:   nd = DxWord(); sprintf(buff,"ldd   $%.4X",nd); break;
  case 0x10FE: nd = DxWord(); sprintf(buff,"lds   $%.4X",nd); break; 
  case 0xFE:   nd = DxWord(); sprintf(buff,"ldu   $%.4X",nd); break; 
  case 0xBE:   nd = DxWord(); sprintf(buff,"ldx   $%.4X",nd); break;
  case 0x10BE: nd = DxWord(); sprintf(buff,"ldy   $%.4X",nd); break;
    // LEA-IDX
  case 0x32:   sprintf(buff,"leas  %s",INDY(L,&addx)); break;
  case 0x33:   sprintf(buff,"leau  %s",INDY(L,&addx)); break;
  case 0x30:   sprintf(buff,"leax  %s",INDY(L,&addx)); break;  
  case 0x31:   sprintf(buff,"leay  %s",INDY(L,&addx)); break;
    // LSR-DIR
  case 0x04:   nd = DxByte(); sprintf(buff,"lsr    $%.2X",nd); break;
    // LSR-IDX
  case 0x64:   sprintf(buff,"lsr   %s",INDY(L,&addx)); break;
    // LSR-EXT
  case 0x74:   nd = DxWord(); sprintf(buff,"lsr    $%.4X",nd); break;
    // LSR-INH
  case 0x44:   sprintf(buff,"lsra"); break;
  case 0x54:   sprintf(buff,"lsrb"); break;
    // MUL-INH
  case 0x3D:   nd = DxWord(); sprintf(buff,"mul"); break;
    // NEG-DIR
  case 0x00:   nd = DxByte(); sprintf(buff,"neg    $%.2X",nd); break;
    // NEG-IDX
  case 0x60:   sprintf(buff,"neg   %s",INDY(L,&addx)); break;
    // NEG-EXT
  case 0x70:   nd = DxWord(); sprintf(buff,"neg    $%.4X",nd); break;
    // NEG-INH
  case 0x40:   sprintf(buff,"nega"); break;
  case 0x50:   sprintf(buff,"negb"); break;    
    // NOP-INH
  case 0x12:   sprintf(buff,"nop"); break;
    // OR-IMM
  case 0x8A:   nd = DxByte(); sprintf(buff,"ora  #$%.2X",nd); break;
  case 0xCA:   nd = DxWord(); sprintf(buff,"orb  #$%.2X",nd); break;
  case 0x1A:   nd = DxWord(); sprintf(buff,"orcc #$%.2X",nd); break;
    // OR-DIR
  case 0x9A:   nd = DxByte(); sprintf(buff,"ora   $%.2X",nd); break;
  case 0xDA:   nd = DxByte(); sprintf(buff,"orb   $%.2X",nd); break;
    // OR-IDX
  case 0xAA:   sprintf(buff,"ora   %s",INDY(L,&addx)); break;
  case 0xEA:   sprintf(buff,"orb   %s",INDY(L,&addx)); break;
    // OR-EXT
  case 0xBA:   nd = DxWord(); sprintf(buff,"ora   $%.4X",nd); break;
  case 0xFA:   nd = DxWord(); sprintf(buff,"orb   $%.4X",nd); break;
    // PSH
  case 0x34:   nd = DxByte(); PshFlags(buff,nd,'s','u'); break;
  case 0x36:   nd = DxByte(); PshFlags(buff,nd,'u','s'); break;
    // PUL
  case 0x35:   nd = DxByte(); PulFlags(buff,nd,'s','u'); break;
  case 0x37:   nd = DxByte(); PulFlags(buff,nd,'s','u'); break;
    // ROL-DIR
  case 0x09:   nd = DxByte(); sprintf(buff,"rol    $%.2X",nd); break;
    // ROL-IDX
  case 0x69:   sprintf(buff,"rol   %s",INDY(L,&addx)); break;
    // ROL-EXT
  case 0x79:   nd = DxWord(); sprintf(buff,"rol    $%.4X",nd); break;
    // ROL-INH
  case 0x49:   sprintf(buff,"rola"); break;
  case 0x59:   sprintf(buff,"rolb"); break;
    // ROR-DIR
  case 0x06:   nd = DxByte(); sprintf(buff,"ror    $%.2X",nd); break;
    // ROR-IDX
  case 0x66:   sprintf(buff,"ror   %s",INDY(L,&addx)); break;
    // ROR-EXT
  case 0x76:   nd = DxWord(); sprintf(buff,"ror    $%.4X",nd); break;
    // ROR-INH
  case 0x46:   sprintf(buff,"rora"); break;
  case 0x56:   sprintf(buff,"rorb"); break;
    // RTI-INH
  case 0x3B:   sprintf(buff,"rti"); break;
    // RTS-INH
  case 0x39:   sprintf(buff,"rts"); break;
    // SBC-IMM
  case 0x82:   nd = DxByte(); sprintf(buff,"sbca  #$%.2X",nd); break; 
  case 0xC2:   nd = DxByte(); sprintf(buff,"sbcb  #$%.2X",nd); break;
    // SBC-DIR
  case 0x92:   nd = DxByte(); sprintf(buff,"sbca   $%.2X",nd); break;
  case 0xD2:   nd = DxByte(); sprintf(buff,"sbcb   $%.2X",nd); break;
    // SBC-IDX
  case 0xA2:   sprintf(buff,"sbca  %s",INDY(L,&addx)); break;
  case 0xE2:   sprintf(buff,"sbcb  %s",INDY(L,&addx)); break;
    // SBC-EXT
  case 0xB2:   nd = DxWord(); sprintf(buff,"sbca   $%.4X",nd); break;
  case 0xF2:   nd = DxWord(); sprintf(buff,"sbcb   $%.4X",nd); break;
    // SEX-INH
  case 0x1D:   sprintf(buff,"sex"); break;
    // ST-DIR
  case 0x97:   nd = DxByte(); sprintf(buff,"sta  $%.2X",nd); break;
  case 0xD7:   nd = DxByte(); sprintf(buff,"stb  $%.2X",nd); break;
  case 0xDD:   nd = DxByte(); sprintf(buff,"std  $%.2X",nd); break;  
  case 0x10DF: nd = DxByte(); sprintf(buff,"sts  $%.2X",nd); break;
  case 0xDF:   nd = DxByte(); sprintf(buff,"stu  $%.2X",nd); break;
  case 0x9F:   nd = DxByte(); sprintf(buff,"stx  $%.2X",nd); break;
  case 0x109F: nd = DxByte(); sprintf(buff,"sty  $%.2X",nd); break;
    // ST-IDX  
  case 0xA7:   sprintf(buff,"sta  %s",INDY(L,&addx)); break;  
  case 0xE7:   sprintf(buff,"stb  %s",INDY(L,&addx)); break;
  case 0xED:   sprintf(buff,"std  %s",INDY(L,&addx)); break;
  case 0x10EF: sprintf(buff,"sts  %s",INDY(L,&addx)); break;
  case 0xEF:   sprintf(buff,"stu  %s",INDY(L,&addx)); break;
  case 0xAF:   sprintf(buff,"stx  %s",INDY(L,&addx)); break;
  case 0x10AF: sprintf(buff,"sty  %s",INDY(L,&addx)); break;
    // ST-EXT
  case 0xB7:   nd = DxWord(); sprintf(buff,"sta  $%.4X",nd); break;
  case 0xF7:   nd = DxWord(); sprintf(buff,"stb  $%.4X",nd); break;
  case 0xFD:   nd = DxWord(); sprintf(buff,"std  $%.4X",nd); break;  
  case 0x10FF: nd = DxWord(); sprintf(buff,"sts  $%.4X",nd); break;
  case 0xFF:   nd = DxWord(); sprintf(buff,"stu  $%.4X",nd); break;
  case 0xBF:   nd = DxWord(); sprintf(buff,"stx  $%.4X",nd); break;
  case 0x10BF: nd = DxWord(); sprintf(buff,"sty  $%.4X",nd); break;
    // SUB-IMM
  case 0x80:   nd = DxByte(); sprintf(buff,"suba  #$%.2X",nd); break;
  case 0xC0:   nd = DxByte(); sprintf(buff,"subb  #$%.2X",nd); break;
  case 0x83:   nd = DxWord(); sprintf(buff,"subd  #$%.4X",nd); break;
    // SUB-DIR
  case 0x90:   nd = DxByte(); sprintf(buff,"suba  $%.2X",nd); break;
  case 0xD0:   nd = DxByte(); sprintf(buff,"subb  $%.2X",nd); break;
  case 0x93:   nd = DxByte(); sprintf(buff,"subd  $%.2X",nd); break;
    // SUB-IDX
  case 0xA0:   sprintf(buff,"suba  %s",INDY(L,&addx)); break;
  case 0xE0:   sprintf(buff,"suba  %s",INDY(L,&addx)); break;
  case 0xA3:   sprintf(buff,"suba  %s",INDY(L,&addx)); break;
    // SUB-EXT
  case 0xB0:   nd = DxWord(); sprintf(buff,"suba  $%.4X",nd); break;
  case 0xF0:   nd = DxWord(); sprintf(buff,"subb  $%.4X",nd); break;
  case 0xB3:   nd = DxWord(); sprintf(buff,"subd  $%.4X",nd); break;
    // SWI-INH
  case 0x3F:   sprintf(buff,"swi"); break;
  case 0x103F: sprintf(buff,"swi2"); break;
  case 0x113F: sprintf(buff,"swi3"); break;
    // SYNC-INH
  case 0x13:   sprintf(buff,"sync"); break;
    // TFR
  case 0x1F:   nd = DxByte(); sprintf(buff,"tfr   %s,%s",
                PickTEchar(nd>>4),PickTEchar(nd & 0x0F));break;
    // TST-DIR
  case 0x0D:   nd = DxByte(); sprintf(buff,"tst   $%.2X",nd); break;
    // TST-IDX
  case 0x6D:   sprintf(buff,"tst   %s",INDY(L,&addx)); break;
    // TST-EXT
  case 0x7D:   nd = DxWord(); sprintf(buff,"tst   $%.4X",nd); break;
    // TST-INH
  case 0x4D:   sprintf(buff,"tsta"); break;
  case 0x5D:   sprintf(buff,"tstb"); break;
    // Branches
  case 0x24: nd = DxByte(); tx = BRT(nd); sprintf(buff,"bcc/bhs $%.4X",tx);break;
  case 0x25: nd = DxByte(); tx = BRT(nd); sprintf(buff,"bcs/blo $%.4X",tx);break;
  case 0x27: nd = DxByte(); tx = BRT(nd); sprintf(buff,"beq $%.4X",tx);break;
  case 0x2C: nd = DxByte(); tx = BRT(nd); sprintf(buff,"bge $%.4X",tx);break;
  case 0x2E: nd = DxByte(); tx = BRT(nd); sprintf(buff,"bgt $%.4X",tx);break;
  case 0x22: nd = DxByte(); tx = BRT(nd); sprintf(buff,"bhi $%.4X",tx);break;
  case 0x2F: nd = DxByte(); tx = BRT(nd); sprintf(buff,"ble $%.4X",tx);break;
  case 0x23: nd = DxByte(); tx = BRT(nd); sprintf(buff,"bls $%.4X",tx);break;
  case 0x2D: nd = DxByte(); tx = BRT(nd); sprintf(buff,"blt $%.4X",tx);break;
  case 0x2B: nd = DxByte(); tx = BRT(nd); sprintf(buff,"bmi $%.4X",tx);break;  
  case 0x26: nd = DxByte(); tx = BRT(nd); sprintf(buff,"bne $%.4X",tx);break;
  case 0x2A: nd = DxByte(); tx = BRT(nd); sprintf(buff,"bpl $%.4X",tx);break;  
  case 0x20: nd = DxByte(); tx = BRT(nd); sprintf(buff,"bra $%.4X",tx);break;
  case 0x21: nd = DxByte(); tx = BRT(nd); sprintf(buff,"brn $%.4X",tx);break;
  case 0x28: nd = DxByte(); tx = BRT(nd); sprintf(buff,"bvc $%.4X",tx);break;    
  case 0x29: nd = DxByte(); tx = BRT(nd); sprintf(buff,"bvs $%.4X",tx);break;

  default:  sprintf(buff,"Unknown 0x%.2X",op); break;
  }
  printf("%s\n",buff);
  lua_settop(L,0);
  lua_checkstack (L,1);
  lua_pushinteger(L,addx);
  return 1;
  
DxErr:
  switch(err){
  case BYTE_NOT_INIT:  sprintf(buff,"MemNotInit"); break;
  case BYTE_NO_VALUE:  sprintf(buff,"MemNoValue"); break;
  case BYTE_UNKNOWN:   sprintf(buff,"MemUnknown"); break; 
  case BYTE_NO_REGION: sprintf(buff,"MemNoRegion"); break;
  }
  printf("%s\n",buff);
  lua_checkstack (L,1);
  lua_pushinteger(L,0x10000);
  return 1;
}

/*
 * Gather bytes and produce string for indexed addressing mode
 */
char *INDY(lua_State *L, UWORD *paddx)
{
  static char buff[16];
  long  fs;
  UWORD addx = *paddx;
  UWORD ff,err,nd = DxByte();

  
  if (!(nd & 0x80)){                        // 5-bit offset
    fs = nd & 0x0F; 
    if (nd & 0x10) fs = fs - 16;
    sprintf(buff,"%d,%s",(int)fs,IndPickRx(nd));
  }
  else{ switch(nd & 0x0F){
    case 0x4: if (nd & 10) sprintf(buff,"[%s]",IndPickRx(nd));             // no offset
               else        sprintf(buff,"%s",IndPickRx(nd)); break;
    case 0x8: fs = DxByte(); if (fs & 0x80) fs = fs - 128;
               if (nd & 10) sprintf(buff,"[%d,%s]",(int)fs,IndPickRx(nd)); // 8-bit offset
               else        sprintf(buff,"%d,%s",(int)fs,IndPickRx(nd)); break;
    case 0x9: fs = DxWord(); if (fs & 0x8000) fs = fs - 32768;
               if (nd & 10) sprintf(buff,"[%d,%s]",(int)fs,IndPickRx(nd)); // 16-bit offset
               else        sprintf(buff,"%d,%s",(int)fs,IndPickRx(nd)); break;
    case 0x6: if (nd & 10) sprintf(buff,"[A,%s]",IndPickRx(nd));           // A offset
               else        sprintf(buff,"A,%s",IndPickRx(nd)); break;
    case 0x5: if (nd & 10) sprintf(buff,"[B,%s]",IndPickRx(nd));           // B offset
               else        sprintf(buff,"B,%s",IndPickRx(nd)); break;
    case 0xB: if (nd & 10) sprintf(buff,"[D,%s]",IndPickRx(nd));           // D offset
               else        sprintf(buff,"D,%s",IndPickRx(nd)); break;
    case 0x0: if (nd & 10) sprintf(buff,"[%s+]",IndPickRx(nd));            // Inc. 1
               else        sprintf(buff,"%s+",IndPickRx(nd)); break;
    case 0x1: if (nd & 10) sprintf(buff,"[%s++]",IndPickRx(nd));           // Inc. 2
               else        sprintf(buff,"%s++",IndPickRx(nd)); break;
    case 0x2: if (nd & 10) sprintf(buff,"[-%s]",IndPickRx(nd));            // Dec. 1
               else        sprintf(buff,"-%s",IndPickRx(nd)); break;
    case 0x3: if (nd & 10) sprintf(buff,"[--%s]",IndPickRx(nd));           // Dec. 2
               else        sprintf(buff,"--%s",IndPickRx(nd)); break;
    case 0xC: fs = DxByte(); if (fs & 0x80) fs = fs - 128;
               if (nd & 10) sprintf(buff,"[%d,PC]",(int)fs);               // 8-bit PC offset
               else        sprintf(buff,"%d,PC",(int)fs); break;
    case 0xD: fs = DxWord(); if (fs & 0x8000) fs = fs - 32768;
               if (nd & 10) sprintf(buff,"[%d,PC]",(int)fs);               // 16-bit PC offset
               else        sprintf(buff,"%d,PC",(int)fs); break;
    case 0xF: ff = DxWord(); sprintf(buff,"[0x%.4X]",ff); break;          // EXT-IND
    }
  }
  *paddx = addx;
  return buff;
  
  DxErr:
    return NULL;
}

/*
 * Pick a string corresponding to the register used in index addressing mode
 */
char *IndPickRx(UWORD nd)
{
  char *px;
  switch(nd&0x60){
  case 0x00: px = "X"; break;
  case 0x20: px = "Y"; break;
  case 0x40: px = "U"; break;
  default:   px = "S";  break;
  }
  return px;
}

char *PshFlags(char *bx, UWORD flags, char MNchar, char SUchar)
{
  *(bx++) = 'p'; *(bx++) = 's'; *(bx++) = 'h'; *(bx++) = MNchar;
  *(bx++) = ' '; *(bx++) = ' '; 
  
  char *pb = bx;
  if (flags & PP_PC_BIT){ *(pb++)='p'; *(pb++)='c';}
  if (flags & PP_SU_BIT){ if (pb != bx){*(pb++)=',';} *(pb++)=SUchar;}  
  if (flags & PP_Y_BIT) { if (pb != bx){*(pb++)=',';} *(pb++)='Y';}  
  if (flags & PP_X_BIT) { if (pb != bx){*(pb++)=',';} *(pb++)='X';}
  if (flags & PP_DP_BIT){ if (pb != bx){*(pb++)=',';} *(pb++)='D'; *(pb++)='P';}   
  if (flags & PP_B_BIT) { if (pb != bx){*(pb++)=',';} *(pb++)='B';}
  if (flags & PP_A_BIT) { if (pb != bx){*(pb++)=',';} *(pb++)='A';}
  if (flags & PP_CC_BIT){ if (pb != bx){*(pb++)=',';} *(pb++)='C'; *(pb++)='C';}
  *pb = '\0';
  return pb;
}

char *PulFlags(char *bx, UWORD flags, char MNchar, char SUchar)
{
  *(bx++) = 'p'; *(bx++) = 'u'; *(bx++) = 'l'; *(bx++) = MNchar;
  *(bx++) = ' '; *(bx++) = ' '; 
  
  char *pb = bx;
  if (flags & PP_CC_BIT){ *(pb++)='C'; *(pb++)='C';}
  if (flags & PP_A_BIT) { if (pb != bx){*(pb++)=',';} *(pb++)='A';}  
  if (flags & PP_B_BIT) { if (pb != bx){*(pb++)=',';} *(pb++)='B';}  
  if (flags & PP_DP_BIT){ if (pb != bx){*(pb++)=',';} *(pb++)='D'; *(pb++)='P';}
  if (flags & PP_X_BIT) { if (pb != bx){*(pb++)=',';} *(pb++)='X'; }   
  if (flags & PP_Y_BIT) { if (pb != bx){*(pb++)=',';} *(pb++)='Y';}
  if (flags & PP_SU_BIT){ if (pb != bx){*(pb++)=',';} *(pb++)=SUchar;}
  if (flags & PP_PC_BIT){ if (pb != bx){*(pb++)=',';} *(pb++)='P'; *(pb++)='C';}
  *pb = '\0';
  return pb;
}

char *PickTEchar(UWORD rx)
{
  char *px;
  switch(rx){
  case 0x0: px = "D";  break;
  case 0x1: px = "X";  break;
  case 0x2: px = "Y";  break;
  case 0x3: px = "U";  break;
  case 0x4: px = "S";  break;
  case 0x5: px = "PC"; break;
  case 0x8: px = "A";  break;
  case 0x9: px = "B";  break;
  case 0xA: px = "CC"; break;
  case 0xB: px = "DP"; break;
  }
  return px;
}

/***********************************************************************
 * CPU_FetchEx_c
 * Perform the Fetch-Execute Cycle
 * What should happen if attempt to execute a conditional branch
 * with an undefined flag?
 * The lsl and asl instructions have the same opcodes but the 
 * flag effects are listed differently, asl and asr undefine H
 **********************************************************************/
int CPU_FetchEx_c(lua_State *L)
{

  UWORD ea,ev,bval,nd,dx;

  UWORD op = FxByte();
  PCtmp = PCreg;
  // Consider the page switch
  switch(op){
  case 0x10: op = (op << 8) + FxByte(); break;
  case 0x11: op = (op << 8) + FxByte(); break;
  }

  // Execute the instruction
  switch(op){
    // ABX
  case 0x3A: Xreg = Breg + Xreg; break; // abx-inh
    // ADC-IMM
  case 0x89:   nd   = FxByte(); Areg = AddBytesCx(Areg,nd,CC0Cflag); break; // adca-imm
  case 0xC9:   nd   = FxByte(); Breg = AddBytesCx(Breg,nd,CC0Cflag); break; // adcb-imm
    // ADC-DIR
  case 0x99:   ea=DPreg+FxByte();Areg=AddBytesCx(Areg,RdByte(ea),CC0Cflag); break; // adca-dir
  case 0xD9:   ea=DPreg+FxByte();Breg=AddBytesCx(Breg,RdByte(ea),CC0Cflag); break; // adcb-dir
    // ADC-IDX
  case 0xA9:   ea=INDX(L); ev=RdByte(ea); Areg=AddBytesCx(Areg,ev,CC0Cflag); break; // adca-idx
  case 0xE9:   ea=INDX(L); ev=RdByte(ea); Breg=AddBytesCx(Breg,ev,CC0Cflag); break; // adcb-idx
    // ADC-EXT
  case 0xB9:   ea = FxWord(); Areg = AddBytesCx(Areg,RdByte(ea),CC0Cflag); break; // adca-ext
  case 0xF9:   ea = FxWord(); Breg = AddBytesCx(Breg,RdByte(ea),CC0Cflag); break; // adcb-ext
    // ADD-IMM
  case 0x8B:   nd   = FxByte(); Areg = AddBytesCx(Areg,nd,0x00); break; // adda-imm
  case 0xCB:   nd   = FxByte(); Breg = AddBytesCx(Breg,nd,0x00); break; // addb-imm
  case 0xC3:   nd   = FxWord(); PutDreg(AddWords(GetDreg(),nd));  break; // addd-imm
    // ADD-DIR
  case 0x9B:   ea=DPreg+FxByte();Areg = AddBytesCx(Areg,RdByte(ea),0x00); break; // adda-dir
  case 0xDB:   ea=DPreg+FxByte();Breg = AddBytesCx(Breg,RdByte(ea),0x00); break; // addb-dir
  case 0xD3:   ea=DPreg+FxByte();PutDreg( AddWords(GetDreg(),RdWord(ea)));break; // addd-dir
    // ADD-IDX
  case 0xAB:   ea=INDX(L); ev=RdByte(ea); Areg=AddBytesCx(Areg,ev,0x0); break; // adda-idx
  case 0xEB:   ea=INDX(L); ev=RdByte(ea); Breg=AddBytesCx(Breg,ev,0x0); break; // addb-idx
  case 0xE3:   ea=INDX(L); ev=RdWord(ea); PutDreg(AddWords(GetDreg(),ev)); break; // addd-idx
    // ADD-EXT
  case 0xBB:   ea = FxWord(); Areg = AddBytesCx(Areg,RdByte(ea),0x00); break; // adda-ext
  case 0xFB:   ea = FxWord(); Breg = AddBytesCx(Breg,RdByte(ea),0x00); break; // addb-ext
  case 0xF3:   ea = FxWord(); PutDreg( AddWords(GetDreg(),RdWord(ea))); break;    // addd-ext
    // AND-IMM
  case 0x84:   nd   = FxByte(); Areg &= nd; FlagBNZ0(Areg); break;       // anda-imm
  case 0xC4:   nd   = FxByte(); Breg &= nd; FlagBNZ0(Breg); break;       // andb-imm
  case 0x1C:   nd   = FxByte(); PutCreg( GetCreg() & nd );  break;       // andcc-imm
    // AND-DIR
  case 0x94:   ea=DPreg+FxByte(); Areg &= RdByte(ea); FlagBNZ0(Areg); break; // anda-dir
  case 0xD4:   ea=DPreg+FxByte(); Breg &= RdByte(ea); FlagBNZ0(Breg); break; // andb-dir
    // AND-IDX
  case 0xA4:   ea=INDX(L); ev=RdByte(ea); Areg &= ev; FlagBNZ0(Areg); break; // anda-idx
  case 0xE4:   ea=INDX(L); ev=RdByte(ea); Breg &= ev; FlagBNZ0(Breg); break; // andb-idx
    // AND-EXT
  case 0xB4:   ea = FxWord(); Areg &= RdByte(ea); FlagBNZ0(Areg); break; // anda-ext
  case 0xF4:   ea = FxWord(); Breg &= RdByte(ea); FlagBNZ0(Breg); break; // andb-ext
    // ASL-DIR & LSL-DIR
  case 0x08:   ea=DPreg+FxByte(); ev = RdByte(ea); bval = ASLfnct(ev);        // asl-dir
               StByte(ea,bval); break;
    // ASL-IDX & LSL-IDX
  case 0x68:   ea=INDX(L); ev=RdByte(ea); bval = ASLfnct(ev);             // asl-idx  
               StByte(ea,bval); break;       
    // ASL-EXT & LSL-EXT
  case 0x78:   ea = FxWord(); ev = RdByte(ea); bval = ASLfnct(ev);        // asl-ext
               StByte(ea,bval); break;
    // ASL-INH & LSL-INH
  case 0x48:   Areg = ASLfnct(Areg); break; // asla-inh
  case 0x58:   Breg = ASLfnct(Breg); break; // aslb-inh
    // ASR-DIR
  case 0x07:   ea=DPreg+FxByte(); ev = RdByte(ea); bval = ASRfnct(ev);     // asr-dir
               StByte(ea,bval); break; 
    // ASR-IDX
  case 0x67:   ea=INDX(L); ev=RdByte(ea); bval = ASRfnct(ev);              // asr-idx
               StByte(ea,bval); break; 
    // ASR-EXT
  case 0x77:   ea = FxWord(); ev = RdByte(ea); bval = ASRfnct(ev);         // asr-ext
               StByte(ea,bval); break; 
    // ASR-INH
  case 0x47:   Areg = ASRfnct(Areg); break;                               // asra-inh
  case 0x57:   Breg = ASLfnct(Breg); break;                               // asrb-inh
    // BIT-IMM
  case 0x85:   nd   = FxByte(); nd &= Areg; FlagBNZ0(nd);   break;        // bita-imm
  case 0xC5:   nd   = FxByte(); nd &= Breg; FlagBNZ0(nd);   break;        // bitb-imm
    // BIT-DIR
  case 0x95:   ea=DPreg+FxByte(); ev=RdByte(ea); bval=Areg&RdByte(ea);   // bita-dir
               FlagBNZ0(bval); break;
  case 0xD5:   ea=DPreg+FxByte(); ev=RdByte(ea); bval=Breg&RdByte(ea);   // bitb-dir
               FlagBNZ0(bval); break;
    // BIT-IDX
  case 0xA5:   ea=INDX(L);ev=RdByte(ea); ev &= Areg; FlagBNZ0(ev); break; // bita-idx
  case 0xE5:   ea=INDX(L);ev=RdByte(ea); ev &= Breg; FlagBNZ0(ev); break; // bitb-idx
    // BIT-EXT
  case 0xB5:   ea = FxWord(); ev = RdByte(ea); bval = Areg & RdByte(ea);   // bita-ext
               FlagBNZ0(bval); break;
  case 0xF5:   ea = FxWord(); ev = RdByte(ea); bval = Breg & RdByte(ea);   // bitb-ext
               FlagBNZ0(bval); break;
    // CLR-DIR
  case 0x0F:   ea=DPreg+FxByte();StByte(ea,0x00);FlagClr(); break;        // clra-ext
    // CLR-IDX
  case 0x6F:   ea=INDX(L);StByte(ea,0x00);FlagClr(); break;               // clra-idx
    // CLR-EXT
  case 0x7F:   ea=FxWord();StByte(ea,0x00);FlagClr(); break;              // clra-ext
    // CLR-INH
  case 0x4F:   Areg = 0x00; FlagClr(); break; // clra-inh
  case 0x5F:   Breg = 0x00; FlagClr(); break; // clrb-inh
    // CMP-DIR
  case 0x91:   ea=DPreg+FxByte(); ev=RdByte(ea);SubBytesCx(Areg,ev,0x00); break; // cmpa-dir
  case 0xD1:   ea=DPreg+FxByte(); ev=RdByte(ea);SubBytesCx(Breg,ev,0x00); break; // cmpb-dir
  case 0x1093: ea=DPreg+FxByte(); ev=RdWord(ea);SubWords(GetDreg(),ev); break;   // cmpd-dir
  case 0x119C: ea=DPreg+FxByte(); ev=RdWord(ea);SubWords(Sreg,ev); break;        // cmps-dir
  case 0x1193: ea=DPreg+FxByte(); ev=RdWord(ea);SubWords(Ureg,ev); break;        // cmpu-dir
  case 0x9C:   ea=DPreg+FxByte(); ev=RdWord(ea);SubWords(Xreg,ev); break;        // cmpx-dir
  case 0x109C: ea=DPreg+FxByte(); ev=RdWord(ea);SubWords(Yreg,ev); break;        // cmpy-dir
    // CMP-IMM
  case 0x81:   nd   = FxByte(); SubBytesCx(Areg,nd,0x00); break; // cmpa-imm
  case 0xC1:   nd   = FxByte(); SubBytesCx(Breg,nd,0x00); break; // cmpb-imm
  case 0x1083: nd   = FxWord(); SubWords(GetDreg(),nd);   break; // cmpd-imm
  case 0x118C: nd   = FxWord(); SubWords(Sreg,nd);        break; // cmps-imm
  case 0x1183: nd   = FxWord(); SubWords(Ureg,nd);        break; // cmpu-imm
  case 0x8C:   nd   = FxWord(); SubWords(Xreg,nd);        break; // cmpx-imm
  case 0x108C: nd   = FxWord(); SubWords(Yreg,nd);        break; // cmpy-imm
    // CMP-IDX
  case 0xA1:   ea = INDX(L); ev = RdByte(ea); SubBytesCx(Areg,ev,0x00); break; // cmpa-idx
  case 0xE1:   ea = INDX(L); ev = RdByte(ea); SubBytesCx(Breg,ev,0x00); break; // cmpb-idx
  case 0x10A3: ea = INDX(L); ev = RdWord(ea); SubWords(GetDreg(),ev); break;   // cmpd-idx
  case 0x11AC: ea = INDX(L); ev = RdWord(ea); SubWords(Sreg,ev); break;        // cmps-idx
  case 0x11A3: ea = INDX(L); ev = RdWord(ea); SubWords(Ureg,ev); break;        // cmpu-idx
  case 0xAC:   ea = INDX(L); ev = RdWord(ea); SubWords(Xreg,ev); break;        // cmpx-idx
  case 0x10AC: ea = INDX(L); ev = RdWord(ea); SubWords(Yreg,ev); break;        // cmpy-idx
    // CMP-EXT
  case 0xB1:   ea = FxWord(); ev = RdByte(ea);SubBytesCx(Areg,ev,0x00); break; // cmpa-ext
  case 0xF1:   ea = FxWord(); ev = RdByte(ea);SubBytesCx(Breg,ev,0x00); break; // cmpb-ext
  case 0x10B3: ea = FxWord(); ev = RdWord(ea);SubWords(GetDreg(),ev); break;   // cmpd-ext
  case 0x11BC: ea = FxWord(); ev = RdWord(ea);SubWords(Sreg,ev); break;        // cmps-ext
  case 0x11B3: ea = FxWord(); ev = RdWord(ea);SubWords(Ureg,ev); break;        // cmpu-ext
  case 0xBC:   ea = FxWord(); ev = RdWord(ea);SubWords(Xreg,ev); break;        // cmpx-ext
  case 0x10BC: ea = FxWord(); ev = RdWord(ea);SubWords(Yreg,ev); break;        // cmpy-ext
    // COM-DIR
  case 0x03:   ea=DPreg+FxByte(); ev = RdByte(ea); ev ^= 0xFF;                  // com-dir
               StByte(ea,ev); FlagBNZ01(ev); break;       
    // COM-IDX
  case 0x63:   ea = INDX(L); ev = RdByte(ea); ev ^= 0xFF;                       // com-idx
               StByte(ea,ev); FlagBNZ01(ev); break;
    // COM-EXT
  case 0x73:   ea = FxWord(); ev = RdByte(ea); ev ^= 0xFF;                      // com-ext
               StByte(ea,ev); FlagBNZ01(ev); break;       
    // COM-INH
  case 0x43:   Areg = Areg ^ 0xFF; FlagBNZ01(Areg); break;                     // coma-inh
  case 0x53:   Breg = Breg ^ 0xFF; FlagBNZ01(Breg); break;                     // comb-inh
    // CWAI-IMM
  case 0x3C:   nd   = FxByte(); break; // Not implmented yet                   // cwai-imm
    // DAA-INH
  case 0x19:   Areg = DAAfunct(Areg); break;                                   // daa-inh
    // DEC-DIR
  case 0x0A:   ea=DPreg+FxByte(); ev = RdByte(ea); bval = DecByteFnct(ev);      // dec-dir
               StByte(ea,bval); break;
    // DEC-IDX
  case 0x6A:   ea = INDX(L); ev = RdByte(ea); ev = DecByteFnct(ev);             // dec-idx
               StByte(ea,ev); break;
    // DEC-EXT
  case 0x7A:   ea = FxWord(); ev = RdByte(ea); bval = DecByteFnct(ev);          // dec-ext
               StByte(ea,bval); break;
    // DEC-INH
  case 0x4A:   Areg = DecByteFnct(Areg); break;                                // deca-inh
  case 0x5A:   Breg = DecByteFnct(Breg); break;                                // decb-inh
    // EOR-IMM
  case 0x88:   nd   = FxByte(); Areg ^= nd; FlagBNZ0(Areg); break;             // eora-imm
  case 0xC8:   nd   = FxByte(); Breg ^= nd; FlagBNZ0(Breg); break;             // eorb-imm
    // EOR-DIR
  case 0x98:   ea=DPreg+FxByte(); Areg ^= RdByte(ea); FlagBNZ0(Areg); break;   // eora-dir
  case 0xD8:   ea=DPreg+FxByte(); Breg ^= RdByte(ea); FlagBNZ0(Breg); break;   // eora-dir  
    // EOR-IDX
  case 0xA8:   ea = INDX(L);ev = RdByte(ea); Areg ^= ev; FlagBNZ0(Areg);break; // eora-idx
  case 0xE8:   ea = INDX(L);ev = RdByte(ea); Breg ^= ev; FlagBNZ0(Breg);break; // eorb-idx
    // EOR-EXT
  case 0xB8:   ea = FxWord(); Areg ^= RdByte(ea); FlagBNZ0(Areg); break;       // eora-ext
  case 0xF8:   ea = FxWord(); Breg ^= RdByte(ea); FlagBNZ0(Breg); break;       // eora-ext
    // EXG
  case 0x1E:   nd = FxByte(); EXGfnct(L,nd); break;                            // exg
    // INC-DIR
  case 0x0C:   ea=DPreg+FxByte(); ev = RdByte(ea); bval = IncByteFnct(ev);      // inc-dir
                StByte(ea,bval); break;
    // INC-IDX
  case 0x6C:   ea = INDX(L); ev = RdByte(ea); bval = IncByteFnct(ev);           // inc-idx
                StByte(ea,bval); break;
    // INC-EXT
  case 0x7C:   ea = FxWord(); ev = RdByte(ea); bval = IncByteFnct(ev);          // inc-ext
                StByte(ea,bval); break;
    // INC-INH
  case 0x4C:   Areg = IncByteFnct(Areg); break;                                // inca-inh
  case 0x5C:   Breg = IncByteFnct(Breg); break;                                // incb-inh
    // JMP-DIR
  case 0x0E:   ea=DPreg+FxByte(); PCreg = ea; break;                           // jmp-dir
    // JMP-IDX
  case 0x6E:   ea = INDX(L); PCreg = ea; break;                                // jmp-idx
    // JMP-EXT
  case 0x7E:   ea = FxWord(); PCreg = ea; break;                               // jmp-ext
    // JSR-DIR
  case 0x9D:   ea=DPreg+FxByte();Sreg -= 2; StWord(Sreg,PCreg); PCreg = ea; break; // jsr-ext
    // JSR-IDX
  case 0xAD:   ea = INDX(L); Sreg -= 2; StWord(Sreg,PCreg); PCreg = ea; break; // jsr-idx
    // JSR-EXT
  case 0xBD:   ea = FxWord();Sreg -= 2; StWord(Sreg,PCreg); PCreg = ea; break; // jsr-ext
    // LD-IMM
  case 0x86:   Areg = FxByte(); FlagBNZ0(Areg); break;                         // lda-imm
  case 0xC6:   Breg = FxByte(); FlagBNZ0(Breg); break;                         // ldb-imm
  case 0xCC:   Areg = FxByte(); Breg = FxByte(); Flag2NZ0(Areg,Breg); break;   // ldd-imm
  case 0x10CE: Sreg = FxWord(); FlagWNZ0(Sreg); NMI_ARMED_FLAG=1; break;       // lds-imm
  case 0xCE:   Ureg = FxWord(); FlagWNZ0(Ureg); break;                         // ldu-imm
  case 0x8E:   Xreg = FxWord(); FlagWNZ0(Xreg); break;                         // ldx-imm
  case 0x108E: Yreg = FxWord(); FlagWNZ0(Yreg); break;                         // ldy-imm
    // LD-DIR
  case 0x96:   ea=DPreg+FxByte(); Areg = RdByte(ea); FlagBNZ0(Areg); break;    // lda-dir
  case 0xD6:   ea=DPreg+FxByte(); Breg = RdByte(ea); FlagBNZ0(Areg); break;    // ldb-dir
  case 0xDC:   ea=DPreg+FxByte(); ev = RdWord(ea); PutDreg(ev); FlagWNZ0(ev);break;// ldd-dir
  case 0x10DE: ea=DPreg+FxByte(); Sreg = RdWord(ea); FlagWNZ0(Sreg);            // lds-dir
                NMI_ARMED_FLAG=1; break;
  case 0xDE:   ea=DPreg+FxByte(); Ureg = RdWord(ea); FlagWNZ0(Ureg); break;    // ldu-dir
  case 0x9E:   ea=DPreg+FxByte(); Xreg = RdWord(ea); FlagWNZ0(Xreg); break;    // ldx-dir
  case 0x109E: ea=DPreg+FxByte(); Yreg = RdWord(ea); FlagWNZ0(Yreg); break;    // ldy-dir
    // LD-IDX
  case 0xA6:   ea = INDX(L); Areg = RdByteFnct(L,ea); FlagBNZ0(Areg); break;   // lda-idx
  case 0xE6:   ea = INDX(L); Breg = RdByteFnct(L,ea); FlagBNZ0(Areg); break;   // ldb-idx
  case 0xEC:   ea = INDX(L); ev = RdWordFnct(L,ea); PutDreg(ev); FlagWNZ0(ev); break; // ldd-idx
  case 0x10EE: ea = INDX(L); Sreg = RdWordFnct(L,ea); FlagWNZ0(Sreg);           // lds-idx
                NMI_ARMED_FLAG=1; break;
  case 0xEE:   ea = INDX(L); Ureg = RdWordFnct(L,ea); FlagWNZ0(Ureg); break;   // ldu-idx   
  case 0xAE:   ea = INDX(L); Xreg = RdWordFnct(L,ea); FlagWNZ0(Xreg); break;   // ldx-idx 
  case 0x10AE: ea = INDX(L); Yreg = RdWordFnct(L,ea); FlagWNZ0(Yreg); break;   // ldy-idx
    // LD-EXT
  case 0xB6:   ea = FxWord(); Areg = RdByte(ea); FlagBNZ0(Areg); break;        // lda-ext
  case 0xF6:   ea = FxWord(); Breg = RdByte(ea); FlagBNZ0(Areg); break;        // ldb-ext
  case 0xFC:   ea = FxWord(); ev = RdWord(ea); PutDreg(ev); FlagWNZ0(ev);break;// ldd-ext
  case 0x10FE: ea = FxWord(); Sreg = RdWord(ea); FlagWNZ0(Sreg);                // lds-ext 
                NMI_ARMED_FLAG=1; break; 
  case 0xFE:   ea = FxWord(); Ureg = RdWord(ea); FlagWNZ0(Ureg); break;        // ldu-ext   
  case 0xBE:   ea = FxWord(); Xreg = RdWord(ea); FlagWNZ0(Xreg); break;        // ldx-ext 
  case 0x10BE: ea = FxWord(); Yreg = RdWord(ea); FlagWNZ0(Yreg); break;        // ldy-ext
    // LEA-IDX
  case 0x32:   Sreg = INDX(L); NMI_ARMED_FLAG=1; break;                                          // leas-idx
  case 0x33:   Ureg = INDX(L); break;                                          // leau-idx
  case 0x30:   Xreg = INDX(L); CC2Zflag = (Xreg)? 0x0 : CCREG_Z_FLAG; break;   // leax-idx
  case 0x31:   Yreg = INDX(L); CC2Zflag = (Yreg)? 0x0 : CCREG_Z_FLAG; break;   // leay-idx
    // LSR-DIR
  case 0x04:   ea=DPreg+FxByte(); ev = RdByte(ea); bval = LSRfnct(ev);          // lsr-dir
                StByte(ea,bval); break;
    // LSR-IDX
  case 0x64:   ea = INDX(L); ev = RdByte(ea); bval = LSRfnct(ev);               // lsr-idx
               StByte(ea,bval); break;
    // LSR-EXT
  case 0x74:   ea = FxWord(); ev = RdByte(ea); bval = LSRfnct(ev);              // lsr-ext
                StByte(ea,bval); break;
    // LSR-INH
  case 0x44:   Areg = LSRfnct(Areg); break;                                    // lsra-inh
  case 0x54:   Breg = LSRfnct(Breg); break;                                    // lsrb-inh
    // MUL-INH
  case 0x3D:   bval = Areg*Breg; PutDreg(bval);                                 // mul-inh
               CC2Zflag = (bval)?      0x0: CCREG_Z_FLAG;
               CC0Cflag = (bval&0x80)? CCREG_C_FLAG: 0x0; break;
    // NEG-DIR
  case 0x00:   ea=DPreg+FxByte(); ev=RdByte(ea); bval=NegByteFnct(ev);          // dec-dir
               StByte(ea,bval); break;
    // NEG-IDX
  case 0x60:  ea = INDX(L); ev = RdByte(ea);   bval = NegByteFnct(ev);          // dec-idx
               StByte(ea,bval); break;
    // NEG-EXT
  case 0x70:   ea = FxWord(); ev = RdByte(ea); bval = NegByteFnct(ev);          // dec-ext
               StByte(ea,bval); break;
    // NEG-INH
  case 0x40:   Areg = NegByteFnct(Areg); break;                                // deca
  case 0x50:   Breg = NegByteFnct(Breg); break;                                // decb
    // NOP
  case 0x12: break;                                                            // nop
    // OR-IMM
  case 0x8A:   nd = FxByte(); Areg |= nd; FlagBNZ0(Areg); break;               // ora-imm
  case 0xCA:   nd = FxByte(); Breg |= nd; FlagBNZ0(Breg); break;               // orb-imm
  case 0x1A:   nd = FxByte(); PutCreg( GetCreg() | nd );  break;               // orcc-imm
    // OR-DIR
  case 0x9A:   ea=DPreg+FxWord(); Areg |= RdByte(ea);FlagBNZ0(Areg);break;     // anda-dir
  case 0xDA:   ea=DPreg+FxWord(); Breg |= RdByte(ea);FlagBNZ0(Breg);break;     // andb-dir
    // OR-IDX
  case 0xAA:   ea = INDX(L); Areg |=  RdByte(ea); FlagBNZ0(Areg); break;       // ora-idx
  case 0xEA:   ea = INDX(L); Breg |=  RdByte(ea); FlagBNZ0(Areg); break;       // orb-idx
    // OR-EXT
  case 0xBA:   ea = FxWord(); Areg |= RdByte(ea); FlagBNZ0(Areg); break;       // anda-ext
  case 0xFA:   ea = FxWord(); Breg |= RdByte(ea); FlagBNZ0(Breg); break;       // andb-ext
    // PSH
  case 0x34:   nd   = FxByte(); PSHfnct(L,&Sreg,Ureg,nd);   break;             // pshs
  case 0x36:   nd   = FxByte(); PSHfnct(L,&Ureg,Sreg,nd);   break;             // pshu
    // PUL
  case 0x35:   nd   = FxByte(); PULfnct(L,&Sreg,&Ureg,nd);                      // puls
                NMI_ARMED_FLAG=1; break;
  case 0x37:   nd   = FxByte(); PULfnct(L,&Ureg,&Sreg,nd);                      // pulu
                NMI_ARMED_FLAG=1; break;
    // ROL-DIR
  case 0x09:   ea=DPreg+FxByte(); ev=RdByte(ea); bval=ROLfnct(ev);              // rol-dir
                StByte(ea,bval); break; 
    // ROL-IDX
  case 0x69:   ea = INDX(L); ev = RdByte(ea); bval = ROLfnct(ev);               // rol-idx
               StByte(ea,bval); break; 
    // ROL-EXT
  case 0x79:   ea = FxWord(); ev = RdByte(ea); bval = ROLfnct(ev);              // rol-ext
               StByte(ea,bval); break; 
    // ROL-INH
  case 0x49:   Areg = ROLfnct(Areg); break;                                    // rola-inh
  case 0x59:   Breg = ROLfnct(Breg); break;                                    // rolb-inh
    // ROR-DIR
  case 0x06:   ea=DPreg+FxByte(); ev=RdByte(ea); bval=RORfnct(ev);              // ror-dir
                StByte(ea,bval); break;
    // ROR-IDX
  case 0x66:   ea = INDX(L); ev = RdByte(ea); bval = RORfnct(ev);               // ror-idx
                StByte(ea,bval); break;
    // ROR-EXT
  case 0x76:   ea = FxWord(); ev = RdByte(ea); bval = RORfnct(ev);              // ror-ext
                StByte(ea,bval); break;
    // ROR-INH
  case 0x46:   Areg = ROLfnct(Areg); break;                                    // rora-inh
  case 0x56:   Breg = ROLfnct(Breg); break;                                    // rorb-inh
    // RTI-INH
  case 0x3B:   RTIfnct(L); break;                                              // rti-inh
    // RTS-INH
  case 0x39:   PCreg = RdWord(Sreg); Sreg += 2; NMI_ARMED_FLAG=1; break;       // rts-inh
    // SBC-IMM
  case 0x82:   nd = FxByte(); Areg = SubBytesCx(Areg,nd,CC0Cflag); break;      // sbca-imm
  case 0xC2:   nd = FxByte(); Breg = SubBytesCx(Breg,nd,CC0Cflag); break;      // sbcb-imm
    // SBC-DIR
  case 0x92:   ea=DPreg+FxByte();                                               // sbca-dir
                Areg=SubBytesCx(Areg,RdByte(ea),CC0Cflag);break;
  case 0xD2:   ea=DPreg+FxByte();                                               // sbcb-dir
                Breg=SubBytesCx(Breg,RdByte(ea),CC0Cflag);break;
    // SBC-IDX
  case 0xA2:   ea = INDX(L); ev = RdByte(ea);                                   // sbca-idx
                Areg = SubBytesCx(Areg,ev,CC0Cflag); break; 
  case 0xE2:   ea = INDX(L); ev = RdByte(ea);                                   // sbcb-idx
                Breg = SubBytesCx(Breg,ev,CC0Cflag); break; 
    // SBC-EXT
  case 0xB2:   ea=FxWord(); Areg = SubBytesCx(Areg,RdByte(ea),CC0Cflag);break; // sbca-ext
  case 0xF2:   ea=FxWord(); Breg = SubBytesCx(Breg,RdByte(ea),CC0Cflag);break; // sbcb-ext
    // SEX-INH
  case 0x1D:   Areg = (Breg < 0x80)? 0x00: 0xFF; FlagWNZ0(GetDreg()); break;   // sex-inh
    // ST-DIR
  case 0x97:   ea=DPreg+FxByte(); StByte(ea,Areg); FlagBNZ0(Areg); break;      // sta-dir
  case 0xD7:   ea=DPreg+FxByte(); StByte(ea,Breg); FlagBNZ0(Breg); break;      // stb-dir
  case 0xDD:   ea=DPreg+FxByte(); dx=GetDreg(); StWord(ea,dx); FlagWNZ0(dx); break; // std-dir
  case 0x10DF: ea=DPreg+FxByte(); StWord(ea,Sreg); FlagWNZ0(Sreg); break;      // sts-dir
  case 0xDF:   ea=DPreg+FxByte(); StWord(ea,Ureg); FlagWNZ0(Ureg); break;      // stu-dir
  case 0x9F:   ea=DPreg+FxByte(); StWord(ea,Xreg); FlagWNZ0(Xreg); break;      // stx-dir
  case 0x109F: ea=DPreg+FxByte(); StWord(ea,Yreg); FlagWNZ0(Yreg); break;      // sty-dir
    // ST-IDX
  case 0xA7:   ea = INDX(L);  StByte(ea,Areg); FlagBNZ0(Areg); break;          // sta-idx
  case 0xE7:   ea = INDX(L);  StByte(ea,Breg); FlagBNZ0(Breg); break;          // stb-idx
  case 0xED:   ea = INDX(L);  dx=GetDreg(); StWord(ea,dx); FlagWNZ0(dx); break;// std-idx
  case 0x10EF: ea = INDX(L);  StWord(ea,Sreg); FlagWNZ0(Sreg); break;          // sts-idx
  case 0xEF:   ea = INDX(L);  StWord(ea,Ureg); FlagWNZ0(Ureg); break;          // stu-idx
  case 0xAF:   ea = INDX(L);  StWord(ea,Xreg); FlagWNZ0(Xreg); break;          // stx-idx
  case 0x10AF: ea = INDX(L);  StWord(ea,Yreg); FlagWNZ0(Yreg); break;          // sty-idx
    // ST-EXT
  case 0xB7:   ea = FxWord(); StByte(ea,Areg); FlagBNZ0(Areg); break;          // sta-ext   
  case 0xF7:   ea = FxWord(); StByte(ea,Breg); FlagBNZ0(Breg); break;          // stb-ext   
  case 0xFD:   ea = FxWord(); dx=GetDreg(); StWord(ea,dx); FlagWNZ0(dx); break;// std-ext   
  case 0x10FF: ea = FxWord(); StWord(ea,Sreg); FlagWNZ0(Sreg); break;          // sts-ext
  case 0xFF:   ea = FxWord(); StWord(ea,Ureg); FlagWNZ0(Ureg); break;          // stu-ext
  case 0xBF:   ea = FxWord(); StWord(ea,Xreg); FlagWNZ0(Xreg); break;          // stx-ext
  case 0x10BF: ea = FxWord(); StWord(ea,Yreg); FlagWNZ0(Yreg); break;          // sty-ext
    // SUB-IMM
  case 0x80:   nd = FxByte(); Areg = SubBytesCx(Areg,nd,0x00); break;          // suba-imm
  case 0xC0:   nd = FxByte(); Breg = SubBytesCx(Breg,nd,0x00); break;          // subb-imm
  case 0x83:   nd = FxWord(); PutDreg(SubWords(GetDreg(),nd)); break;          // subd-imm
    // SUB-DIR
  case 0x90:   ea=DPreg+FxByte();                                      // suba-ext
                Areg = SubBytesCx(Areg,RdByte(ea),0x00); break; 
  case 0xD0:   ea=DPreg+FxByte();                                      // subb-ext
                Breg = SubBytesCx(Breg,RdByte(ea),0x00); break;
  case 0x93:   ea=DPreg+FxByte();                                      // subd-ext
                PutDreg(SubWords(GetDreg(),RdWord(ea))); break;
    // SUB-IDX
  case 0xA0:   ea = INDX(L); ev = RdByte(ea);                          // suba-idx
               Areg = SubBytesCx(Areg,ev,0x00); break;
  case 0xE0:   ea = INDX(L); ev = RdByte(ea);                          // subb-idx
               Breg = SubBytesCx(Breg,ev,0x00); break;
  case 0xA3:   ea = INDX(L); ev = RdByte(ea);                          // subd-idx
               PutDreg(SubWords(GetDreg(),ev)); break;
    // SUB-EXT
  case 0xB0:   ea = FxWord(); Areg = SubBytesCx(Areg,RdByte(ea),0x00); break; // suba-ext
  case 0xF0:   ea = FxWord(); Breg = SubBytesCx(Breg,RdByte(ea),0x00); break; // subb-ext
  case 0xB3:   ea = FxWord(); PutDreg(SubWords(GetDreg(),RdWord(ea))); break; // subd-ext
    // SWI-INH
  case 0x3F:   Intfnct(L,INTTYPE_SWI);  break;                        // swi-inh
  case 0x103F: Intfnct(L,INTTYPE_SWI2); break;                        // swi2-inh
  case 0x113F: Intfnct(L,INTTYPE_SWI3); break;                        // swi3-inh
      // SYNC-INH
  case 0x13:   break;  // Not implmented yet                          // sync-inh
    // TFR
  case 0x1F:   nd = FxByte(); TFRfnct(L,nd); break;                   //tfr
    // TST-DIR
  case 0x0D:   ea=DPreg+FxByte(); ev=RdByte(ea); FlagBNZ0(Areg); break; // tst-ext 
    // TST-IDX
  case 0x6D:   ea = INDX(L); ev = RdByte(ea); FlagBNZ0(Areg); break;  // tst-idx
    // TST-EXT
  case 0x7D:   ea = FxWord(); ev = RdByte(ea); FlagBNZ0(Areg); break; // tst-ext
    // TST-INH
  case 0x4D:   FlagBNZ0(Areg); break;                                 // tsta-inh
  case 0x5D:   FlagBNZ0(Breg); break;                                 // tstb-inh
    // Branches
  case 0x24:   nd   = FxByte(); if(!CC0Cflag)
                                    PCreg = AddEx(nd); break;          // bcc-bhs
  case 0x25:   nd   = FxByte(); if( CC0Cflag)
                                    PCreg = AddEx(nd); break;          // bcs-blo
  case 0x27:   nd   = FxByte(); if( CC2Zflag)
                                    PCreg = AddEx(nd); break;          // beq
  case 0x2C:   nd   = FxByte(); if(!((!CC3Nflag && CC1Vflag) || (CC3Nflag && !CC1Vflag)))
                                    PCreg = AddEx(nd); break;          // beg
  case 0x2E:   nd   = FxByte(); if(!(CC2Zflag || (!CC3Nflag && CC1Vflag) || (CC3Nflag && !CC1Vflag)))
                                    PCreg = AddEx(nd); break;          // bgt
  case 0x22:   nd   = FxByte(); if(!(CC2Zflag || CC1Vflag))
                                    PCreg = AddEx(nd); break;          // bhi
  case 0x2F:   nd   = FxByte(); if( CC2Zflag || (!CC3Nflag && CC1Vflag) || (CC3Nflag && !CC1Vflag))
                                    PCreg = AddEx(nd); break;          // ble
  case 0x23:   nd   = FxByte(); if( CC2Zflag || CC0Cflag)
                                    PCreg = AddEx(nd); break;          // bls
  case 0x2D:   nd   = FxByte(); if((!CC3Nflag && CC1Vflag) || (CC3Nflag && !CC1Vflag))
                                    PCreg = AddEx(nd); break;          // blt
  case 0x2B:   nd   = FxByte(); if( CC3Nflag)
                                    PCreg = AddEx(nd); break;          // bmi
  case 0x26:   nd   = FxByte(); if(!CC2Zflag)
                                    PCreg = AddEx(nd); break;          // bne
  case 0x2A:   nd   = FxByte(); if(!CC3Nflag)
                                    PCreg = AddEx(nd); break;          // bpl
  case 0x20:   nd   = FxByte();    PCreg = AddEx(nd); break;          // bra
  case 0x21:   nd   = FxByte(); break;                                // brn
  case 0x28:   nd   = FxByte(); if(!CC1Vflag)
                                     PCreg = AddEx(nd); break;         // bvc
  case 0x29:   nd   = FxByte(); if( CC1Vflag)
                                    PCreg = AddEx(nd); break;          // bvs
  default: printf("Unknown opcode PC: 0x%.4X, code: 0x%.2X\n",(UWORD)PCtmp,op);
  }
  lua_settop(L,0);
  lua_checkstack (L,1);
  lua_pushinteger(L,PCreg);
  return 1;
}

UWORD INDX(lua_State *L)
{
  UWORD ea,ff;
  UWORD nd = FxByte();
  if (!(nd & 0x80)){                        // 5-bit offset
    ff = nd & 0x1F;
    if (ff & 0x10) ff += 0xFFE0;
    ea = IndxGetReg(nd) + ff;
  }
  else{ switch(nd & 0x0F){
    case 0x4: ea = IndxGetReg(nd); break;  // no offset
    case 0x8: ff = FxByte();                // 8-bit offset
      if (ff & 0x80) ff += 0xFF00;
      ea = IndxGetReg(nd) + ff; break;
    case 0x9: ff = FxWord();                // 16-bit offset
      ea = IndxGetReg(nd) + ff; break;
    case 0x6: ff = Areg;                    // A-reg. offset
      if (ff & 0x80) ff += 0xFF00;
      ea = IndxGetReg(nd) + ff; break;
    case 0x5: ff = Breg;                    // B-reg offset
      if (ff & 0x80) ff += 0xFF00;
      ea = IndxGetReg(nd) + ff; break;
    case 0xB: ff = GetDreg();               // D-reg offset
      ea = IndxGetReg(nd) + ff; break;
    case 0x0: ea = IndxGetReg(nd);          // Inc. by one
      IndxPutReg(nd,ea+1); break;
    case 0x1: ea = IndxGetReg(nd);          // Inc. by two
      IndxPutReg(nd,ea+2); break;
    case 0x2: ea = IndxGetReg(nd)-1;        // Dec. by one
      IndxPutReg(nd,ea); break;
    case 0x3: ea = IndxGetReg(nd)-2;        // Dec. by two
      IndxPutReg(nd,ea); break;
    case 0xC: ff = FxByte();                // 8-bit offset with PC
      if (ff & 0x80) ff += 0xFF00;
      ea = PCreg + ff; break;
    case 0xD: ff = FxWord();                 // 16 bit offset with PC
      ea = PCreg + ff; break;
    case 0xF: ea = FxWord();                 // Ext
    }
    if (nd & 0x10){                           // Indirect
      ea = RdWordFnct(L,ea);
    }
  }
  return ea;
}

UWORD IndxGetReg(UWORD nd)
{ 
  UWORD val;
  switch(nd & 0x60){
  case 0x00: val = Xreg; break;
  case 0x20: val = Yreg; break;
  case 0x40: val = Ureg; break;
  default:   val = Sreg; break;
  }
  return val;
}

void IndxPutReg(UWORD nd, UWORD rval)
{ 
  switch(nd & 0x60){
  case 0x00: Xreg = rval; break;
  case 0x20: Yreg = rval; break;
  case 0x40: Ureg = rval; break;
  default:   Sreg = rval; NMI_ARMED_FLAG=1; break;
  }
}

UWORD AddExFnct(UWORD addx,UWORD nd)
{
  if (nd >= 0x80) nd = nd + 0xFF00;
  return addx+nd;
}

UWORD RdByteFnct(lua_State *L,UWORD addx)
{
  UWORD err,bval =  MemsReadByte(addx,&err);
  if (err){
    char errbuff[120];
    char *msgstr  = MemsFlagStr(bval);
    sprintf(errbuff,"CPU: PC: 0x%.4X Addx: 0x%.4X '%s' read access",
            (UWORD)PCtmp,addx,msgstr);
    lua_pushstring(L,errbuff);
    lua_error(L);
  }
  return bval;
}

UWORD RdWordFnct(lua_State *L,UWORD addx)
{
  UWORD aval,bval;
  aval =  RdByteFnct(L,addx); addx++;
  bval =  RdByteFnct(L,addx);
  return (aval<<8)+bval;
}

void StByteFnct(lua_State *L,UWORD addx, UWORD bval)
{
  UWORD rx = MemsStoreByte(addx,bval);
  if (rx == BYTE_NO_REGION || rx == BYTE_STORE_ROM) {
    StoreError(L,addx,bval);
  }
}

void StWordFnct(lua_State *L,UWORD addx, UWORD bval)
{
  UWORD rx,b1,b0;
  b1 = bval >> 8; b0 = bval &0xFF;
  rx = MemsStoreByte(addx,b1); 
  if (rx == BYTE_NO_REGION || rx == BYTE_STORE_ROM) {
    StoreError(L,addx,bval);
  }
  addx = (addx +1) & 0xFFFF;
  rx = MemsStoreByte(addx,b0); 
  if (rx == BYTE_NO_REGION || rx == BYTE_STORE_ROM) {
    StoreError(L,addx,bval);
  }
}

void StoreError(lua_State *L,UWORD addx,UWORD bval)
{
  char errbuff[120];
  char *msgstr  = MemsFlagStr(bval);
  sprintf(errbuff,"CPU: PC: 0x%.4X Addx: 0x%.4X '%s' write access",
          (UWORD)PCtmp,addx,msgstr);
  lua_pushstring(L,errbuff);
  lua_error(L);
}

UWORD AddBytesCx(UWORD aval, UWORD bval, UWORD flag)
{
  // Break up the numbers
  UWORD a2 = aval & 0x80, b2 = bval & 0x80;
  UWORD a1 = aval & 0x70, b1 = bval & 0x70;
  UWORD a0 = aval & 0x0F, b0 = bval & 0x0F;
  UWORD sx,s2,s1,s0,cx8,cx7;

  // Add up the pieces
  s0 = a0 + b0 + ((flag)?1:0);
  s1 = a1 + b1 + s0;  cx7 = (s1 & 0x080)? CCREG_C_FLAG:0x00;
  s2 = a2 + b2 + s1;  cx8 = (s2 & 0x100)? CCREG_C_FLAG:0x00;
  sx = s2 & 0xFF;
  
  // Handle the flags
  CC5Hflag = (s0 & 0x10)? CCREG_H_FLAG : 0x00;
  CC3Nflag = (sx & 0x80)? CCREG_N_FLAG : 0x00;
  CC2Zflag = (sx)?        0x00 : CCREG_Z_FLAG;
  CC1Vflag = (cx8 ^ cx7)? CCREG_V_FLAG : 0x00;
  CC0Cflag = cx8;

  // All done
  return( 0xFF & s2 );
}

UWORD AddWords(UWORD aval, UWORD bval)
{
  // Break up the numbers
  int a2 = aval & 0x8000, b2 = bval & 0x8000;
  int a1 = aval & 0x7FFF, b1 = bval & 0x7FFF;
  int sx,s2,s1,c16,c15;

  // Add up the pieces
  s1 = a1 + b1;      c15 = (s1 & 0x08000)? CCREG_C_FLAG:0x00;
  s2 = a2 + b2 + s1; c16 = (s2 & 0x10000)? CCREG_C_FLAG:0x00;
  sx = s2 & 0xFFFF;

  // Handle the flags
  CC3Nflag = (sx & 0x8000)? CCREG_N_FLAG : 0x00;
  CC2Zflag = (sx)?          0x00 : CCREG_Z_FLAG;
  CC1Vflag = (c16 ^ c15)?   CCREG_V_FLAG : 0x00;
  CC0Cflag = c16;

  // All done
  return sx;
}

UWORD ROLfnct(UWORD aval)
{
  UWORD tmp = (CC0Cflag)? 1: 0;
  UWORD bval = (aval << 1) + tmp;
  UWORD cval = bval & 0xFF;
  
  CC3Nflag = (cval & 0x80)?          CCREG_N_FLAG : 0x00;
  CC2Zflag = (cval)?                 0x00 : CCREG_Z_FLAG;
  CC1Vflag = ((aval&0x80) ^ (bval&0x80))? CCREG_V_FLAG : 0x00;
  CC0Cflag = (bval & 0x100)?          CCREG_C_FLAG : 0;
  return cval;
}

UWORD RORfnct(UWORD aval)
{
  UWORD tmp = (CC0Cflag)? 0x80:0;
  UWORD bval = tmp + (aval >> 1);
  
  CC3Nflag = (bval & 0x80)?          CCREG_N_FLAG : 0x00;
  CC2Zflag = (bval)?                 0x00 : CCREG_Z_FLAG;
  CC0Cflag = (bval & 0x01)? CCREG_C_FLAG : 0;  
  return bval;
}

UWORD SubBytesCx(UWORD aval, UWORD bval, UWORD flag)
{
  // Break up the numbers
  UWORD a2 = aval & 0x80, b2 = bval & 0x80;
  UWORD a1 = aval & 0x70, b1 = bval & 0x70;
  UWORD a0 = aval & 0x0F, b0 = bval & 0x0F;
  UWORD sx,s2,s1,s0,cx8,cx7;

  // Add up the pieces
  s0 = a0 - (b0 +  ((flag)?1:0));
  s1 = a1 - b1 + s0;  cx7 = (s1 & 0x080)? CCREG_C_FLAG:0x00;
  s2 = a2 - b2 + s1;  cx8 = (s2 & 0x100)? CCREG_C_FLAG:0x00;
  sx = s2 & 0xFF;
  
  // Handle the flags
  CC3Nflag = (sx & 0x80)? CCREG_N_FLAG : 0x00;
  CC2Zflag = (sx)?        0x00 : CCREG_Z_FLAG;
  CC1Vflag = (cx8 ^ cx7)? CCREG_V_FLAG : 0x00;
  CC0Cflag = cx8;

  // All done
  return sx;
}

UWORD SubWords(UWORD aval, UWORD bval)
{
  // Break up the numbers
  int a2 = aval & 0x8000, b2 = bval & 0x8000;
  int a1 = aval & 0x7FFF, b1 = bval & 0x7FFF;
  int sx,s2,s1,c16,c15;

  // Add up the pieces
  s1 = a1 - b1;      c15 = (s1 & 0x08000)? CCREG_C_FLAG:0x00;
  s2 = a2 - b2 + s1; c16 = (s2 & 0x10000)? CCREG_C_FLAG:0x00;
  sx = s2 & 0xFFFF;

  // Handle the flags
  CC3Nflag = (sx & 0x8000)? CCREG_N_FLAG : 0x00;
  CC2Zflag = (sx)?          0x00 : CCREG_Z_FLAG;
  CC1Vflag = (c16 ^ c15)?   CCREG_V_FLAG : 0x00;
  CC0Cflag = c16;

  // All done
  return sx;
}

UWORD ASLfnct(UWORD aval)
{
  UWORD bval = aval << 1;
  UWORD cval = bval & 0xFF;
  
  CC3Nflag = (cval & 0x80)? CCREG_N_FLAG : 0x00;
  CC2Zflag = (cval)?        0x00   : CCREG_Z_FLAG;
  CC1Vflag = ((aval ^ bval)&0x80)? : CCREG_V_FLAG;
  CC0Cflag = (bval & 0x100)? CCREG_C_FLAG : 0x00;
  return cval;
}

UWORD ASRfnct(UWORD aval)
{
  UWORD bval;
  bval = (aval & 0x80) + (aval >> 1);
  
  CC3Nflag = (bval & 0x80)? CCREG_N_FLAG : 0x00;
  CC2Zflag = (bval)?         0x00 : CCREG_Z_FLAG;
  CC0Cflag = (bval & 0x001)? CCREG_C_FLAG : 0x00;
  return bval;
}

// Consider carry-in as well as carry out
UWORD DAAfunct(UWORD aval)
{
  UWORD a1 = aval & 0xF0;
  UWORD a0 = aval & 0x0F;
  UWORD bval;
  
  // Consider cases
  if (CC5Hflag || a0 >= 0x0A) a0 += 0x06;
  a1 += a0;   // possible new carry-in
  if (CC0Cflag || a1 >= 0xA0) a1 += 0x60;
  bval = a1 & 0xFF;
  
  // Handle flags
  CC3Nflag = (bval & 0x80)? CCREG_N_FLAG : 0x00;
  CC2Zflag = (bval)?         0x00 : CCREG_Z_FLAG;
  CC1Vflag = 0x00;
  CC0Cflag |= (a1 & 0x100)? CCREG_C_FLAG : 0x00;
  
  // Done
  return bval;
}

UWORD DecByteFnct(UWORD bval)
{
   // Break up the number
   UWORD a2 = bval & 0x80;
   UWORD a1 = bval & 0x7F;
   UWORD cx8,cx7,sx,s2,s1;

   // Combine the pieces
   s1 = a1 - 0x01;  cx7 = (s1 & 0x080)? CCREG_C_FLAG:0x00;
   s2 = a2 + s1;    cx8 = (s2 & 0x100)? CCREG_C_FLAG:0x00;
   sx = s2 & 0xFF;

   // Handle the flags
   CC3Nflag = (sx & 0x80)? CCREG_N_FLAG : 0x00;
   CC2Zflag = (sx)?        0x00 : CCREG_Z_FLAG;
   CC1Vflag = (cx8 ^ cx7)? CCREG_V_FLAG : 0x00;
   return sx;
}

UWORD IncByteFnct(UWORD bval)
{
   // Break up the number
   UWORD a2 = bval & 0x80;
   UWORD a1 = bval & 0x7F;
   UWORD cx8,cx7,sx,s2,s1;

   // Combine the pieces
   s1 = a1 + 0x01;  cx7 = (s1 & 0x080)? CCREG_C_FLAG:0x00;
   s2 = a2 + s1;    cx8 = (s2 & 0x100)? CCREG_C_FLAG:0x00;
   sx = s2 & 0xFF;

   CC3Nflag = (sx & 0x80)? CCREG_N_FLAG : 0x00;
   CC2Zflag = (sx)?        0x00 : CCREG_Z_FLAG;
   CC1Vflag = (cx8 ^ cx7)? CCREG_V_FLAG : 0x00;
   return sx;
}

UWORD LSRfnct(UWORD aval)
{
  UWORD bval;
  bval = aval >> 1;
  
  CC3Nflag = 0x00;
  CC2Zflag = (bval)?         0x00 : CCREG_Z_FLAG;
  CC0Cflag = (aval & 0x001)? CCREG_C_FLAG : 0x00;
  return bval;
}

UWORD NegByteFnct(UWORD bval)
{
  // Flip the bits
  bval ^= 0x00FF;
   
  // Break up the number
  UWORD a2 = bval & 0x80;
  UWORD a1 = bval & 0x7F;

  // Combine the pieces
  UWORD c8,c7,sx,s2,s1;
  s1 = a1 + 0x01;  c7 = (s1 & 0x080)? CCREG_C_FLAG:0x00;
  s2 = a2 + s1;    c8 = (s2 & 0x100)? CCREG_C_FLAG:0x00;
  sx = s2 & 0xFF;

  // Handle the flags
  CC3Nflag = (sx & 0x080)? CCREG_N_FLAG : 0x0;
  CC2Zflag = (sx)?         0x0 : CCREG_Z_FLAG;
  CC1Vflag = (c8 ^ c7)?    CCREG_V_FLAG : 0x0;
  CC0Cflag = c8;
  return sx;
}

void  PSHfnct(lua_State *L, ULONG *pstk, UWORD ostk, UWORD flags)
{
  UWORD px = *pstk;
  if (flags & PP_PC_BIT) {px = px-2; StWordFnct(L,px,PCreg);}
  if (flags & PP_SU_BIT) {px = px-2; StWordFnct(L,px,ostk);}
  if (flags & PP_Y_BIT)  {px = px-2; StWordFnct(L,px,Yreg);}
  if (flags & PP_X_BIT)  {px = px-2; StWordFnct(L,px,Xreg);}
  if (flags & PP_DP_BIT) {px = px-1; StByteFnct(L,px,DPreg);}
  if (flags & PP_B_BIT)  {px = px-1; StByteFnct(L,px,Breg);}
  if (flags & PP_A_BIT)  {px = px-1; StByteFnct(L,px,Areg);}
  if (flags & PP_CC_BIT) {px = px-1; StByteFnct(L,px,GetCreg());}
  *pstk = px;
}

void  PULfnct(lua_State *L, ULONG *pstk, ULONG *ostk, UWORD flags)
{
  UWORD px = *pstk;
  if (flags & PP_CC_BIT) {PutCreg( RdByteFnct(L,px) ); px = px+1;}
  if (flags & PP_A_BIT)  {Areg  = RdByteFnct(L,px);    px = px+1;}
  if (flags & PP_B_BIT)  {Breg  = RdByteFnct(L,px);    px = px+1;}
  if (flags & PP_DP_BIT) {DPreg = RdByteFnct(L,px)<<8; px = px+1;}
  if (flags & PP_X_BIT)  {Xreg  = RdWordFnct(L,px);    px = px+2;}
  if (flags & PP_Y_BIT)  {Yreg  = RdWordFnct(L,px);    px = px+2;}
  if (flags & PP_SU_BIT) {*ostk = RdWordFnct(L,px);    px = px+2;}
  if (flags & PP_PC_BIT) {PCreg = RdWordFnct(L,px);    px = px+2;}
  *pstk = px;
}

void  RTIfnct(lua_State *L)
{
  UWORD px = Sreg;

  PutCreg( RdByteFnct(L,px) ); px = px+1;
  if (CC7Eflag){
    Areg  = RdByteFnct(L,px);    px = px+1;
    Breg  = RdByteFnct(L,px);    px = px+1;
    DPreg = RdByteFnct(L,px)<<8; px = px+1;
    Xreg  = RdWordFnct(L,px);    px = px+2;
    Yreg  = RdWordFnct(L,px);    px = px+2;
    Ureg  = RdWordFnct(L,px);    px = px+2;
  }
  PCreg = RdWordFnct(L,px); px = px+2;
  Sreg = px; NMI_ARMED_FLAG=1;
}

void  Intfnct(lua_State *L, UWORD SwiType)
{
  // Push at least two CPU registers
  if (!(SwiType == INTTYPE_RESET ||
        (SwiType == INTTYPE_NMI  && NMI_ARMED_FLAG ==0))){
    UWORD px = Sreg;
    px = px-2; StWordFnct(L,px,PCreg);
    if (SwiType != INTTYPE_FIRQ){
      px = px-2; StWordFnct(L,px,Ureg);
      px = px-2; StWordFnct(L,px,Yreg);
      px = px-2; StWordFnct(L,px,Xreg);
      px = px-1; StByteFnct(L,px,DPreg);
      px = px-1; StByteFnct(L,px,Breg);
      px = px-1; StByteFnct(L,px,Areg);
      px = px-1; StByteFnct(L,px,GetCreg());
      CC7Eflag = CCREG_E_FLAG;   // All CPU regs pushed
    }
    else{
      px = px-1; StByteFnct(L,px,GetCreg());
      CC7Eflag = 0x00;           // Twp CPU regs pushed
    }
    Sreg = px; NMI_ARMED_FLAG = 1;
  }
  switch(SwiType){
  case INTTYPE_RESET:
    DPreg = 0;                 // Clear direct page
    NMI_ARMED_FLAG = 0x0;      // Disarm NMI
    CC6Fflag = CCREG_F_FLAG;   // prevent F ints
    CC4Iflag = CCREG_I_FLAG;   // prevent I ints
    PCreg = RdWordFnct(L,INTVECT_RESET); break;
  case INTTYPE_NMI:
    if (!NMI_ARMED_FLAG) break;
    CC6Fflag = CCREG_F_FLAG;   // prevent F ints
    CC4Iflag = CCREG_I_FLAG;   // prevent I ints 
    PCreg = RdWordFnct(L,INTVECT_NMI); break;
  case INTTYPE_SWI:
    CC6Fflag = CCREG_F_FLAG;   // prevent F ints
    CC4Iflag = CCREG_I_FLAG;   // prevent I ints
    PCreg = RdWordFnct(L,INTVECT_SWI); break;
  case INTTYPE_IRQ:
    CC6Fflag = CCREG_F_FLAG;   // prevent F ints
    PCreg = RdWordFnct(L,INTVECT_IRQ); break;
  case INTTYPE_FIRQ:
    CC6Fflag = CCREG_F_FLAG;   // prevent F ints
    CC4Iflag = CCREG_I_FLAG;   // prevent I ints 
    PCreg = RdWordFnct(L,INTVECT_FIRQ); break;
  case INTTYPE_SWI2:
    PCreg = RdWordFnct(L,INTVECT_SW2); break;
  case INTTYPE_SWI3:
    PCreg = RdWordFnct(L,INTVECT_SWI3); break;
  case INTTYPE_RESV:
    PCreg = RdWordFnct(L,INTVECT_RESV); break;
  }

}

void TFRfnct(lua_State *L,UWORD sxdx)
{
  UWORD sx = sxdx >> 4;
  UWORD dx = sxdx & 0x0F;
  ULONG val;

  // check for valid pair
  if ((sx ^ dx) & 0x08){
    char errbuff[32];
    sprintf(errbuff,"TFR invalid pair PC: 0x%.4X",(UWORD)PCtmp);
    lua_pushstring(L,errbuff); lua_error(L);
  }

  // transfer the data
  val = GetRx(sx); 
  PutRx(val,dx);
}

void EXGfnct(lua_State *L,UWORD sxdx)
{
  UWORD sx = sxdx >> 4;
  UWORD dx = sxdx & 0x0F;
  ULONG val1,val2;

  // check for valid pair
  if ((sx ^ dx) & 0x08){
    char errbuff[32];
    sprintf(errbuff,"EXG invalid pair PC: 0x%.4X",(UWORD)PCtmp);
    lua_pushstring(L,errbuff); lua_error(L);
  }

  // exchange the data
  val1 = GetRx(sx);
  val2 = GetRx(dx);
  PutRx(val1,dx);
  PutRx(val2,sx);
}

ULONG GetRx(UWORD sx)
{
  ULONG val;
  switch(sx){
  case 0x0: val = GetDreg(); break;
  case 0x1: val = Xreg;      break;
  case 0x2: val = Yreg;      break;
  case 0x3: val = Ureg;      break;
  case 0x4: val = Sreg;      break;
  case 0x5: val = PCreg;     break;
  case 0x8: val = Areg;      break;
  case 0x9: val = Breg;      break;
  case 0xA: val = GetCreg(); break;
  case 0xB: val = DPreg>>8;  break;
  }
  return val;
}

void PutRx(ULONG val, UWORD dx)
{
  switch(dx){
  case 0x0: PutDreg(val);       break;
  case 0x1: Xreg = val;         break;
  case 0x2: Yreg = val;         break;
  case 0x3: Ureg = val;         break;
  case 0x4: Sreg = val; NMI_ARMED_FLAG=1; break;
  case 0x5: PCreg = val;        break;
  case 0x8: Areg = (UWORD)val;  break;
  case 0x9: Breg = (UWORD)val;  break;
  case 0xA: PutCreg(val);       break;
  case 0xB: DPreg = (UWORD)val<<8; break;
  }
}

UWORD FxWordFnct(lua_State *L)
{
  UWORD aval,bval;
  aval =  RdByteFnct(L,PCreg); IncPCreg(1);
  bval =  RdByteFnct(L,PCreg); IncPCreg(1);
  return (aval<<8)+bval;
}

UWORD DxWordFnct(UWORD *paddx, UWORD *perr)
{
  UWORD aval,bval;
  UWORD addx = *paddx;
  
  aval = MemsReadByte(addx,perr);
  if (*perr) return aval;
  bval = MemsReadByte(addx+1,perr); 
  if (*perr) return bval;
  
  *paddx = (addx+2) & 0xFFFF;
  return (aval<<8)+bval;
}

UWORD GetCreg(void)
{
  UWORD cval;
  cval = CC7Eflag + CC6Fflag + CC5Hflag + CC4Iflag +
         CC3Nflag + CC2Zflag + CC1Vflag + CC0Cflag;
  return cval;
}

void PutCreg(UWORD cval)
{
  CC7Eflag = cval & CCREG_E_FLAG;
  CC6Fflag = cval & CCREG_F_FLAG;
  CC5Hflag = cval & CCREG_H_FLAG;
  CC4Iflag = cval & CCREG_I_FLAG;
  CC3Nflag = cval & CCREG_N_FLAG;
  CC2Zflag = cval & CCREG_Z_FLAG;
  CC1Vflag = cval & CCREG_V_FLAG;
  CC0Cflag = cval & CCREG_C_FLAG;
}

UWORD GetDreg(void)
{
  UWORD Dreg;
  Dreg = CAT(Areg,Breg);
  return Dreg;
}

void PutDreg(UWORD bval)
{
  //printf("PutDreg: 0x%.2X\n",bval);
  Areg = bval >> 8;
  Breg = bval & 0xFF;
}



void FlagBNZ0(UWORD bval)
{
  CC3Nflag = (bval & 0x80)? CCREG_N_FLAG : 0x0;
  CC2Zflag = (bval)?        0x0 : CCREG_Z_FLAG;
  CC1Vflag = 0x0;
}
void Flag2NZ0(UWORD aval, UWORD bval)
{
  CC3Nflag = (aval & 0x80)?   CCREG_N_FLAG : 0x0;
  CC2Zflag = (aval || bval)?  0x0 : CCREG_Z_FLAG;
  CC1Vflag = 0x0;
}
void FlagWNZ0(UWORD bval)
{
  CC3Nflag = (bval & 0x8000)? CCREG_N_FLAG : 0x0;
  CC2Zflag = (bval)?          0x0 : CCREG_Z_FLAG;
  CC1Vflag = 0x0;
}
void FlagBNZ01(UWORD bval)
{
  CC3Nflag = (bval & 0x80)? CCREG_N_FLAG : 0x0;
  CC2Zflag = (bval)?        0x0 : CCREG_Z_FLAG;
  CC1Vflag = 0x0;
  CC0Cflag = CCREG_C_FLAG;
}
void FlagClr(void)
{
  CC3Nflag = 0x0;
  CC2Zflag = CCREG_Z_FLAG;
  CC1Vflag = 0x0;
  CC0Cflag = 0x0;
}


/****************************************************************
 * CPU_ReportRegs
 * Report the contents of CPU registers
 ***************************************************************/
int CPU_ReportRegs_c(lua_State *L)
{
  char buff[12];
  printf("PC:%.4X A:%s ",(UWORD)PCreg,MemsByteDisp(Areg,buff));
  printf("B:%s ", MemsByteDisp(Breg,buff));

  AssignFlagChar(CC7Eflag,buff,  'E');
  AssignFlagChar(CC6Fflag,buff+1,'F');
  AssignFlagChar(CC5Hflag,buff+2,'H');
  AssignFlagChar(CC4Iflag,buff+3,'I');
  AssignFlagChar(CC3Nflag,buff+4,'N');
  AssignFlagChar(CC2Zflag,buff+5,'Z');
  AssignFlagChar(CC1Vflag,buff+6,'V');
  AssignFlagChar(CC0Cflag,buff+7,'C');
  buff[8] = '\0';
  printf("CC:%s ",buff);

  printf("DP:%s ",MemsByteDisp((UWORD)(DPreg>>8),buff));
  printf("X:%s ", MemsWordDisp(Xreg,buff));
  printf("Y:%s ", MemsWordDisp(Yreg,buff));
  printf("S:%s ", MemsWordDisp(Sreg,buff));
  printf("U:%s ", MemsWordDisp(Ureg,buff));
  printf("\n");
  
  lua_settop(L,0);
  lua_checkstack (L,0);
  return 0;
}

void AssignFlagChar(UWORD flag, char *pntr, char AssertFlag)
{
  if (!flag) *pntr = '.'; else {
    UWORD code = BYTE_FLAGS_MASK & flag;
    switch(code){
    case BYTE_FLAGNORM: *pntr = AssertFlag; break;
    case BYTE_NOT_INIT: *pntr = '-'; break;
    case BYTE_NO_VALUE: *pntr = '~'; break;
    case BYTE_UNKNOWN:  *pntr = '?'; break;
    default:            *pntr = 'X'; break;
    }
  }
}
/* end of CPU6809.c */
