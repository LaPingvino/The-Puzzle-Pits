#ifndef SDL_COMPAT_H
#define SDL_COMPAT_H

#include <SDL2/SDL.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// DOS compatibility macros
#define MK_FP(seg, off) ((void*)((seg << 4) + off))
#define farmalloc(size) malloc(size)
#define farfree(ptr) free(ptr)
#define _fmemcpy(dest, src, size) memcpy(dest, src, size)
#define halloc(nitems, size) malloc((nitems) * (size))
#define hfree(ptr) free(ptr)

// Video memory emulation
extern uint8_t *vga_memory_buffer;  // 320x200 VGA buffer

// SDL globals
extern SDL_Window *sdl_window;
extern SDL_Renderer *sdl_renderer;
extern SDL_Texture *sdl_texture;
extern SDL_Event sdl_event;

// Video mode functions
void setmode(int mode);
void cls(char *screen);
void screenswap(char *screen);
void setpalette(char *colorlist);
void clearscreen(char *vscreen);

// Mouse functions
uint16_t initmouse(void);
uint16_t readmbutton(void);
void relpos(int *x, int *y);

// Keyboard functions
void setkb(void);
void resetkb(void);
int kbhit(void);
int getch(void);

// Timer functions
uint32_t get_ticks(void);
void delay(uint32_t ms);

// Sound stubs (to be implemented with SDL_mixer later)
#ifndef SOUND_DEFINED
#define SOUND_DEFINED
typedef struct {
    uint8_t *samples;
    uint32_t length;
} SOUND;
#endif

#ifndef ENV_BLASTER_DEFINED
#define ENV_BLASTER_DEFINED
typedef struct {
    int ioaddr;
    int irqlev;
    int dmachan;
} env_BLASTER;
#endif

// DOS interrupt stubs
#define disable() SDL_DisableScreenSaver()
#define enable() SDL_EnableScreenSaver()
#define _disable() SDL_DisableScreenSaver()
#define _enable() SDL_EnableScreenSaver()

// Port I/O stubs
static inline uint8_t inp(uint16_t port) {
    // Stub for port input - return 0
    return 0;
}

static inline void outp(uint16_t port, uint8_t value) {
    // Stub for port output - no operation
    (void)port;
    (void)value;
}

static inline uint8_t inportb(uint16_t port) {
    return inp(port);
}

static inline void outportb(uint16_t port, uint8_t value) {
    outp(port, value);
}

// Interrupt function declaration stub
#define _interrupt
#define interrupt

// DOS interrupt vector functions
typedef void (*interrupt_handler)(void);

static inline interrupt_handler _dos_getvect(int intno) {
    (void)intno;
    return NULL;  // Return null handler
}

static inline void _dos_setvect(int intno, interrupt_handler handler) {
    (void)intno;
    (void)handler;
    // No operation for stub
}

// File mode constants
#ifndef O_BINARY
#define O_BINARY 0
#endif

// Initialize SDL subsystems
int init_sdl_compat(void);
void cleanup_sdl_compat(void);

#endif // SDL_COMPAT_H