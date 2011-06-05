/******************************************************************************
*File:			dma.c
*Copyright: 1995 DiamondWare, Ltd.	All rights reserved.
*Written: 	Erik Lorenzen & Keith Weiner
*Purpose: 	Contains code to manage DMA controller
*Note:			This code requires an AT-Class machine or higher!
* 					This code will handle all DMA channels
******************************************************************************/



#include <conio.h>
#include <dos.h>

#include "sound.h"
#include "dma.h"



/* masks for use in controlling the DMA chip */
/*							 DMA CHANNEL		#0		#1		#2		#3		#4		#5		#6		#7 */
static BYTE dma_on[8] = 			{0x00, 0x01, 0x02, 0x03, 0x00, 0x01, 0x02, 0x03};
static BYTE dma_off[8] =			{0x04, 0x05, 0x06, 0x07, 0x04, 0x05, 0x06, 0x07};
static BYTE dma_mode[8] = 		{0x58, 0x59, 0x5a, 0x5b, 0x58, 0x59, 0x5a, 0x5b};

/* DMA controller ports */
static BYTE dma_page[8] = 		{0x87, 0x83, 0x81, 0x82, 0x8f, 0x8b, 0x89, 0x8a};
static BYTE dma_bnca[8] = 		{0x00, 0x02, 0x04, 0x06, 0xc0, 0xc4, 0xc8, 0xcc};
static BYTE dma_bncc[8] = 		{0x01, 0x03, 0x05, 0x07, 0xc2, 0xc6, 0xca, 0xce};
static BYTE dma_maskreg[8] =	{0x0a, 0x0a, 0x0a, 0x0a, 0xd4, 0xd4, 0xd4, 0xd4};
static BYTE dma_modereg[8] =	{0x0b, 0x0b, 0x0b, 0x0b, 0xd6, 0xd6, 0xd6, 0xd6};
static BYTE dma_flipflop[8] = {0x0c, 0x0c, 0x0c, 0x0c, 0xd8, 0xd8, 0xd8, 0xd8};



void dma_ChanOff(BYTE chan)
{
	outp(dma_maskreg[chan], dma_off[chan]); 		 //disable DMA channel
}


void dma_ProgramChan(BYTE chan, INT8 *sound, WORD numsamps)
{
	DWORD  padd;
	DWORD  page;
	DWORD  off;
	DWORD  seg;

	_disable(); 																 //disable interrupts

	outp(dma_maskreg[chan], dma_off[chan]); 		 //disable DMA channel
																							 //before programming
	outp(dma_flipflop[chan], 0x00); 						 //reset flip-flop

	outp(dma_modereg[chan], dma_mode[chan]);		 //set DMA mode

	off = (*((WORD *)&(sound)));
	seg = (*((WORD *)&(sound) + 1));
	seg <<= 4;

	padd = seg + off; 													 //calc physical address
	page = padd >> 16;													 //calc page number

	outp(dma_bnca[chan], (BYTE)(padd));
	outp(dma_bnca[chan], (BYTE)(padd >> 8));
	outp(dma_page[chan], (BYTE)page);

	numsamps--; 																 //DMA cntrlr expects 1 less
	outp(dma_bncc[chan], (BYTE)(numsamps & 0x00ff)); //low byte
	outp(dma_bncc[chan], (BYTE)(numsamps >> 8));		 //high byte

	outp(dma_maskreg[chan], dma_on[chan]);			 //enable DMA channel

	_enable();																	 //re-enable interrupts
}
