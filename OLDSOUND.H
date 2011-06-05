/*
** sound.h
** 11.6.95 by Abe Pralle
*/

#include "file.h"

#ifndef DWORD
	#define DWORD unsigned long
#endif

#ifndef INT8
	#define INT8 signed char
#endif

#ifndef INT16
	#define INT16 signed short
#endif

#ifndef INT32
	#define INT32 signed long
#endif


typedef struct
{
	INT8    far *samples;
	DWORD  length;

} SOUND;

typedef struct
{
	WORD ioaddr;
	BYTE irqlev;
	BYTE dmachan;

} env_BLASTER;

INT8 env_GetBlaster(env_BLASTER *blaster);


INT8 dig_Init(env_BLASTER *blaster);
void dig_Kill(void);
void dig_Play(SOUND *sound);

int  load_Wave(char *filename, SOUND *sound);

#define sb_RESET         0x6                            //port
#define sb_READ_DATA     0xa                            //port
#define sb_WRITE_COMMAND 0xc                            //port
#define sb_WRITE_STATUS  0xc                            //port
#define sb_READ_STATUS   0xe                            //port
#define sb_ACKIRQ        0xe                            //port (used in dig.c)

#define sb_DACSPKRON        0xd1            //DSP command
#define sb_DACSPKROFF       0xd3            //DSP command
#define sb_SETTIMECONST     0x40            //DSP command
#define sb_PLAY8BITMONO     0x1c            //DSP command (autoinit DMA)
#define sb_SETBLOCKTRANSIZE 0x48                //DSP command

INT8 sb_Reset(WORD baseport);
void sb_Delay(WORD baseport, BYTE delay);
void sb_DacSpkrOn(WORD baseport);
void sb_DacSpkrOff(WORD baseport);
void sb_Speed(WORD baseport, WORD rate, BYTE numchan);
void sb_Play(WORD baseport, WORD buffsize);
void sb_WriteDSP(WORD baseport, BYTE value);

void dma_ProgramChan(BYTE chan, INT8 far *sound, WORD numsamps);
