#ifndef NOSDL_COMPAT_H
#define NOSDL_COMPAT_H

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>

// Only define stub functions if not using full implementation
#ifndef USE_FULL_COMPAT_IMPL

// DOS compatibility macros
#define MK_FP(seg, off) ((void*)((seg << 4) + off))
#define farmalloc(size) malloc(size)
#define farfree(ptr) free(ptr)
#define _fmemcpy(dest, src, size) memcpy(dest, src, size)
#define halloc(nitems, size) malloc((nitems) * (size))
#define hfree(ptr) free(ptr)

// Video memory emulation
static uint8_t *vga_memory_buffer = NULL;

// DOS interrupt stubs
#define disable() 
#define enable() 
#define _disable() 
#define _enable() 

// Port I/O stubs
static inline uint8_t inp(uint16_t port) {
    return 0;
}

static inline void outp(uint16_t port, uint8_t value) {
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
// Video mode functions
// named apart from the C library's setmode (io.h, unistd.h), included first
#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif
#define setmode pits_setmode
static inline void setmode(int mode) {
    if (mode == 0x13) {
        if (!vga_memory_buffer) {
            vga_memory_buffer = (uint8_t*)malloc(320 * 200);
            memset(vga_memory_buffer, 0, 320 * 200);
        }
    }
}

static inline void cls(char *screen) {
    memset(screen, 0, 320 * 200);
}

static inline void screenswap(char *screen) {
    // In real implementation, this would update the display
    (void)screen;
}

static inline void setpalette(char *colorlist) {
    (void)colorlist;
}

static inline void clearscreen(char *vscreen) {
    memset(vscreen, 0, 64000);
}

// Mouse functions
static inline uint16_t initmouse(void) {
    return 1;  // Mouse available
}

static inline uint16_t readmbutton(void) {
    return 0;  // No buttons pressed
}

static inline void warpmouse(int x, int y) { (void)x; (void)y; }
static inline int progress_get(char *level) { level[0] = 0; return 0; }
static inline void progress_reached(const char *level) { (void)level; }
static inline void relpos(int16_t *x, int16_t *y) {
    *x = 0;
    *y = 0;
}

// Keyboard functions
static inline void setkb(void) {
}

static inline void resetkb(void) {
}

static inline int kbhit(void) {
    return 0;
}

static inline int getch(void) {
    return getchar();
}

static inline int get_next_keypress(void) {
    return 0;  // No keypress in stub mode
}

// Timer functions
static inline uint32_t get_ticks(void) {
    return 0;
}

static inline void delay(uint32_t ms) {
    (void)ms;
}

// Initialize compatibility layer
static inline int init_sdl_compat(void) {
    vga_memory_buffer = (uint8_t*)malloc(320 * 200);
    if (!vga_memory_buffer) {
        return -1;
    }
    memset(vga_memory_buffer, 0, 320 * 200);
    return 0;
}

static inline void cleanup_sdl_compat(void) {
    if (vga_memory_buffer) {
        free(vga_memory_buffer);
        vga_memory_buffer = NULL;
    }
}

// Sound stubs - typedefs moved to OLDSOUND.H to avoid duplication

#endif // USE_FULL_COMPAT_IMPL

#endif // NOSDL_COMPAT_H