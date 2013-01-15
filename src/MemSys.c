/************************************************************************
 * MemSys.c - Part of HwSim6809 project
 * This file is associated with modelling the HwSim6809 memory system
 * Krista Hill - kmhill@hartford.edu - 01/15/2013 - version 0.1.0
 * Krista Hill - kmhill@hartford.edu - 12/31/2012 - File is created
 * The following will build and test this code.  The gcc compilier and
 * lua 5.1 interpreter were used
 * $ gcc -c -D MEMSYS_MAIN MemSys.c -Wall -I "/usr/include/lua5.1/"
 * $ gcc -c S19FileRead.c -Wall -I "/usr/include/lua5.1/"
 * $ gcc -shared -o MemSys.so MemSys.o S19FileRead.o
 * $ lua MemSys01.lua
 ************************************************************************/
#include "includes.h"

// Globals go here
static MEMS_REGION_LIST mx;

// Prototypes go here
MEMS_REGION *MemsMakeRegion(lua_State *L,
  UWORD first_addx, UWORD region_size, MEMTYPE MemRegionType,
  UWORD fillval,    MEMS_DEV_FNCT *fnct, MEMS_INIT_FNCT *initfnct);
void MemsSortRegions(MEMS_REGION *LList, MEMS_REGION **List);

#ifdef MEMSYS_MAIN
// Configure library for use
int luaopen_MemSys(lua_State *L)
{
  MemsInit();
   
  lua_register(L,"MemsInsertRAM", MemsInsertRAM_c);
  lua_register(L,"MemsInsertROM", MemsInsertROM_c);
  lua_register(L,"MemsConfig",    MemsConfig_c);
  lua_register(L,"MemsReadS19",   MemsReadS19_c);
  lua_register(L,"MemsReportLL",  MemsReportLL_c);
  lua_register(L,"MemsReportList",MemsReportList_c);
  lua_register(L,"MemsDumpRange", MemsDumpRange_c);
   
  return 0;
}
#endif

void MemsInit(void)
{
  // Initialize memory regions to empty
  mx.MemLinkList    = (MEMS_REGION  *)NULL;
  mx.MemList        = (MEMS_REGION  **)NULL;
  mx.MemRegionCount = 0;
  mx.IDlast         = 0;
}

/*
 *  Insert a region representing RAM
 */
int MemsInsertRAM_c(lua_State *L)
{
  //int n = lua_gettop(L);
  //printf("MemsInsertRAM_c args: %d\n",n);

  if (!lua_isnumber(L, 1) || !lua_isnumber(L,2)){
    lua_pushstring(L, "MemsInsertRAM expects start and size ints");
    lua_error(L);
  }
  
  UWORD first_addx,region_size;
  first_addx  = 0xFFFF & lua_tointeger(L,1);
  region_size = 0xFFFF & lua_tointeger(L,2);
  
  MEMS_REGION *mr;
  mr = MemsMakeRegion(L,first_addx,region_size,MemRam,BYTE_NOT_INIT,NULL,NULL);
  mr->next = mx.MemLinkList;
  mx.MemLinkList = mr;
  mx.MemRegionCount++;
        
  return 0;
}

/* 
 * Insert a region representing ROM
 */
int MemsInsertROM_c(lua_State *L)
{
  //int n = lua_gettop(L);
  //printf("MemsInsertROM_c args: %d\n",n);
   
  if (!lua_isnumber(L, 1) || !lua_isnumber(L,2)){
    lua_pushstring(L, "MemsInsertROM expects start and size integers");
    lua_error(L);
  }
 
  UWORD first_addx,region_size;
  first_addx  = 0xFFFF & lua_tointeger(L,1);
  region_size = 0xFFFF & lua_tointeger(L,2);
 
  MEMS_REGION *mr;
  mr = MemsMakeRegion(L,first_addx,region_size,MemRom,BYTE_NO_VALUE,NULL,NULL);
  mr->next = mx.MemLinkList;
  mx.MemLinkList = mr;

  return 0;
}

// Insert a region representing a device

/*======================================================================
 * Call this after all the regions have been inserted
 *====================================================================*/
int MemsConfig_c(lua_State *L)
{
  int ix, NumBytes, nn = 0;
  MEMS_REGION *LList = mx.MemLinkList;
  MEMS_REGION **pList;

  //int n = lua_gettop(L);
  //printf("MemsConfig_c args: %d\n",n);

  // Count the number of regions
  while(LList){
    nn = nn + 1;
    LList = LList->next; }
  mx.MemRegionCount = nn;
   	
  // Attempt to allocate memory to sort list to
  NumBytes = nn * sizeof(*pList);
  //printf("memc: 0x%.4X bytes\n",NumBytes);
  //pList = (MEMS_REGION **)lua_newuserdata(L,NumBytes);
  pList = (MEMS_REGION **)malloc(NumBytes);
  if (pList == (MEMS_REGION **)NULL) {
    lua_pushstring(L, "Not able to allocate memory list");
    lua_error(L); }
  mx.MemList = pList; 
   	
  // Sort the memory regions into a new list
  MemsSortRegions(mx.MemLinkList, pList);

  // Check for overlaps between memory regions
  for (ix = 1; ix < mx.MemRegionCount; ix++){
    if (pList[ix-1]->last_addx >= pList[ix]->first_addx){
      char buff[64];
      sprintf(buff,"regions overlap: %d, %d",
              pList[ix-1]->IDno,pList[ix]->IDno);
      lua_pushstring(L, buff);
      lua_error(L);
    }
  }
  return 0;
}

/*
 * Read an S19 file
 */
int MemsReadS19_c(lua_State *L)
{
  int nr;
  //int n = lua_gettop(L);
  //printf("MemsReadS19_c args: %d\n",n);
  
  // Check that there is a file name
  if ( !lua_isstring(L, 1) ){
    lua_pushstring(L, "MemsReadS19 expects a filename string");
    lua_error(L);
  }
    
  // Get the file name and attempt to open the file
  const char *fname = lua_tostring(L,1);
  FILE *fp = fopen(fname,"r");
  if (fp == NULL){
    char buff[80];
    sprintf(buff,"MemsReadS19: Not able to open '%s' for reading",fname);
    lua_pushstring(L,buff);
    lua_error(L);
  }
   
  // Read the S19 file
  nr = ReadS19(L,fp);
  if (nr < 1) {
    lua_pushstring(L,"S19 file is empty");
    lua_error(L);
  }
   
  // All done, close the file
  fclose(fp);
  return 0;
}


// Make a new region
MEMS_REGION *MemsMakeRegion(lua_State *L,
  UWORD first_addx, UWORD region_size, MEMTYPE MemRegionType,
  UWORD fillval,    MEMS_DEV_FNCT *fnct, MEMS_INIT_FNCT *initfnct)
{
  UWORD offs, *mdata;
  MEMS_REGION *mreg;
  int          NumBytes;

  // Check if this is not a valid memory region
  if (MemRegionType != MemRam && MemRegionType != MemRom &&
      MemRegionType != MemDev){
      goto EXIT0;
  }

   // Make a region
  NumBytes = sizeof(*mreg);
  //printf("region1: 0x%.4X bytes\n",NumBytes);
  //mreg = lua_newuserdata(L,NumBytes);
  mreg = (MEMS_REGION *)malloc(NumBytes);
  if (mreg == (MEMS_REGION *)NULL){
     goto EXIT0;
  }

  // Size and make memory
  NumBytes = region_size*sizeof(*mdata);
  //printf("region2: 0x%.4X bytes\n",NumBytes);
  //mdata = (UWORD *)lua_newuserdata(L,NumBytes);
  mdata = (UWORD *)malloc(NumBytes);
  if (mdata == (UWORD *)NULL){
    goto EXIT1;
  }

  // Execute initilizer if needed
  if (initfnct != (MEMS_INIT_FNCT *)NULL) {
    for (offs = 0; offs < region_size; offs++){
      mdata[offs] = initfnct(offs);
    }
  }
  
  // Otherwise fill with given fill value
  else {
    for (offs = 0; offs < region_size; offs++){
      mdata[offs] =  fillval;
    }
  }

  // Fill in the fields
  mreg->IDno = ++mx.IDlast;
  mreg->MemRegionType = MemRegionType;
  mreg->first_addx = first_addx;
  mreg->last_addx  = 0xFFFF & ( first_addx + region_size - 1);
  mreg->data = mdata;
  mreg->fnct = fnct;
  return mreg;

  // Error handler stack
EXIT1:
EXIT0:
  return (MEMS_REGION *)NULL;
}


/*
 * Organize items from linked list into sorted list
 */
void MemsSortRegions(MEMS_REGION *LList, MEMS_REGION **pList)
{ 
  int ix,nn = 0;

  // traverse the items in the linked list
  while (LList) {
   	
    // Insert item into list
    ix = nn;
    while (ix > 0 && (LList->first_addx) < (pList[ix-1]->first_addx)) {
      pList[ix] = pList[ix-1];
      ix = ix - 1;
    }
    pList[ix] = LList;
      
    // Move to next item
    LList = LList->next;
    nn = nn + 1;
  }
}


// stand-in functions
// watch for special case codes
// fix handling of devices
// write to rom will alter its contents
// perhaps a second function that allows writes to ROM?
UWORD MemsStoreByte(UWORD addx, UWORD bval)
{  
  //printf("[0x%.4X] = 0x%.2X\n",addx,bval);
   
  int mid, lft = 0, rht = mx.MemRegionCount - 1;
  UWORD         rx;
  MEMS_REGION *rp;
   
  while (lft <= rht){
    mid = (lft+rht)/2;
    rp = mx.MemList[mid];
    if (addx < rp->first_addx){
      rht = mid-1;
    }
    else if (addx > rp->last_addx){
      lft = mid+1;
    }
    else break;
  }
   
  if (lft > rht) {
    rx = BYTE_NO_REGION;
    //printf("addx: 0x%.4X - not in any region\n",addx);
  }
      
  else {
  // ** Fix me for devices...
    switch(rp->MemRegionType){
    case MemRam: rx = BYTE_STORE_RAM;
    case MemRom: rx = BYTE_STORE_ROM;
    case MemDev: rx = BYTE_STORE_DEV;
    }
    UWORD offs = addx - rp->first_addx;
    (rp->data)[offs] = bval;
    //printf("[0x%.4X] = 0x%.2X - mid: %d, range 0x%.4X -- 0x%.4X\n",
    //       addx,(rp->data)[offs],mid,rp->first_addx, rp->last_addx);
  }
  return rx;
}

// stand-in functions
// handle device writes
UWORD MemsReadByte(UWORD addx, UWORD *perr)
{     
  int mid, lft = 0, rht = mx.MemRegionCount - 1;
  UWORD bval = 0x0200;
  UWORD offs;
  MEMS_REGION *rp;

  fflush(stdout);

  while (lft <= rht){
    mid = (lft+rht)/2;
    rp = mx.MemList[mid];
    if (addx < rp->first_addx){     // Guessed too high
      rht = mid-1;
    }
    else if (addx > rp->last_addx){ // Guessed too low
      lft = mid+1;
    }
    else break;                     // Found the region!
  }
  
  if (lft > rht) {                   // Was a region found?
   //printf("addx: 0x%.4X - not in any region\n",addx);
   bval = BYTE_NO_REGION; *perr = BYTE_NO_REGION; 
  }
  else {                             // A region was found
    offs = addx - rp->first_addx;
    //printf("region: %d offs: 0x%.4X data: 0x%.4X\n",
    //       mid,offs,(rp->data)[offs]);
    bval = (rp->data)[offs];
    if (bval < BYTE_NOT_INIT) {
      *perr = BYTE_FLAGNORM;
    }
    else {
      *perr = bval;
    }
  }

  fflush(stdout);
  return bval;
}


// Report the memory map from the linked list
int MemsReportLL_c(lua_State *L)
{
  UWORD first_addx, last_addx;
  char  *msg;
  MEMS_REGION *mr = mx.MemLinkList;
   
  printf("MemsReportLL:\n");
  while(mr){
    first_addx = mr->first_addx;
    last_addx  = mr->last_addx;
    switch(mr->MemRegionType){
    case MemRam:
       msg = "RAM"; break;
    case MemRom:
       msg = "ROM"; break;
    case MemDev:
       msg = "DEV"; break;
    default:
      msg = "Unx"; break;
    }
    printf("%s: 0x%.4X - 0x%.4X\n",msg,first_addx,last_addx);
    mr = mr->next;
  }
  return 0;
}

// Report the memory map from the sorted list
int MemsReportList_c(lua_State *L)
{
  int ix;
  UWORD first_addx, last_addx;
  char  *msg;
   
  if (mx.MemList == NULL){
    lua_pushstring(L, "Configure mems before use");
    lua_error(L);
  }
   
  printf("MemsReportList: Regions = %d\n",mx.MemRegionCount);   
  for (ix = 0; ix < mx.MemRegionCount; ix++){
    first_addx = mx.MemList[ix]->first_addx;
    last_addx  = mx.MemList[ix]->last_addx;
    switch(mx.MemList[ix]->MemRegionType){
    case MemRam:
      msg = "RAM"; break;
    case MemRom:
      msg = "ROM"; break;
    case MemDev:
      msg = "DEV"; break;
    default:
      msg = "Unx"; break;
    }
    printf("%s: 0x%.4X - 0x%.4X\n",msg,first_addx,last_addx);
  }
  return 0;
}

int MemsDumpRange_c(lua_State *L)
{
  if (!lua_isnumber(L, 1) || !lua_isnumber(L,2)){
    lua_pushstring(L, "MemsDumpRange: needs first, last address");
    lua_error(L);
  }
   
  int first_addx,last_addx,size;
  int ix,cx,addx,iz;
  UWORD bval,err;
  static char  buff[16];

  first_addx  = 0xFFF0 & lua_tointeger(L,1);
  size        = 0xFFFF & lua_tointeger(L,2);
  last_addx   = 0xFFFF & (first_addx + size - 1);
  printf("MemsDumpRange: 0x%.4X -- 0x%.4X\n",first_addx,last_addx);

  for (ix = first_addx; ix < last_addx; ix += 0x10){
    addx = ix; printf("%.4X:",addx);
    for (cx = 0; cx < 4; cx++){
      for (iz = addx + 4; addx < iz; addx++) {
        bval = MemsReadByte(addx,&err);
        printf(" %s",MemsByteDisp(bval,buff));
      }
      if (cx < 3) printf(" .");
	 }
    printf("\n");
  }
  //lua_pushinteger(L,last_addx+1);
  return 0;
}

char *MemsByteDisp(UWORD val, char buff[])
{
  UWORD code = val & BYTE_FLAGS_MASK;
  
  switch(code) {
  case BYTE_FLAGNORM:
    sprintf(buff,"%.2X",val); break;
  case BYTE_NOT_INIT:           // noninit RAM
    sprintf(buff,"--"); break;  
  case BYTE_NO_VALUE:           // noninit ROM
    sprintf(buff,"~~"); break;
  case BYTE_UNKNOWN:            // unknown value assigned
    sprintf(buff,"??"); break;
  case BYTE_NO_REGION:          // no-such region
    sprintf(buff,"XX"); break;
  default:                      // unknown code
    sprintf(buff,"%.4X",code); break;
  }
  return buff;
}

char *MemsFlagStr(UWORD bval)
{
  char *str;
  bval &= BYTE_FLAGS_MASK;
  
  switch(bval){
  case BYTE_FLAGNORM:
    str = "ByteNormal"; break;
  case BYTE_NOT_INIT:
    str = "ByteNotInit"; break;
  case BYTE_NO_VALUE:
    str = "ByteNoValue"; break;
  case BYTE_UNKNOWN:
    str = "ByteUnknown"; break;
  case BYTE_NO_REGION:
    str = "ByteNoRegion"; break;
  default:
    str = "SysErr-UnknownCode"; break;
  }
  return str;
}


char *MemsWordDisp(ULONG val, char buff[])
{
  ULONG code = val & WORD_FLAGS_MASK;
  
  switch(code) {
  case WORD_FLAGNORM:
    sprintf(buff,"%.4X", (UWORD)val); break;
  case WORD_NOT_INIT:           // noninit RAM
    sprintf(buff,"----"); break;  
  case WORD_NO_VALUE:           // noninit ROM
    sprintf(buff,"~~~~"); break;
  case WORD_UNKNOWN:            // unknown value assigned
    sprintf(buff,"????"); break;
  case BYTE_NO_REGION:          // no-such region
    sprintf(buff,"XXXX"); break;
  default:
    sprintf(buff,"*%.4X*", (UWORD)val); break;
  }
  return buff;
}

/* end of MemSys.c */
