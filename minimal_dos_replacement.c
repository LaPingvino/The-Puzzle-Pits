/* minimal_dos_replacement.c
 * Minimal DOS replacement functions to enable Linux compatibility
 * Only replaces functions that absolutely cannot work on Linux
 * Avoids symbol conflicts with existing game code
 */

#define USE_FULL_COMPAT_IMPL

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "compat.h"

#ifdef USE_SDL
#include <SDL2/SDL.h>
#endif

/* Global state for compatibility */
static int video_mode = 3;
static uint8_t *vga_buffer = NULL;
static int sdl_initialized = 0;

#ifdef USE_SDL
static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;
static SDL_Texture *texture = NULL;
#endif

/* Assembly function replacements - these MUST be replaced since GFXA.ASM won't compile */

void _setmode(int mode) {
    video_mode = mode;
    
    if (mode == 0x13) {  // VGA 320x200x256
        if (!vga_buffer) {
            vga_buffer = malloc(320 * 200);
            if (vga_buffer) {
                memset(vga_buffer, 0, 320 * 200);
            }
        }
        
        #ifdef USE_SDL
        if (!sdl_initialized) {
            if (SDL_Init(SDL_INIT_VIDEO) == 0) {
                window = SDL_CreateWindow("The Puzzle Pits",
                    SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                    640, 400, SDL_WINDOW_SHOWN);
                if (window) {
                    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
                    if (renderer) {
                        texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB332,
                            SDL_TEXTUREACCESS_STREAMING, 320, 200);
                        sdl_initialized = 1;
                    }
                }
            }
        }
        #endif
    } else if (mode == 3) {  // Text mode
        #ifdef USE_SDL
        if (sdl_initialized) {
            if (texture) SDL_DestroyTexture(texture);
            if (renderer) SDL_DestroyRenderer(renderer);  
            if (window) SDL_DestroyWindow(window);
            SDL_Quit();
            sdl_initialized = 0;
            texture = NULL;
            renderer = NULL;
            window = NULL;
        }
        #endif
    }
}

void _cls(char *screen) {
    if (screen) {
        memset(screen, 0, 320 * 200);
    } else if (vga_buffer) {
        memset(vga_buffer, 0, 320 * 200);
    }
}

void _screenswap(char *screen) {
    if (!screen) return;
    
    #ifdef USE_SDL
    if (sdl_initialized && texture && renderer) {
        // Convert 8-bit palette to RGB
        uint32_t pixels[320 * 200];
        for (int i = 0; i < 320 * 200; i++) {
            uint8_t c = screen[i];
            // Simple palette conversion - could be improved
            pixels[i] = 0xFF000000 | (c << 16) | (c << 8) | c;
        }
        
        SDL_UpdateTexture(texture, NULL, pixels, 320 * sizeof(uint32_t));
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);
        
        // Process events to keep window responsive
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                exit(0);
            }
        }
    }
    #endif
    
    // Always copy to our buffer
    if (vga_buffer) {
        memcpy(vga_buffer, screen, 320 * 200);
    }
}

void _setpalette(char *regs) {
    // Palette setting not implemented - would need proper VGA palette handling
    (void)regs;
}

/* Mouse function replacements */
uint16_t _initmouse(void) {
    return 1;  // Mouse available
}

uint16_t _readmbutton(void) {
    #ifdef USE_SDL
    if (sdl_initialized) {
        return SDL_GetMouseState(NULL, NULL);
    }
    #endif
    return 0;  // No buttons pressed
}

void _relpos(int *x, int *y) {
    if (!x || !y) return;
    
    #ifdef USE_SDL
    if (sdl_initialized) {
        SDL_GetRelativeMouseState(x, y);
        return;
    }
    #endif
    
    *x = 0;
    *y = 0;
}

/* Keyboard function replacements */
void _setkb(void) {
    // SDL handles keyboard input automatically
}

void _resetkb(void) {
    // SDL cleanup handled elsewhere
}

/* Note: Hardware I/O stubs and DOS memory functions are handled by nosdl_compat.h */

/* Sound Blaster function stubs - required by original game code */
INT8 sb_Reset(WORD baseport) {
    (void)baseport;
    return 1;  // Success
}

void sb_DacSpkrOn(WORD baseport) {
    (void)baseport;
    // No-op on Linux
}

void sb_DacSpkrOff(WORD baseport) {
    (void)baseport;
    // No-op on Linux
}

void sb_Speed(WORD baseport, WORD speed, BYTE value) {
    (void)baseport;
    (void)speed;
    (void)value;
    // No-op on Linux
}

void sb_Play(WORD baseport, WORD buffsize) {
    (void)baseport;
    (void)buffsize;
    // No-op on Linux
}

/* Cleanup function */
void cleanup_minimal_dos(void) {
    #ifdef USE_SDL
    if (sdl_initialized) {
        if (texture) SDL_DestroyTexture(texture);
        if (renderer) SDL_DestroyRenderer(renderer);
        if (window) SDL_DestroyWindow(window);
        SDL_Quit();
        sdl_initialized = 0;
    }
    #endif
    
    if (vga_buffer) {
        free(vga_buffer);
        vga_buffer = NULL;
    }
}

/* Initialization function */
int init_minimal_dos(void) {
    // Basic initialization
    if (!vga_buffer) {
        vga_buffer = malloc(320 * 200);
        if (!vga_buffer) {
            return -1;
        }
        memset(vga_buffer, 0, 320 * 200);
    }
    
    #ifdef USE_SDL
    // SDL initialization happens in _setmode when needed
    #endif
    
    return 0;
}

/* Exit handler to ensure cleanup */
void minimal_dos_exit_handler(void) {
    cleanup_minimal_dos();
}

/* Auto-cleanup on program exit */
static void __attribute__((constructor)) setup_exit_handler(void) {
    atexit(minimal_dos_exit_handler);
}