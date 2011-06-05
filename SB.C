/******************************************************************************
*File:			sb.c
*Copyright: 1995 DiamondWare, Ltd.	All rights reserved.
*Written: 	Erik Lorenzen & Keith Weiner
*Purpose: 	Provides a basic API to the SB DSP
******************************************************************************/



#include <stdio.h>
#include <conio.h>
#include <stdlib.h>

#include "sound.h"
#include "sb.h"



void sb_Delay(WORD baseport, BYTE delayinusec)
{
	delayinusec *= 2; 		//reads the DSP staus port hopefully over an 8MHz
												//ISA-bus in order to induce a "timed" Delay.
	while (delayinusec--) //approx. 2 port reads to 1 æsec delay
	{
		inp (baseport + sb_WRITE_STATUS);
	}
}


void sb_WriteDSP(WORD baseport, BYTE value)
{
	while (0x80 & inp(baseport + sb_WRITE_STATUS))
	{
		//wait for DSP to be ready to accept command/data
		//by waiting for the MSB to be clear
	}

	outp(baseport + sb_WRITE_COMMAND, (int)value);
}


/* Should be turned on before DMA becomes active */
void sb_DacSpkrOn(WORD baseport)
{
	sb_WriteDSP(baseport, sb_DACSPKRON);	//turns on the DAC spkr so we can
																				//hear DSP (not needed on SB16 & AWE32)
	sb_Delay(baseport, 112);							//must delay at least 112 æsec
}


/* Should be turned Off before DMA becomes active */
void sb_DacSpkrOff(WORD baseport)
{
	sb_WriteDSP(baseport, sb_DACSPKROFF); //turns off DAC spkr
																				//(not needed on SB16 & AWE32)
	sb_Delay(baseport, 220);							//must delay at least 220 æsec
}


/*
 .	The SB does not take a rate directly; calculate a "time constant".
 .
 .		TC = 256 - (1,000,000 / (# channels * sampling rate))
*/
void sb_Speed(WORD baseport, WORD rate, BYTE numchan)
{
	rate = (WORD)(256 - (1000000 / (numchan * rate)));

	sb_WriteDSP(baseport, sb_SETTIMECONST);
	sb_WriteDSP(baseport, (BYTE)rate);
}


void sb_Play(WORD baseport, WORD buffsize)
{
	sb_WriteDSP(baseport, sb_SETBLOCKTRANSIZE);
	sb_WriteDSP(baseport, (BYTE)(buffsize - 1));				//low byte
	sb_WriteDSP(baseport, (BYTE)((buffsize - 1) >> 8)); //high byte
	sb_WriteDSP(baseport, sb_PLAY8BITMONO);
}


INT8 sb_Reset(WORD baseport)
{
	WORD x;
	int result;

	outp(baseport + sb_RESET, 1); 				//write a 1 to the reset port

	inp(baseport + sb_RESET); 						//wait at least 3 æsec's
	inp(baseport + sb_RESET); 						//can't use sb_Delay, we're still
	inp(baseport + sb_RESET); 						//not sure a DSP is present
	inp(baseport + sb_RESET);
	inp(baseport + sb_RESET);
	inp(baseport + sb_RESET);

	outp(baseport + sb_RESET, 0); 				//write a 0 to the reset port

	for (x=1;x;x++)
	{
		result = inp (baseport + sb_READ_STATUS);

		if (result & 0x80)									//if msb is set, data is ready
		{
			result = inp(baseport + sb_READ_DATA);

			if (result == 0xaa) 							//0xaa is the magic value the DSP
			{
				return (1); 										//returns after a successfull reset
			}
		}
	}

	return (0); 													//failed to find & reset a SB DSP
}
