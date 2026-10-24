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

// Debug flag for file operations - shared between functions
static int debug_file_open = 0;

void cleanExit(WORD retval);

// Case-insensitive file opening for DOS compatibility
FILE *fopen_case_insensitive(char *fname, const char *mode)
{
  FILE *file;
  char temp_fname[256];
  char *path_part, *filename_part, *ext_part;
  char path_backup[256];
  int i, len;
  
  if (debug_file_open) {
  }
  
  // Ensure we don't overflow the buffer
  len = strlen(fname);
  if (len >= 255) {
    if (debug_file_open) {
    }
    return NULL;
  }
  
  // First try the filename as-is
  if (debug_file_open) {
  }
  file = fopen(fname, mode);
  if (file != NULL) {
    if (debug_file_open) {
    }
    return file;
  }
  
  // Make a backup of the original path for manipulation
  strcpy(path_backup, fname);
  
  // Try all lowercase
  strcpy(temp_fname, fname);
  for (i = 0; temp_fname[i]; i++) {
    if (temp_fname[i] >= 'A' && temp_fname[i] <= 'Z') {
      temp_fname[i] = temp_fname[i] + 32; // Convert to lowercase
    }
  }
  if (debug_file_open) {
  }
  file = fopen(temp_fname, mode);
  if (file != NULL) {
    if (debug_file_open) {
    }
    return file;
  }
  
  // Try all uppercase
  strcpy(temp_fname, fname);
  for (i = 0; temp_fname[i]; i++) {
    if (temp_fname[i] >= 'a' && temp_fname[i] <= 'z') {
      temp_fname[i] = temp_fname[i] - 32; // Convert to uppercase
    }
  }
  if (debug_file_open) {
  }
  file = fopen(temp_fname, mode);
  if (file != NULL) {
    if (debug_file_open) {
    }
    return file;
  }
  
  // Find the last directory separator to split path and filename
  path_part = strrchr(path_backup, '/');
  if (!path_part) {
    path_part = strrchr(path_backup, '\\');
  }
  
  if (path_part) {
    // We have a path component
    *path_part = '\0'; // Temporarily terminate the path
    filename_part = path_part + 1;
    
    // Try different case variations of just the filename part
    // while keeping the path unchanged
    
    // Try lowercase filename with original path
    strcpy(temp_fname, path_backup);
    strcat(temp_fname, "/");
    len = strlen(temp_fname);
    strcpy(&temp_fname[len], filename_part);
    for (i = len; temp_fname[i]; i++) {
      if (temp_fname[i] >= 'A' && temp_fname[i] <= 'Z') {
        temp_fname[i] = temp_fname[i] + 32;
      }
    }
    file = fopen(temp_fname, mode);
    if (file != NULL) {
      return file;
    }
    
    // Try uppercase filename with original path
    strcpy(temp_fname, path_backup);
    strcat(temp_fname, "/");
    len = strlen(temp_fname);
    strcpy(&temp_fname[len], filename_part);
    for (i = len; temp_fname[i]; i++) {
      if (temp_fname[i] >= 'a' && temp_fname[i] <= 'z') {
        temp_fname[i] = temp_fname[i] - 32;
      }
    }
    file = fopen(temp_fname, mode);
    if (file != NULL) {
      return file;
    }
    
    // Try title case (first letter uppercase, rest lowercase)
    strcpy(temp_fname, path_backup);
    strcat(temp_fname, "/");
    len = strlen(temp_fname);
    strcpy(&temp_fname[len], filename_part);
    for (i = len; temp_fname[i]; i++) {
      if (i == len && temp_fname[i] >= 'a' && temp_fname[i] <= 'z') {
        temp_fname[i] = temp_fname[i] - 32; // First char uppercase
      } else if (temp_fname[i] >= 'A' && temp_fname[i] <= 'Z') {
        temp_fname[i] = temp_fname[i] + 32; // Rest lowercase
      }
    }
    file = fopen(temp_fname, mode);
    if (file != NULL) {
      return file;
    }
    
    // Find extension and try different cases for base name and extension
    ext_part = strrchr(filename_part, '.');
    if (ext_part) {
      // Try lowercase base, uppercase extension
      strcpy(temp_fname, path_backup);
      strcat(temp_fname, "/");
      len = strlen(temp_fname);
      
      // Copy and convert base name to lowercase
      for (i = 0; filename_part + i < ext_part; i++) {
        if (filename_part[i] >= 'A' && filename_part[i] <= 'Z') {
          temp_fname[len + i] = filename_part[i] + 32;
        } else {
          temp_fname[len + i] = filename_part[i];
        }
      }
      temp_fname[len + i] = '\0';
      
      // Append dot and convert extension to uppercase
      strcat(temp_fname, ".");
      len = strlen(temp_fname);
      for (i = 1; ext_part[i]; i++) { // Skip the dot
        if (ext_part[i] >= 'a' && ext_part[i] <= 'z') {
          temp_fname[len + i - 1] = ext_part[i] - 32;
        } else {
          temp_fname[len + i - 1] = ext_part[i];
        }
      }
      temp_fname[len + i - 1] = '\0';
      
      file = fopen(temp_fname, mode);
      if (file != NULL) {
        return file;
      }
      
      // Try uppercase base, lowercase extension
      strcpy(temp_fname, path_backup);
      strcat(temp_fname, "/");
      len = strlen(temp_fname);
      
      // Copy and convert base name to uppercase
      for (i = 0; filename_part + i < ext_part; i++) {
        if (filename_part[i] >= 'a' && filename_part[i] <= 'z') {
          temp_fname[len + i] = filename_part[i] - 32;
        } else {
          temp_fname[len + i] = filename_part[i];
        }
      }
      temp_fname[len + i] = '\0';
      
      // Append dot and convert extension to lowercase
      strcat(temp_fname, ".");
      len = strlen(temp_fname);
      for (i = 1; ext_part[i]; i++) { // Skip the dot
        if (ext_part[i] >= 'A' && ext_part[i] <= 'Z') {
          temp_fname[len + i - 1] = ext_part[i] + 32;
        } else {
          temp_fname[len + i - 1] = ext_part[i];
        }
      }
      temp_fname[len + i - 1] = '\0';
      
      file = fopen(temp_fname, mode);
      if (file != NULL) {
        return file;
      }
    }
  } else {
    // No path component, just filename - try additional variations
    
    // Try title case (first letter uppercase, rest lowercase)
    strcpy(temp_fname, fname);
    for (i = 0; temp_fname[i]; i++) {
      if (i == 0 && temp_fname[i] >= 'a' && temp_fname[i] <= 'z') {
        temp_fname[i] = temp_fname[i] - 32; // First char uppercase
      } else if (temp_fname[i] >= 'A' && temp_fname[i] <= 'Z') {
        temp_fname[i] = temp_fname[i] + 32; // Rest lowercase
      }
    }
    file = fopen(temp_fname, mode);
    if (file != NULL) {
      return file;
    }
    
    // Find extension and try different cases for base name and extension
    ext_part = strrchr(fname, '.');
    if (ext_part) {
      // Try lowercase base, uppercase extension
      strcpy(temp_fname, fname);
      
      // Convert base name to lowercase
      char *temp_ext = strrchr(temp_fname, '.');
      int base_len = temp_ext ? (temp_ext - temp_fname) : strlen(temp_fname);
      for (i = 0; i < base_len; i++) {
        if (temp_fname[i] >= 'A' && temp_fname[i] <= 'Z') {
          temp_fname[i] = temp_fname[i] + 32;
        }
      }
      
      // Convert extension to uppercase
      temp_ext = strrchr(temp_fname, '.');
      if (temp_ext) {
        for (i = 1; temp_ext[i]; i++) { // Skip the dot
          if (temp_ext[i] >= 'a' && temp_ext[i] <= 'z') {
            temp_ext[i] = temp_ext[i] - 32;
          }
        }
      }
      
      file = fopen(temp_fname, mode);
      if (file != NULL) {
        return file;
      }
      
      // Try uppercase base, lowercase extension
      strcpy(temp_fname, fname);
      
      // Convert base name to uppercase
      temp_ext = strrchr(temp_fname, '.');
      base_len = temp_ext ? (temp_ext - temp_fname) : strlen(temp_fname);
      for (i = 0; i < base_len; i++) {
        if (temp_fname[i] >= 'a' && temp_fname[i] <= 'z') {
          temp_fname[i] = temp_fname[i] - 32;
        }
      }
      
      // Convert extension to lowercase
      temp_ext = strrchr(temp_fname, '.');
      if (temp_ext) {
        for (i = 1; temp_ext[i]; i++) { // Skip the dot
          if (temp_ext[i] >= 'A' && temp_ext[i] <= 'Z') {
            temp_ext[i] = temp_ext[i] + 32;
          }
        }
      }
      
      file = fopen(temp_fname, mode);
      if (file != NULL) {
        return file;
      }
    }
  }
  
  // If none of the variants work, return NULL
  if (debug_file_open) {
  }
  return NULL;
}

void set_file_debug(int enable)
{
  debug_file_open = enable;
}

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

void Error(const char *mesg)
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
  file=fopen_case_insensitive(fname,"rb");
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
  file=fopen_case_insensitive(fname,"r");
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

