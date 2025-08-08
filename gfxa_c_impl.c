/* gfxa_c_impl.c
 * Complete C replacement for GFXA.ASM assembly graphics functions
 * Provides SDL and stub implementations for DOS graphics compatibility
 */

#define USE_FULL_COMPAT_IMPL

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "compat.h"

// External keyboard table - this needs to be accessible from other modules
unsigned char keytable[256];

// Video memory buffer for VGA mode 13h (320x200x256)
uint8_t *vga_memory_buffer = NULL;

// Static variables for keyboard handling
static int keyboard_initialized = 0;
static int mouse_initialized = 0;
static int current_video_mode = 3;  // Default text mode

// Mouse state variables
static int mouse_x = 0, mouse_y = 0;
static int mouse_buttons = 0;
static int mouse_rel_x = 0, mouse_rel_y = 0;

// Video memory buffer is defined above

// Forward declarations for compatibility layer functions
#ifdef USE_SDL
extern int init_sdl_compat(void);
extern void cleanup_sdl_compat(void);
extern void update_sdl_display(void);
extern int sdl_kbhit(void);
extern int sdl_getch(void);
extern uint16_t sdl_initmouse(void);
extern uint16_t sdl_readmbutton(void);
extern void sdl_relpos(int *x, int *y);
extern void sdl_setpalette(char *colorlist);
#endif

/*
 * Video Mode Functions
 */

// _setmode - Set video mode (replaces assembly _setmode PROC)
void setmode(int mode) {
    current_video_mode = mode;
    
    if (mode == 0x13) {  // VGA 320x200x256 mode
        #ifdef USE_SDL
        // Initialize SDL graphics mode
        if (init_sdl_compat() < 0) {
            fprintf(stderr, "Failed to initialize SDL graphics mode\n");
            return;
        }
        #else
        // Allocate video buffer for stub mode
        if (!vga_memory_buffer) {
            vga_memory_buffer = (uint8_t*)malloc(320 * 200);
            if (vga_memory_buffer) {
                memset(vga_memory_buffer, 0, 320 * 200);
            }
        }
        #endif
    } else if (mode == 3) {  // Text mode
        #ifdef USE_SDL
        cleanup_sdl_compat();
        #else
        if (vga_memory_buffer) {
            free(vga_memory_buffer);
            vga_memory_buffer = NULL;
        }
        #endif
    }
}

// _cls - Clear screen (replaces assembly _cls PROC)
void cls(char *screen) {
    if (!screen) {
        if (vga_memory_buffer) {
            memset(vga_memory_buffer, 0, 320 * 200);
        }
        return;
    }
    
    // Clear the specified screen buffer
    if (current_video_mode == 0x13) {
        memset(screen, 0, 320 * 200);
    } else {
        // Text mode - clear 80x25 characters (2 bytes each)
        memset(screen, 0, 80 * 25 * 2);
    }
}

// _screenswap - Copy buffer to video memory (replaces assembly _screenswap PROC)
void screenswap(char *screen) {
    if (!screen) return;
    
    if (current_video_mode == 0x13) {
        #ifdef USE_SDL
        // Copy screen buffer to SDL surface and update display
        if (vga_memory_buffer) {
            memcpy(vga_memory_buffer, screen, 320 * 200);
            update_sdl_display();
        }
        #else
        // Stub mode - just copy to our buffer
        if (vga_memory_buffer) {
            memcpy(vga_memory_buffer, screen, 320 * 200);
        }
        #endif
    }
}

// _setpalette - Set VGA palette (replaces assembly _setpalette PROC)
void setpalette(char *regs) {
    if (!regs) return;
    
    #ifdef USE_SDL
    sdl_setpalette(regs);
    #else
    // Stub mode - no operation needed
    (void)regs;
    #endif
}

/*
 * Mouse Functions
 */

// _initmouse - Initialize mouse (replaces assembly _initmouse PROC)
uint16_t initmouse(void) {
    if (mouse_initialized) {
        return 1;  // Already initialized
    }
    
    #ifdef USE_SDL
    uint16_t result = sdl_initmouse();
    if (result) {
        mouse_initialized = 1;
        mouse_x = 160;  // Center of 320px screen
        mouse_y = 100;  // Center of 200px screen
        mouse_buttons = 0;
    }
    return result;
    #else
    // Stub mode - simulate successful initialization
    mouse_initialized = 1;
    mouse_x = 160;
    mouse_y = 100;
    mouse_buttons = 0;
    return 1;
    #endif
}

// _readmbutton - Read mouse button status (replaces assembly _readmbutton PROC)
uint16_t readmbutton(void) {
    if (!mouse_initialized) {
        return 0;
    }
    
    #ifdef USE_SDL
    return sdl_readmbutton();
    #else
    // Stub mode - no buttons pressed
    return 0;
    #endif
}

// _relpos - Get relative mouse movement (replaces assembly _relpos PROC)
void relpos(int *x, int *y) {
    if (!x || !y) return;
    
    if (!mouse_initialized) {
        *x = 0;
        *y = 0;
        return;
    }
    
    #ifdef USE_SDL
    sdl_relpos(x, y);
    #else
    // Stub mode - no movement
    *x = 0;
    *y = 0;
    #endif
}

/*
 * Keyboard Functions
 */

// Initialize keyboard table
static void init_keyboard_table(void) {
    memset(keytable, 0, sizeof(keytable));
    keyboard_initialized = 1;
}

// _setkb - Install keyboard handler (replaces assembly _setkb PROC)
void setkb(void) {
    if (!keyboard_initialized) {
        init_keyboard_table();
    }
    
    #ifdef USE_SDL
    // SDL handles keyboard input through event polling
    // No interrupt handler needed
    #else
    // Stub mode - keyboard table is ready
    #endif
}

// _resetkb - Remove keyboard handler (replaces assembly _resetkb PROC)
void resetkb(void) {
    if (keyboard_initialized) {
        memset(keytable, 0, sizeof(keytable));
        keyboard_initialized = 0;
    }
    
    #ifdef USE_SDL
    // SDL cleanup handled elsewhere
    #else
    // Stub mode - nothing to do
    #endif
}

// Keyboard input handling functions
int kbhit(void) {
    if (!keyboard_initialized) {
        init_keyboard_table();
    }
    
    #ifdef USE_SDL
    return sdl_kbhit();
    #else
    // Stub mode - no input available
    return 0;
    #endif
}

int getch(void) {
    if (!keyboard_initialized) {
        init_keyboard_table();
    }
    
    #ifdef USE_SDL
    return sdl_getch();
    #else
    // Stub mode - return a safe default
    return 0;
    #endif
}

// Get next keypress (used by GFX.C)
int get_next_keypress(void) {
    #ifdef USE_SDL
    // Process SDL events and return pending key
    if (kbhit()) {
        return getch();
    }
    return 0;
    #else
    // Stub mode - no keypress
    return 0;
    #endif
}

/*
 * Timer Functions
 */

// Timer tick counter (18.2 Hz like DOS)
static uint32_t timer_tick_counter = 0;

uint32_t get_ticks(void) {
    #ifdef USE_SDL
    // Convert SDL millisecond timer to DOS-style 18.2 Hz ticks
    extern uint32_t sdl_get_ticks(void);
    return sdl_get_ticks();
    #else
    // Stub mode - increment counter slowly
    return ++timer_tick_counter;
    #endif
}

void delay(uint32_t ms) {
    #ifdef USE_SDL
    extern void sdl_delay(uint32_t ms);
    sdl_delay(ms);
    #else
    // Stub mode - no actual delay
    (void)ms;
    #endif
}

/*
 * Sound Functions (stubs)
 */

void sb_Reset(int baseport) {
    #ifdef USE_SDL
    // TODO: Implement SDL audio initialization
    #endif
    (void)baseport;
}

void sb_DacSpkrOn(int baseport) {
    #ifdef USE_SDL
    // TODO: Implement SDL audio speaker on
    #endif
    (void)baseport;
}

void sb_DacSpkrOff(int baseport) {
    #ifdef USE_SDL
    // TODO: Implement SDL audio speaker off
    #endif
    (void)baseport;
}

void sb_Speed(int baseport, int speed, unsigned char value) {
    #ifdef USE_SDL
    // TODO: Implement SDL audio speed setting
    #endif
    (void)baseport;
    (void)speed;
    (void)value;
}

void sb_Play(int baseport, int buffsize) {
    #ifdef USE_SDL
    // TODO: Implement SDL audio playback
    #endif
    (void)baseport;
    (void)buffsize;
}

/*
 * Utility Functions
 */

// Clean exit function
void cleanExit(int retval) {
    // Reset video mode
    if (current_video_mode == 0x13) {
        setmode(3);  // Return to text mode
    }
    
    // Clean up keyboard
    if (keyboard_initialized) {
        resetkb();
    }
    
    // Clean up SDL
    #ifdef USE_SDL
    cleanup_sdl_compat();
    #endif
    
    // Free video buffer
    if (vga_memory_buffer) {
        free(vga_memory_buffer);
        vga_memory_buffer = NULL;
    }
    
    exit(retval);
}

/*
 * Compatibility Function Aliases
 * These provide the exact function names expected by the DOS code
 */

// Video functions with underscore prefix (DOS calling convention)
void _setmode(int mode) { setmode(mode); }
void _cls(char *screen) { cls(screen); }
void _screenswap(char *screen) { screenswap(screen); }
void _setpalette(char *regs) { setpalette(regs); }

// Mouse functions with underscore prefix
uint16_t _initmouse(void) { return initmouse(); }
uint16_t _readmbutton(void) { return readmbutton(); }
void _relpos(int *x, int *y) { relpos(x, y); }

// Keyboard functions with underscore prefix
void _setkb(void) { setkb(); }
void _resetkb(void) { resetkb(); }

/*
 * Initialization and Cleanup
 */

// Initialize all graphics subsystems
int init_gfx_compat(void) {
    // Initialize keyboard
    init_keyboard_table();
    
    // Initialize video buffer
    if (!vga_memory_buffer) {
        vga_memory_buffer = (uint8_t*)malloc(320 * 200);
        if (vga_memory_buffer) {
            memset(vga_memory_buffer, 0, 320 * 200);
        } else {
            return -1;
        }
    }
    
    #ifdef USE_SDL
    return init_sdl_compat();
    #else
    return 0;  // Stub mode always succeeds
    #endif
}

// Cleanup all graphics subsystems
void cleanup_gfx_compat(void) {
    resetkb();
    
    if (vga_memory_buffer) {
        free(vga_memory_buffer);
        vga_memory_buffer = NULL;
    }
    
    #ifdef USE_SDL
    cleanup_sdl_compat();
    #endif
}