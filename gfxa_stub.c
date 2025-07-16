/* gfxa_stub.c
 * Stub implementations for assembly functions
 * These are now handled by SDL compatibility layer
 */

#include "compat.h"

#ifdef __cplusplus
extern "C" {
#endif

/* These functions are implemented in sdl_compat.c
 * This file exists to satisfy any direct references
 * during the transition from assembly to C/SDL
 */

/* Video mode functions - implemented in sdl_compat.c */
/* void setmode(int mode) - in sdl_compat.c */
/* void cls(char *screen) - in sdl_compat.c */ 
/* void screenswap(char *screen) - in sdl_compat.c */
/* void setpalette(char *colorlist) - in sdl_compat.c */

/* Mouse functions - implemented in sdl_compat.c */
/* uint16_t initmouse(void) - in sdl_compat.c */
/* uint16_t readmbutton(void) - in sdl_compat.c */
/* void relpos(uint16_t *x, uint16_t *y) - in sdl_compat.c */

/* Keyboard functions - implemented in sdl_compat.c */
/* void setkb(void) - in sdl_compat.c */
/* void resetkb(void) - in sdl_compat.c */

/* Additional assembly stubs that might be needed */

/* Fast memory operations - use standard C library */
void *fmemcpy(void *dest, const void *src, size_t n) {
    return memcpy(dest, src, n);
}

void *fmemset(void *s, int c, size_t n) {
    return memset(s, c, n);
}

/* Timer interrupt stub */
volatile unsigned int timer_ticks = 0;

unsigned int get_timer_ticks(void) {
    return get_ticks();  /* Use SDL timer */
}

/* Sound Blaster function stubs */
void sb_Reset(int baseport) {
    (void)baseport;
    /* Stub - no operation */
}

void sb_DacSpkrOn(int baseport) {
    (void)baseport;
    /* Stub - no operation */
}

void sb_DacSpkrOff(int baseport) {
    (void)baseport;
    /* Stub - no operation */
}

void sb_Speed(int baseport, int speed, unsigned char value) {
    (void)baseport;
    (void)speed;
    (void)value;
    /* Stub - no operation */
}

void sb_Play(int baseport, int buffsize) {
    (void)baseport;
    (void)buffsize;
    /* Stub - no operation */
}

/* Graphics function stubs */
extern void Blit8(int bmap, int x, int y, int image);





#ifdef __cplusplus
}
#endif

