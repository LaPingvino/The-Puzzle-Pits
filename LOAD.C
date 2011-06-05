/******************************************************************************
*File:                  load.c
*Copyright: 1995 DiamondWare, Ltd.      All rights reserved.
*Written:       Erik Lorenzen & Keith Weiner
*Purpose:       Contains code load waves and convert .wav into a .raw
******************************************************************************/



#include <sys\types.h>
#include <sys\stat.h>
#include <stdio.h>
#include <dos.h>
#include <conio.h>
#include <stdlib.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "sound.h"
#include "env.h"



#define LENIDS  4
#define LENLENS 4



typedef struct                                          //these structs hold the header struct of a .WAV
{
	char    riffid[4];                              //"RIFF"
	DWORD rifflen;
	char    waveid[4];                              //"WAVE"
	char    fmtid[4];                               //"fmt " note the space after fmt
	DWORD fmtlen;                                   //number of bytes till DATAHDR starts

} HDR;


typedef struct
{
	char             dataid[4];             //"data"
	DWORD    datalen;

} DATAHDR;


typedef struct
{
	HDR              hdr;
	DATAHDR  datahdr;

} WAV;



int load_Wave(char *filename, SOUND *sound)
{
	WAV      wav;
	FILE    *fp = NULL;
	DWORD  x;

	if ((fp = fopen(filename, "rb")) == NULL)
	{
		//printf("\nUnable to open file %s", filename);
		goto ABORTLOAD;
	}

	fread(&wav.hdr, sizeof(HDR), 1, fp);

	if ((wav.hdr.waveid[0] != 'W') || (wav.hdr.waveid[1] != 'A') ||
			(wav.hdr.waveid[2] != 'V') || (wav.hdr.waveid[3] != 'E'))
	{
		//printf("%s is not a wave file", filename);
		goto ABORTLOAD;
  }

	fseek(fp, wav.hdr.fmtlen, SEEK_CUR);                                    //seek to begining of data
	fread(&wav.datahdr, sizeof(DATAHDR), 1, fp);

	if ((sound->samples = (INT8 *)halloc(wav.datahdr.datalen, 1)) == NULL)
	{
		//printf("Not enough memory to load File %s \n", filename);
		goto ABORTLOAD;
  }

	sound->length = wav.datahdr.datalen;
	fread(sound->samples, (size_t)sound->length, 1, fp);            //read the data

	for (x=0;x<sound->length;x++)                                                           //convert to signed format
	{
		sound->samples[x] += (INT8)0x80;
  }

	fclose(fp);
	return (1);


	/* -----------------------------------------------------------------------*/
	ABORTLOAD:

	if (fp != NULL)
	{
		fclose(fp);
	}

	return (0);
}
