#include "compat.h"
#include <stdio.h>

// Global SDL variables
SDL_Window *sdl_window = NULL;
SDL_Renderer *sdl_renderer = NULL;
SDL_Texture *sdl_texture = NULL;
SDL_Event sdl_event;

// VGA memory buffer (320x200)
uint8_t *vga_memory_buffer = NULL;

// VGA palette (256 colors, RGB)
static uint8_t vga_palette[256][3];

// Mouse state
static int mouse_x = 160, mouse_y = 100;
static int mouse_dx = 0, mouse_dy = 0;
static uint8_t mouse_buttons = 0;

// Keyboard state
static uint8_t key_buffer[256];
static int key_buffer_head = 0;
static int key_buffer_tail = 0;

// Direct keypress handling for inkey
static int pending_inkey = 0;

// External keytable array from game code
extern char keytable[256];

// Debug keytable to intercept and log access
static char debug_keytable[256];
static int keytable_access_logged[256] = {0};

// Sync debug keytable to real keytable
static void sync_keytable(void) {
    for (int i = 0; i < 256; i++) {
        keytable[i] = debug_keytable[i];
    }
}

// External mouse and keyboard state from game code
extern int button1, button2;
extern int mx, my;
extern char inkey;

// DOS memory allocation compatibility functions
void* farmalloc(size_t size) {
    return malloc(size);
}

void farfree(void* ptr) {
    free(ptr);
}

// Initialize SDL subsystems
int init_sdl_compat(void) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) < 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return -1;
    }

    // Create window (scale up 2x for modern displays)
    sdl_window = SDL_CreateWindow(
        "The Puzzle Pits",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        640, 400,  // 320x200 scaled 2x
        SDL_WINDOW_SHOWN
    );
    
    if (!sdl_window) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        return -1;
    }

    sdl_renderer = SDL_CreateRenderer(sdl_window, -1, SDL_RENDERER_ACCELERATED);
    if (!sdl_renderer) {
        fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        return -1;
    }

    // Create texture for VGA buffer
    sdl_texture = SDL_CreateTexture(
        sdl_renderer,
        SDL_PIXELFORMAT_RGB888,
        SDL_TEXTUREACCESS_STREAMING,
        320, 200
    );
    
    if (!sdl_texture) {
        fprintf(stderr, "SDL_CreateTexture failed: %s\n", SDL_GetError());
        return -1;
    }

    // Allocate VGA memory buffer
    vga_memory_buffer = (uint8_t*)malloc(320 * 200);
    if (!vga_memory_buffer) {
        fprintf(stderr, "Failed to allocate VGA buffer\n");
        return -1;
    }
    
    // Clear buffer
    memset(vga_memory_buffer, 0, 320 * 200);
    
    // Initialize default VGA palette
    for (int i = 0; i < 256; i++) {
        vga_palette[i][0] = (i & 0xE0) | ((i & 0xE0) >> 3) | ((i & 0xC0) >> 6);
        vga_palette[i][1] = ((i & 0x1C) << 3) | ((i & 0x1C)) | ((i & 0x18) >> 3);
        vga_palette[i][2] = ((i & 0x03) << 6) | ((i & 0x03) << 4) | ((i & 0x03) << 2) | (i & 0x03);
    }

    return 0;
}

void cleanup_sdl_compat(void) {
    if (vga_memory_buffer) {
        free(vga_memory_buffer);
        vga_memory_buffer = NULL;
    }
    
    if (sdl_texture) {
        SDL_DestroyTexture(sdl_texture);
        sdl_texture = NULL;
    }
    
    if (sdl_renderer) {
        SDL_DestroyRenderer(sdl_renderer);
        sdl_renderer = NULL;
    }
    
    if (sdl_window) {
        SDL_DestroyWindow(sdl_window);
        sdl_window = NULL;
    }
    
    SDL_Quit();
}

// Video mode functions
void setmode(int mode) {
    // Mode 0x13 is VGA 320x200 256 colors
    if (mode == 0x13) {
        // Already initialized in init_sdl_compat
        memset(vga_memory_buffer, 0, 320 * 200);
    } else if (mode == 0x03) {
        // Text mode - just clear for now
        memset(vga_memory_buffer, 0, 320 * 200);
    }
}

void cls(char *screen) {
    memset(screen, 0, 320 * 200);
}

void screenswap(char *screen) {
    // Convert indexed color to RGB
    uint8_t *pixels;
    int pitch;
    
    SDL_LockTexture(sdl_texture, NULL, (void**)&pixels, &pitch);
    
    for (int y = 0; y < 200; y++) {
        for (int x = 0; x < 320; x++) {
            uint8_t color = screen[y * 320 + x];
            int offset = y * pitch + x * 4;
            pixels[offset + 0] = vga_palette[color][2];  // B
            pixels[offset + 1] = vga_palette[color][1];  // G
            pixels[offset + 2] = vga_palette[color][0];  // R
            pixels[offset + 3] = 0xFF;                   // A
        }
    }
    
    SDL_UnlockTexture(sdl_texture);
    
    // Render to screen
    SDL_RenderClear(sdl_renderer);
    SDL_RenderCopy(sdl_renderer, sdl_texture, NULL, NULL);
    SDL_RenderPresent(sdl_renderer);
}

void setpalette(char *colorlist) {
    // Assume colorlist is 768 bytes (256 colors * 3 bytes RGB)
    for (int i = 0; i < 256; i++) {
        // VGA palette values are 6-bit, scale to 8-bit
        vga_palette[i][0] = (colorlist[i * 3 + 0] & 0x3F) << 2;
        vga_palette[i][1] = (colorlist[i * 3 + 1] & 0x3F) << 2;
        vga_palette[i][2] = (colorlist[i * 3 + 2] & 0x3F) << 2;
    }
}

void clearscreen(char *vscreen) {
    memset(vscreen, 0, 320 * 200);
}

// Mouse functions
uint16_t initmouse(void) {
    SDL_ShowCursor(SDL_DISABLE);
    mouse_x = 160;
    mouse_y = 100;
    // Don't override mx, my - let the game set them in gfxInit()
    return 1;  // Mouse available
}

// Convert SDL keycode directly to game inkey value
static int sdl_to_inkey(SDL_Keycode key) {
    const Uint8* state = SDL_GetKeyboardState(NULL);
    int shift = state[SDL_SCANCODE_LSHIFT] || state[SDL_SCANCODE_RSHIFT];
    
    switch (key) {
        case SDLK_BACKSPACE: return 8;
        case SDLK_RETURN: return 13;
        case SDLK_ESCAPE: return 27;
        case SDLK_DELETE: return 127;
        case SDLK_UP: return 28;
        case SDLK_DOWN: return 29;
        case SDLK_RIGHT: return 30;
        case SDLK_LEFT: return 31;
        case SDLK_SPACE: return ' ';
        
        // Function keys
        case SDLK_F1: return 129;
        case SDLK_F2: return 130;
        case SDLK_F3: return 131;
        case SDLK_F4: return 132;
        case SDLK_F5: return 133;
        case SDLK_F6: return 134;
        case SDLK_F7: return 135;
        case SDLK_F8: return 136;
        case SDLK_F9: return 137;
        case SDLK_F10: return 138;
        
        // Letters
        case SDLK_a: case SDLK_b: case SDLK_c: case SDLK_d: case SDLK_e:
        case SDLK_f: case SDLK_g: case SDLK_h: case SDLK_i: case SDLK_j:
        case SDLK_k: case SDLK_l: case SDLK_m: case SDLK_n: case SDLK_o:
        case SDLK_p: case SDLK_q: case SDLK_r: case SDLK_s: case SDLK_t:
        case SDLK_u: case SDLK_v: case SDLK_w: case SDLK_x: case SDLK_y:
        case SDLK_z:
            return shift ? (key - 32) : key;
            
        // Numbers and symbols
        case SDLK_0: return shift ? ')' : '0';
        case SDLK_1: return shift ? '!' : '1';
        case SDLK_2: return shift ? '@' : '2';
        case SDLK_3: return shift ? '#' : '3';
        case SDLK_4: return shift ? '$' : '4';
        case SDLK_5: return shift ? '%' : '5';
        case SDLK_6: return shift ? '^' : '6';
        case SDLK_7: return shift ? '&' : '7';
        case SDLK_8: return shift ? '*' : '8';
        case SDLK_9: return shift ? '(' : '9';
        
        default: return 0;
    }
}

// Process SDL events (called from CheckMouse and kbhit)
static void process_sdl_events(void) {
    while (SDL_PollEvent(&sdl_event)) {
        if (sdl_event.type == SDL_KEYDOWN) {
            // Handle movement keys directly for keytable (always, even on repeat)
            switch (sdl_event.key.keysym.sym) {
                case SDLK_UP: debug_keytable[72] = 72; break;
                case SDLK_DOWN: debug_keytable[80] = 80; break;
                case SDLK_LEFT: debug_keytable[75] = 75; break;
                case SDLK_RIGHT: debug_keytable[77] = 77; break;
                case SDLK_RETURN: debug_keytable[28] = 28; break;
                case SDLK_SPACE: debug_keytable[58] = 58; break;  // Fixed: SPKEY = 58, not 57
                case SDLK_F1: debug_keytable[59] = 59; break;
                case SDLK_F4: debug_keytable[62] = 62; break;
                case SDLK_ESCAPE: debug_keytable[1] = 1; break;
                case SDLK_LSHIFT: debug_keytable[42] = 42; break;
                case SDLK_RSHIFT: debug_keytable[54] = 54; break;
            }
            
            // Handle single keypresses for inkey (only on first press)
            if (!sdl_event.key.repeat && pending_inkey == 0) {
                pending_inkey = sdl_to_inkey(sdl_event.key.keysym.sym);
            }
            
            // Sync debug keytable to real keytable
            sync_keytable();
        } else if (sdl_event.type == SDL_KEYUP) {
            // Clear movement keys
            switch (sdl_event.key.keysym.sym) {
                case SDLK_UP: debug_keytable[72] = 0; break;
                case SDLK_DOWN: debug_keytable[80] = 0; break;
                case SDLK_LEFT: debug_keytable[75] = 0; break;
                case SDLK_RIGHT: debug_keytable[77] = 0; break;
                case SDLK_RETURN: debug_keytable[28] = 0; break;
                case SDLK_SPACE: debug_keytable[58] = 0; break;  // Fixed: SPKEY = 58, not 57
                case SDLK_F1: debug_keytable[59] = 0; break;
                case SDLK_F4: debug_keytable[62] = 0; break;
                case SDLK_ESCAPE: debug_keytable[1] = 0; break;
                case SDLK_LSHIFT: debug_keytable[42] = 0; break;
                case SDLK_RSHIFT: debug_keytable[54] = 0; break;
            }
            
            // Sync debug keytable to real keytable
            sync_keytable();
        } else if (sdl_event.type == SDL_MOUSEBUTTONDOWN) {
            if (sdl_event.button.button == SDL_BUTTON_LEFT) {
                mouse_buttons |= 1;
                button1 = 1;
            }
            if (sdl_event.button.button == SDL_BUTTON_RIGHT) {
                mouse_buttons |= 2;
                button2 = 1;
            }
        } else if (sdl_event.type == SDL_MOUSEBUTTONUP) {
            if (sdl_event.button.button == SDL_BUTTON_LEFT) {
                mouse_buttons &= ~1;
                button1 = 0;
            }
            if (sdl_event.button.button == SDL_BUTTON_RIGHT) {
                mouse_buttons &= ~2;
                button2 = 0;
            }
        } else if (sdl_event.type == SDL_MOUSEMOTION) {
            // Completely disable mouse movement to prevent array bounds crashes
            // The game's mouse sprite system can't handle SDL's large movement values
            mouse_dx = 0;
            mouse_dy = 0;
        } else if (sdl_event.type == SDL_QUIT) {
            // Handle quit event - this is critical for window closing!
            printf("Window close requested - exiting cleanly\n");
            cleanup_sdl_compat();
            exit(0);
        }
    }
}

uint16_t readmbutton(void) {
    // Process SDL events first to handle window closing and input
    process_sdl_events();
    
    // Check for access to unmapped keys
    for (int i = 0; i < 256; i++) {
        if (keytable[i] != 0 && debug_keytable[i] == 0 && !keytable_access_logged[i]) {
            printf("DEBUG: Game accessing unmapped keytable[%d] = %d\n", i, keytable[i]);
            fflush(stdout);
            keytable_access_logged[i] = 1;
        }
    }
    
    // Return current mouse button state
    return mouse_buttons;
}

void relpos(int *x, int *y) {
    // Process SDL events first to handle window closing and input
    process_sdl_events();
    
    // Return accumulated mouse movement
    *x = mouse_dx;
    *y = mouse_dy;
    
    // Reset movement counters
    mouse_dx = 0;
    mouse_dy = 0;
}

// Keyboard functions
void setkb(void) {
    // Clear keyboard buffer
    key_buffer_head = 0;
    key_buffer_tail = 0;
    memset(key_buffer, 0, sizeof(key_buffer));
}

void resetkb(void) {
    // Nothing special needed for SDL
}

int kbhit(void) {
    // Process SDL events
    process_sdl_events();
    
    return key_buffer_head != key_buffer_tail;
}

int getch(void) {
    // Wait for key if buffer is empty
    while (key_buffer_head == key_buffer_tail) {
        kbhit();
        SDL_Delay(10);
    }
    
    uint8_t key = key_buffer[key_buffer_head];
    key_buffer_head = (key_buffer_head + 1) % 256;
    return key;
}

// Get next keypress for inkey processing
int get_next_keypress(void) {
    // Process SDL events first
    process_sdl_events();
    
    if (pending_inkey > 0) {
        int key = pending_inkey;
        pending_inkey = 0;
        return key;
    }
    
    return 0;
}



// Timer functions
uint32_t get_ticks(void) {
    // Return ticks at 18.2 Hz like DOS timer
    return (SDL_GetTicks() * 182) / 10000;
}

void delay(uint32_t ms) {
    SDL_Delay(ms);
}