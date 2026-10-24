// file.h
// various file-handling routines
// By Abe Pralle 10.25.95

#ifndef abe_h

#include <stdio.h>
#include "compat.h"

typedef unsigned char BYTE;
typedef unsigned char UBYTE;
typedef int16_t WORD;   // int and long as the DOS compiler had them: 16 and 32 bits
typedef uint16_t UWORD;
typedef int32_t LONG;
typedef uint32_t ULONG;
typedef char CHAR;
#ifndef INT8
	#define INT8 signed char
#endif
#define REGISTER register;

extern char cryptkey[];

void Error(const char *mesg);

FILE *ReadFile(char *fname);
ULONG FileSize(FILE *fp);
WORD Exists(char *fname);
FILE *fopen_case_insensitive(char *fname, const char *mode);
void set_file_debug(int enable);
void CloseFile(FILE *fp);

void ReadMem(FILE *fp, char *addr, ULONG size);
WORD ReadWord(FILE *fp);
UBYTE ReadUByte(FILE *fp);
void ReadString(FILE *fp,char *str);

#endif


