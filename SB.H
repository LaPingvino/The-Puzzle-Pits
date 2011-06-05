/******************************************************************************
*File:			sb.h
*Copyright: 1995 DiamondWare, Ltd.	All rights reserved.
*Written: 	Erik Lorenzen & Keith Weiner
*Purpose: 	Declares prototypes & #defines for SB
******************************************************************************/



#define sb_RESET				 0x6				//port
#define sb_READ_DATA		 0xa				//port
#define sb_WRITE_COMMAND 0xc				//port
#define sb_WRITE_STATUS  0xc				//port
#define sb_READ_STATUS	 0xe				//port
#define sb_ACKIRQ 			 0xe				//port (used in dig.c)


#define sb_DACSPKRON				0xd1		//DSP command
#define sb_DACSPKROFF 			0xd3		//DSP command
#define sb_SETTIMECONST 		0x40		//DSP command
#define sb_PLAY8BITMONO 		0x1c		//DSP command (autoinit DMA)
#define sb_SETBLOCKTRANSIZE 0x48		//DSP command



INT8 sb_Reset(WORD baseport);


void sb_Delay(WORD baseport, BYTE delay);


void sb_DacSpkrOn(WORD baseport);

void sb_DacSpkrOff(WORD baseport);


void sb_Speed(WORD baseport, WORD rate, BYTE numchan);


void sb_Play(WORD baseport, WORD buffsize);


void sb_WriteDSP(WORD baseport, BYTE value);
