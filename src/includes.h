/************************************************************************
 * includes.h - Part of HwSim6809 project
 * This file contains all the common material for the C sources
 * Krista Hill - kmhill@hartford.edu - 01/15/2013 - version 0.1.0
 * Krista Hill - kmhill@hartford.edu - 12/31/2012 - File is created
 ************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <lua.h>
#include <lauxlib.h>  // note the odd spelling
#include <lualib.h>

#define BYTE_VAL_MASK        0x00FF
#define BYTE_FLAGS_MASK      0xFF00
#define BYTE_FLAGNORM        0x0000
#define BYTE_NOT_INIT        0x0100
#define BYTE_NO_VALUE        0x0200
#define BYTE_UNKNOWN         0x0400
#define BYTE_NO_REGION       0x0800
#define BYTE_STORE_RAM       0x1000
#define BYTE_STORE_ROM       0x2000
#define BYTE_STORE_DEV       0x4000

#define WORD_VAL_MASK   0x00FFFF
#define WORD_FLAGS_MASK 0xFF0000
#define WORD_FLAGNORM   0x000000
#define WORD_NOT_INIT   0x010000
#define WORD_NO_VALUE   0x020000
#define WORD_UNKNOWN    0x040000
#define WORD_NO_REGION  0x080000

#define PP_PC_BIT     0x80
#define PP_SU_BIT     0x40
#define PP_Y_BIT      0x20
#define PP_X_BIT      0x10
#define PP_DP_BIT     0x08
#define PP_B_BIT      0x04
#define PP_A_BIT      0x02
#define PP_CC_BIT     0x01

#define CCREG_E_FLAG    0x80
#define CCREG_F_FLAG    0x40
#define CCREG_H_FLAG    0x20
#define CCREG_I_FLAG    0x10
#define CCREG_N_FLAG    0x08
#define CCREG_Z_FLAG    0x04
#define CCREG_V_FLAG    0x02
#define CCREG_C_FLAG    0x01

#define INTVECT_RESET   0xFFFE
#define INTVECT_NMI     0xFFFC
#define INTVECT_SWI     0xFFFA
#define INTVECT_IRQ     0xFFF8
#define INTVECT_FIRQ    0xFFF6
#define INTVECT_SW2     0xFFF4
#define INTVECT_SWI3    0xFFF2
#define INTVECT_RESV    0xFFF0

#define INTTYPE_RESET   0x00
#define INTTYPE_NMI     0x01
#define INTTYPE_SWI     0x02
#define INTTYPE_IRQ     0x03
#define INTTYPE_FIRQ    0x04
#define INTTYPE_SWI2    0x05
#define INTTYPE_SWI3    0x06
#define INTTYPE_RESV    0x07

typedef unsigned short UWORD;
typedef unsigned long  ULONG;
typedef enum {MemRam, MemRom, MemDev} MEMTYPE;
typedef UWORD ((MEMS_DEV_FNCT)(UWORD offs, UWORD valx));
typedef UWORD ((MEMS_INIT_FNCT)(UWORD offs));


typedef struct
MEMS_REGION_STRUCT {
  int            IDno;
  MEMTYPE       MemRegionType;
  UWORD         first_addx;
  UWORD         last_addx;
  UWORD        *data;
  MEMS_DEV_FNCT *fnct;
  struct MEMS_REGION_STRUCT *next;
} MEMS_REGION;

typedef struct
MEMS_REGION_LIST_STRUCT {
  MEMS_REGION  *MemLinkList;
  MEMS_REGION **MemList;
  int            MemRegionCount;
  int            IDlast;
} MEMS_REGION_LIST;


// Registered functions from MemSys.c
int MemsInsertRAM_c(lua_State *L);
int MemsInsertROM_c(lua_State *L);
int MemsConfig_c(lua_State *L);
int MemsReadS19_c(lua_State *L);
int MemsReportLL_c(lua_State *L);
int MemsReportList_c(lua_State *L);
int MemsDumpRange_c(lua_State *L);

// Functions accessible from MemSys.c
void   MemsInit     (void);
UWORD  MemsStoreByte(UWORD addx, UWORD bval);
UWORD  MemsReadByte (UWORD addx, UWORD *perr);
char  *MemsByteDisp (UWORD val, char buff[]);
char  *MemsWordDisp (ULONG val, char buff[]);
char  *MemsFlagStr  (UWORD bval);

// Registered functions from CPU6809.c
int CPU_DisOpCode_c(lua_State *L);
int CPU_FetchEx_c(lua_State *L);
int CPU_GetPCreg_c(lua_State *L);
int CPU_ReportRegs_c(lua_State *L);
int CPU_Reset_c(lua_State *L);

#define BUFF_LENGTH_S19 100
int ReadS19      (lua_State *L, FILE *fp);

/* end of includes.h */
