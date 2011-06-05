int getvmode( void );
#pragma aux getvmode =\
    "mov    ah,0x0F",\
    "int    0x10,"\
    "and	eax,0xFF",\

int setvmode(int);
#pragma aux setvmode =\
        "int 0x10",\
        parm [eax]\

