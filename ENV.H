/******************************************************************************
*File:			env.h
*Copyright: 1995 DiamondWare, Ltd.	All rights reserved.
*Written: 	Erik Lorenzen & Keith Weiner
*Purpose: 	Contains code get the SB setting from the BLASTER variable
******************************************************************************/



typedef struct
{
	WORD ioaddr;
	BYTE irqlev;
	BYTE dmachan;

} env_BLASTER;



INT8 env_GetBlaster(env_BLASTER *blaster);
