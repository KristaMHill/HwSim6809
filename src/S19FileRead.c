/****************************************************************************
 * S19FileRead.c - Part of HwSim6809
 * A collection of things for reading the contents of an S19 file
 * Krista Hill - kmhill@hartford.edu - 01/15/2013 - version 0.1.0
 * Krista Hill - kmhill@hartford.edu - 12/30/2012 - File is created
 ***************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include "includes.h"

//Functions private to this file
int  CheckSumS19  (char *buff, char *bp, char **pErrMsg);
int  StoreBytesS19(char *buff, char *bp);
void ReportErrS19 (lua_State *L, int recno, char *msg);

#ifdef S19FILE_MAIN
/****************************************************************************
 * test main for S19FileRead.c
 * The following code allows this module to be compiled and executed by
 * itself, probably for testing and development purposes.
 * $ gcc -g -Wall S19FileRead.c -o S19TestRead -I "/usr/include/lua5.1/"
 *   -D S19FILE_MAIN
 ***************************************************************************/
int main(int argc, char *argv[])
{
  int nr;
  char *fname;
  
  // Pick a file name
  if (argc == 1) {
    fname = "ex02.s19"; }
  else {
    fname = argv[1];
  }

  // Attempt to open the file
  printf("Test for - S19FileRead - report contents of an S19 file\n");
  FILE *fp = fopen(fname,"r");
  if (fp == NULL){
    printf("Not able to open '%s' for reading\n",fname);
    return 0;
  }

  // Read and close the file, then report the number of records
  nr = ReadS19(NULL,fp);
  fclose(fp);
  printf("Record count = %d\n",nr);
  return 0;
}

// stand-in functions
UWORD MemsStoreByte(UWORD addx, UWORD bval)
{ printf("[0x%.4X] = 0x%.2X\n",addx,bval); return BYTE_STORE_ROM;}
void lua_pushstring(lua_State *L, const char *s)
{ printf("%s\n",s); }
int lua_error(lua_State *L)
{ exit(0); return 0; }
#endif


/****************************************************************************
 * ReadS19() - read an S19 file
 * The start of a record is marked with an 'S' and the next character
 * indicates the record type.  S1 is a cargo record and S9 is an end
 * of file record, though an S1 without cargo also marks the file end.
 ***************************************************************************/
int ReadS19(lua_State *L, FILE *fp)
{
  enum scanstate {stinit,stmark,stdata,stend};
  static char buff[BUFF_LENGTH_S19];
  char *bp, *bpx, *errmsg, tmp;
  int filedone,recno;
  enum scanstate state;
	
  filedone = 0; recno = 0; state = stinit;
  bp = buff; bpx = buff + BUFF_LENGTH_S19;
  while(!filedone) {
    tmp = fgetc(fp);

    // Watch for end of file
    if (tmp == EOF){
      if (state == stdata){
        if (CheckSumS19(buff,bp,&errmsg)){
          ReportErrS19(L,recno,errmsg);
        }
        else {
          StoreBytesS19(buff,bp);
        }
      }
      else if (state == stend){
        if (CheckSumS19(buff,bp,&errmsg)){
          ReportErrS19(L,recno,errmsg);
        }
      }
      filedone = 1; continue;
    }

    // Watch for start of record marker character
    else if (tmp == 's' || tmp == 'S'){
      if (state == stdata){
        if (CheckSumS19(buff,bp,&errmsg)){
          ReportErrS19(L,recno,errmsg);
        }
        else {
          StoreBytesS19(buff,bp);
        }
      }
      // Get ready for the next record
      recno++;
      bp = buff;          
      state = stmark;
    }

    // The record type character follows the record marker
    else if (state == stmark){
      if (tmp == '1')      { state = stdata; }
      else if (tmp == '9') { state = stend;  }
      else                 { state = stinit; }
      continue;
    }

    // Until the record type is known, ignore characters
    else if (state <= stmark){
      continue;
    }

    // Might be a hexadecimal digit
    else if (bp < bpx){
      if (tmp >= '0' && tmp <= '9'){
        *bp = 0xFF & (tmp - '0'); bp++; 
      }
      else if (tmp >= 'A' && tmp <= 'F'){
        *bp = 0xFF & (tmp - 55); bp++;
      }
      else if (tmp >= 'a' && tmp <= 'f'){
        *bp = 0xFF & (tmp - 87); bp++;
      }
    }

    // Record is too long for the buffer
    else {
      ReportErrS19(L,recno,"too long for S19 read buffer");
      state = stinit;
    }
  }
  return recno;
}

/**********************************************************************
 * CheckSumS19()
 * The record length is checked as well as the sum of the bytes.  
 * Add up the value pairs representing bytes.  The last byte is the
 * checksum, which was the one's complement of the lower 8-bits of all
 * the prior bytes.  As such, in adding all the bytes we expect the
 * lower 8-bits to equal $FF, and if so return 1 for success.
 **********************************************************************/
int CheckSumS19(char *buff, char *bp, char **pErrMsg)
{

  // Calculate the number of nibbles and check the record length
  int nb, nc;
  nb = ((*buff << 4) + *(buff+1) + 1)*2;
  nc = (int)(bp - buff);
  if (nb != nc) {
    *pErrMsg = "record length error"; return 1;
  }

  // Add up all the bytes and check the length
  char *bx = buff, *by = bp-1;
  int bsum = 0;  
  while (bx < by){
    bsum = (bsum + (*bx << 4) + *(bx+1)) & 0xFF;
    bx = bx + 2; 
  }

  // Check the resulting sum
  if (bsum == 0xFF) {
    *pErrMsg = ""; return 0; }
  else {
    *pErrMsg = "checksum error"; return 2;
  }
}

/**********************************************************************
 * StoreBytesS19
 * Move the contents of the bytes in the buffer to memory
 *********************************************************************/
int StoreBytesS19(char *buff, char *bp)
{
  int    ix, bval;
  UWORD  addx;
  char  *bx = buff+2, *by = bp-3;
  
  // Collect the record base address
  for (ix = 0, addx = 0; ix < 4; ix++){
    addx = ((addx << 4) + *(bx++)) & 0xFFFF;
  }

  #ifdef S19FILE_MAIN
  printf("addx = 0x%.4X\n",addx);
  #endif  

  while (bx < by){
    bval = (*bx << 4) + *(bx+1);
    bx = bx + 2;
    MemsStoreByte(addx,bval);
    addx++;
  }
  return 0;
}

/**********************************************************************
 * ReportErrS19
 * Central place to go for reporting S19 file errors
 *********************************************************************/
void ReportErrS19(lua_State *L, int recno, char *msg)
{
  char msgx[80];
  sprintf(msgx,"S19 recno: %d, %s",recno,msg);
  lua_pushstring(L,msgx);
  lua_error(L);
}
/* end of S19FileRead.c */
