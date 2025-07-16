// file.c
// various file-handling routines
// By Abe Pralle 10.25.95

#ifndef _file_c_
#define _file_c_ 1

#include "FILE.H"
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

void Pause(LONG n);  //defined in gfx.c

extern char *shapedata;

union wordbytes{
  WORD word;
  BYTE byte[2];
};

char cryptkey[]={'M'-4,'i'-8,'n'-12,'d'-16,'y'-20,0};

void cleanExit(WORD retval);

LONG Encrypt(char *data,char *key)
{
  LONG i=0,keypos=0;

  while(data[i]!=0){
    data[i++]^=key[keypos++];
    if(key[keypos]==0) keypos=0;
  }
  return i;
}

void Decrypt(char *data,char *key,LONG slen)
{
  LONG i=0,keypos=0;
  while(slen>0){
    data[i++]^=key[keypos++];
    if(key[keypos]==0) keypos=0;
    slen--;
  }
}

void Error(char *mesg)
{
  fprintf(stderr,"%s",mesg);
  Pause(240);
  cleanExit(0);
  exit(0);
}

FILE *ReadFile(char *fname)
{
  FILE *file;
  char mesg[120];
  file=fopen(fname,"rb");
  if(file==0){
    sprintf(mesg,"Could not open \"%s\" for reading",fname);
    Error(mesg);
  }
  return file;
}

ULONG FileSize(FILE *fp)
{
  long pos, endpos;
  pos = ftell(fp);
  fseek(fp, 0, SEEK_END);
  endpos = ftell(fp);
  fseek(fp, pos, SEEK_SET);
  return ((ULONG) endpos);
}

WORD Exists(char *fname)
{
  FILE *file;
  file=fopen(fname,"r");
  if(file==0){
    return 0;
  }else{
    fclose(file);
    return 1;
  }
}

void CloseFile(FILE *fp)
{
  fclose(fp);
}

void ReadMem(FILE *fp, char *addr, ULONG size)
{
  static char buffer[4096];

  while(size>=4096){
    fread(buffer,4096,1,fp);
    _fmemcpy(addr,buffer,4096);
    size-=4096;
    addr+=4096;
  }
  fread(buffer,size,1,fp);
  _fmemcpy(addr,buffer,size);
  addr+=size;
}

UBYTE ReadUByte(FILE *fp)
{
  UBYTE data;
  fread(&data,1,1,fp);
  return data;
}

WORD ReadWord(FILE *fp)
{
  union wordbytes data;
  fscanf(fp,"%c%c",&data.byte[1],&data.byte[0]);
  return data.word;
}

void ReadString(FILE *fp,char *str)
{
  WORD size;
  size=ReadWord(fp)^22;
  ReadMem(fp,str,size);
  Decrypt(str,cryptkey,size);
  str[size]=0;
}

#endif

