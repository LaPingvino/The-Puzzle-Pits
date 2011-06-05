/******************************************************************************
*File:                  dig.c
*Copyright: 1995 DiamondWare, Ltd.      All rights reserved.
*Written:       Erik Lorenzen & Keith Weiner
*Purpose:       To control SB dig
*Notes:                 This code will only handle IRQ's 2, 3, 5, and 7 (1st PIC)
*****************************************************************************/



#pragma check_stack(off)



#include <stdio.h>
#include <conio.h>
#include <dos.h>

#include "sound.h"
#include "env.h"
#include "dig.h"
#include "sb.h"
#include "dma.h"



#define dig_EOI                                 0x20
#define dig_PICPORT             0x20
#define dig_IMRPORT             0x21

#define dig_NUMSOUNDS   0x10

#define dig_SAMPLERATE  11000                                                                   //reasonable number for a .WAV

#define dig_BUFFSIZE            256
#define dig_ALLOCSIZE   (dig_BUFFSIZE * 2 + 16) //Minimum size buffer
																								//needed to guarantee finding
																								//suitable DMA buffer

//INTERRUPT REQUEST NUMBER #0    #1      #2      #3      #4      #5      #6      #7
static BYTE intnum[8]  = {0x00, 0x00, 0x0a, 0x0b, 0x00, 0x0d, 0x00, 0x0f};
static BYTE onmask[8]  = {0x00, 0x00, 0xfb, 0xf7, 0x00, 0xdf, 0x00, 0x7f};


static INT8 unadjustedbuffer[dig_ALLOCSIZE];
static INT8 *buffer;

static void (_interrupt *oldisr)();
static BYTE old_imr;

static WORD sb_base;
static BYTE sb_irq;
static BYTE sb_dma;

static volatile INT8     numsnds;
static volatile INT8    *sndptr[dig_NUMSOUNDS];
static volatile DWORD  sndlen[dig_NUMSOUNDS];



static INT8 *BuffAdjust(INT8 *somebuffer)
{
	DWORD x, ptr, page1, page2, off, seg;

	for (x=0;x<=(dig_BUFFSIZE);x++)                         //check to make sure that we
	{                                                                                                                                               //have a buffer w/o a page break
		ptr = (DWORD)(somebuffer + x);

		off = (*((WORD *)&(ptr)));                                      //break the ptr up into segment
		seg = (*((WORD *)&(ptr)+1));                            //and offset

		printf("");                                                                                             //work-around for Watcom 10 bug!!!
																				//as per KW.
		seg <<= 4;

		page1 = seg + off;                                                                      //calculate physical address
		page1 >>= 16;                                                                                   //shift right by 16 to get page
																				//number of begining of buffer
		off += (dig_BUFFSIZE);
		page2 = seg + off;                                                                      //do it again ,but this time for
		page2 >>= 16;                                                                                   //the end of the buffer

		if (page1 == page2)                                                             //if the begining and end of the
		{                                                                                                                                       //buffer are in the same 64k
			return ((INT8 *)ptr);                                           //page we have our address
		}
	}

	//printf("\n\nFatal horrible error!!"); //we shouldn't get here
	//printf("\nUnable to find buffer without page break?");
	return(NULL);
}


static void IntOn(BYTE irqlevel)
{
	BYTE temp;

	temp = (BYTE)inp(dig_IMRPORT);                          //read the IMR (Interrupt Mask Reg)
	old_imr = temp;                                                                                         //save IMR for later.
	temp &= onmask[irqlevel];                                               //make sure to reset our bit
	outp(dig_IMRPORT, temp);                                                        //write the new IMR
}


static void IntOff(BYTE irqlevel)
{
	int temp;

	old_imr |= onmask[irqlevel];                                    //by ORing & XORing we leave only
	old_imr ^= onmask[irqlevel];                                    //the bit we have used untouched

	temp = inp(dig_IMRPORT);                                                        //read the IMR again

	temp |= old_imr;                                                                                        //make sure our channel is left in the
																				//state we found it in.
	outp(dig_IMRPORT, temp);                                                        //write the new IMR
}


static void _interrupt SBISR(void)
{
	WORD    smpnum;
	INT8    sound;
	INT8    x;
	INT16 temp;

	for (smpnum=0;smpnum<dig_BUFFSIZE;smpnum++)
	{
		temp = 0x80;                                                                                            //reset the accumulator

		for (sound=0;sound<numsnds;sound++)
		{
			temp += (INT16)*(sndptr[sound]++);//add a sample from all active voices
			sndlen[sound]--;                                                                        //decrease sound len by one sample

			if (!sndlen[sound])                                                     //if length is 0, the sound is done
			{
				for (x=sound;x<numsnds;x++)             //get rid of it
				{
					sndptr[x] = sndptr[x + 1];
					sndlen[x] = sndlen[x + 1];
				}

				numsnds--;
      }
		}

		if (temp < 0)                                                                                   //clip the accumulator if out of range
		{
			temp = 0;
		}
		else if (temp > 0xff)
		{
			temp = 0xff;
		}

		buffer[smpnum] = (INT8)temp;                            //write data to the buffer
  }

	inp(sb_base+sb_ACKIRQ);                                                         //clear the IRQ from the SB
	outp(dig_PICPORT, dig_EOI);                                     //send EOI to pic
}


static void Hook(BYTE irqlevel)
{
	/* Save the old vector and give our ISR control */
	oldisr = _dos_getvect(intnum[irqlevel]);
	_dos_setvect((WORD)intnum[irqlevel], SBISR);
}


static void UnHook(BYTE irqlevel)
{
	/* Restore control to the old ISR */
	_dos_setvect((WORD)intnum[irqlevel], oldisr);
}


void dig_Play(SOUND *sound)
{
	BYTE x;

	if (numsnds >= dig_NUMSOUNDS)
	{
		for (x=0;x<dig_NUMSOUNDS;x++)
		{
			sndptr[x] = sndptr[x + 1];
			sndlen[x] = sndlen[x + 1];
		}

		numsnds--;
  }

	sndptr[numsnds] = sound->samples;
	sndlen[numsnds] = sound->length;

	numsnds++;
}


INT8 dig_Init(env_BLASTER *blaster)
{
	WORD x;

	numsnds = 0;

	sb_dma  = blaster->dmachan;
	sb_base = blaster->ioaddr;       //setup globals right away
	sb_irq  = blaster->irqlev;       //some are needed in the ISR

	if ((buffer = BuffAdjust(unadjustedbuffer)) == NULL)
	{
		return (0);
	}

	for (x=0;x<dig_BUFFSIZE;x++)     //init buffer so we don't get an extra pop
	{                                                                                                                //on startup
		buffer[x] = (INT8)0x80;
	}

	if (!sb_Reset(sb_base))                          //reset SB, insure the DSP is in its default
	{                                                                                                                //state and make sure the DSP is really there.
		//printf("No Sound Blaster found.\n"); //init didn't find a DSP at the port
		return (0);                                                              //fail!
	}

	Hook(sb_irq);                                                            //hook our interrupt vector
	IntOn(sb_irq);                                                           //turn our IRQ on at PIC

	sb_DacSpkrOn(sb_base);                           //make sure we hear the sounds
	dma_ProgramChan(sb_dma, buffer, dig_BUFFSIZE);
	sb_Speed(sb_base, dig_SAMPLERATE, 1);
	sb_Play(sb_base, dig_BUFFSIZE);//send the DSP commands to start playing

	return (1);                                                                      //success!
}


void dig_Kill(void)
{
	sb_DacSpkrOff(sb_base);
	dma_ChanOff(sb_dma);                                     //disable our DMA channel
	IntOff(sb_irq);                                                          //mask (disable) our IRQ at PIC
	UnHook(sb_irq);                                                          //unhook our interrupt vector
	sb_Reset(sb_base);                                               //leave the SB in a known "good" state
}
