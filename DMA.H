/******************************************************************************
*File:                  dma.h
*Copyright: 1995 DiamondWare, Ltd.      All rights reserved.
*Written:       Erik Lorenzen & Keith Weiner
*Purpose:       Declares prototypes for DMA
******************************************************************************/



void dma_ProgramChan(BYTE chan, INT8 *sound, WORD numsamps);
void dma_ChanOff(BYTE chan);
