// file.h
// various file-handling routines
// By Abe Pralle 10.25.95

#ifndef abe_h

#include <stdio.h>
#include "compat.h"

typedef unsigned char BYTE;
typedef unsigned char UBYTE;
typedef int WORD;
typedef unsigned int UWORD;
typedef long LONG;
typedef unsigned long ULONG;
typedef char CHAR;
#ifndef INT8
	#define INT8 signed char
#endif
#define REGISTER register;

extern char cryptkey[];

void Error(char *mesg);

FILE *ReadFile(char *fname);
ULONG FileSize(FILE *fp);
WORD Exists(char *fname);
void CloseFile(FILE *fp);

void ReadMem(FILE *fp, char *addr, ULONG size);
WORD ReadWord(FILE *fp);
UBYTE ReadUByte(FILE *fp);
void ReadString(FILE *fp,char *str);

#endif


