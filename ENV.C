/******************************************************************************
*File:			env.c
*Copyright: 1995 DiamondWare, Ltd.	All rights reserved.
*Written: 	Erik Lorenzen & Keith Weiner
*Purpose: 	Contains code get the SB setting from the BLASTER variable
******************************************************************************/



#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "sound.h"
#include "env.h"



INT8 env_GetBlaster(env_BLASTER *blaster)
{
	char *temp;
	char *blstr;

	temp = getenv("BLASTER"); 		//attempt to read environment variable

	if (temp == NULL) 						//if temp == NULL the BLASTER env is not set
	{
		return (0); 								//fail!
	}

	blstr = strdup(temp); 				//dup the string so we don't trash the env

	temp = strtok(blstr," \t"); 	//parse the BLASTER variable, find fist token

	while(temp)
	{
		switch(toupper(temp[0]))
		{
			case 'A':
				blaster->ioaddr = (WORD)strtol(temp + 1, NULL, 16);
				break;
			case 'I':
				blaster->irqlev = (BYTE)atoi(temp + 1);
				break;
			case 'D':
				blaster->dmachan = (BYTE)atoi(temp + 1);
				break;
			/* There are other cases but none we will use */
		}

		temp = strtok(NULL," \t");	//find next token
	}

	free(blstr);

	return (1); 									//success
}
