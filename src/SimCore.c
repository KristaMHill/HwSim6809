/************************************************************************
 * SimCore.c - Part of HwSim6809 project
 * This file is associated with modelling a 6809 hardware system from
 * the point of view of the CPU.  The peripheral devices have some hooks
 * into the code at this level.
 * Krista Hill - kmhill@hartford.edu - 01/15/2013 - version 0.1.0
 * Krista Hill - kmhill@hartford.edu - 12/31/2012 - File is created
 * The following will build and test this code.  The gcc compilier and
 * lua 5.1 interpreter were used
 * $ gcc -c MemSys.c -Wall -I "/usr/include/lua5.1/"
 * $ gcc -c S19FileRead.c -Wall -I "/usr/include/lua5.1/"
 * $ gcc -c SimCore.c -Wall -I "/usr/include/lua5.1/"
 * $ gcc -shared -o SimCore.so MemSys.o S19FileRead.o SimCore.o
 * $ lua test02.lua
 ************************************************************************/
#include "includes.h"

// Configure library for use
int luaopen_SimCore(lua_State *L)
{
   // Initialize memory regions to empty
   MemsInit();
   
   lua_register(L,"MemsInsertRAM", MemsInsertRAM_c);
   lua_register(L,"MemsInsertROM", MemsInsertROM_c);
   lua_register(L,"MemsConfig",    MemsConfig_c);
   lua_register(L,"MemsReadS19",   MemsReadS19_c);
   lua_register(L,"MemsReportLL",  MemsReportLL_c);
   lua_register(L,"MemsReportList",MemsReportList_c);
   lua_register(L,"MemsDumpRange", MemsDumpRange_c);
   lua_register(L,"CPU_DisOpCode", CPU_DisOpCode_c);
   lua_register(L,"CPU_FetchEx",   CPU_FetchEx_c);
   lua_register(L,"CPU_GetPCreg",  CPU_GetPCreg_c);
   lua_register(L,"CPU_ReportRegs",CPU_ReportRegs_c);
   lua_register(L,"CPU_Reset",     CPU_Reset_c);

   return 0;
}

