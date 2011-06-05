#ifndef __DOSTIME_H
#define __DOSTIME_H

#pragma library(utils);

// Get the DOS tick count.  Returns 1/18ths since midnight
long cdecl GetDosTicks(void);

#endif


