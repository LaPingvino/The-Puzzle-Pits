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

// farmalloc and farfree are macros for malloc and free (sdl_compat.h):
// functions here would define malloc and free themselves, calling
// themselves for ever

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

// present shows video memory (physical) in the window, in the current
// palette, as the VGA card did all the time; PITS_SHOT=file also writes
// it to file as a PPM (for testing without a window)
static void present(void) {
    uint8_t *pixels;
    int pitch;
    SDL_LockTexture(sdl_texture, NULL, (void**)&pixels, &pitch);
    for (int y = 0; y < 200; y++) {
        for (int x = 0; x < 320; x++) {
            uint8_t *c = vga_palette[vga_memory_buffer[y * 320 + x]];
            uint8_t *p = pixels + y * pitch + x * 4;
            p[0] = c[2]; p[1] = c[1]; p[2] = c[0]; p[3] = 0xFF; // BGRA
        }
    }
    SDL_UnlockTexture(sdl_texture);
    SDL_RenderClear(sdl_renderer);
    SDL_RenderCopy(sdl_renderer, sdl_texture, NULL, NULL);
    SDL_RenderPresent(sdl_renderer);

    const char *shot = getenv("PITS_SHOT");
    if (shot) {
        FILE *f = fopen(shot, "wb");
        if (f) {
            fprintf(f, "P6 320 200 255\n");
            for (int i = 0; i < 320 * 200; i++) {
                fwrite(vga_palette[vga_memory_buffer[i]], 1, 3, f);
            }
            fclose(f);
        }
    }
}

// refresh shows video memory if a frame's time (1/60 s) has passed: the
// game draws into it and changes the palette without saying when it is
// done, and calls this through its timer and input checks while waiting
static void refresh(void) {
    static uint32_t last;
    uint32_t now = SDL_GetTicks();
    if (sdl_texture && now - last >= 16) {
        last = now;
        present();
    }
}

// screenswap copies a page drawn off screen into video memory, and shows it
void screenswap(char *screen) {
    if ((uint8_t *)screen != vga_memory_buffer) {
        memcpy(vga_memory_buffer, screen, 320 * 200);
    }
    present();
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
// key_event is a key pressed or let go: the keyboard table the game reads
// for moving (its DOS scan codes), and on a first press, the key for inkey
static void key_event(SDL_Keycode sym, int down, int repeat) {
    int scan = 0;
    switch (sym) {
        case SDLK_UP: scan = 72; break;
        case SDLK_DOWN: scan = 80; break;
        case SDLK_LEFT: scan = 75; break;
        case SDLK_RIGHT: scan = 77; break;
        case SDLK_RETURN: scan = 28; break;
        case SDLK_SPACE: scan = 58; break; // the game's SPKEY
        case SDLK_F1: scan = 59; break;
        case SDLK_F4: scan = 62; break;
        case SDLK_ESCAPE: scan = 1; break;
        case SDLK_LSHIFT: scan = 42; break;
        case SDLK_RSHIFT: scan = 54; break;
    }
    if (scan) {
        debug_keytable[scan] = down ? scan : 0;
    }
    if (down && !repeat && pending_inkey == 0) {
        pending_inkey = sdl_to_inkey(sym);
    }
    sync_keytable();
}

// scripted presses PITS_KEYS="ms:Key,ms:Key" for testing without a
// keyboard: each key (an SDL key name) is pressed at its time and held
// for 100 ms, as a finger would (the game waits for keys to be let go)
static void scripted_keys(void) {
    static const char *next;
    static int started;
    static SDL_Keycode held;
    static uint32_t release;
    if (held && SDL_GetTicks() >= release) {
        key_event(held, 0, 0);
        held = 0;
    }
    if (held) {
        return;
    }
    if (!started) {
        started = 1;
        next = getenv("PITS_KEYS");
    }
    while (next && *next) {
        char name[32];
        unsigned at;
        int len = 0;
        if (sscanf(next, "%u:%31[^,]%n", &at, name, &len) < 2) {
            next = NULL;
            return;
        }
        if (SDL_GetTicks() < at) {
            return;
        }
        next += len + (next[len] == ',');
        held = SDL_GetKeyFromName(name);
        release = SDL_GetTicks() + 100;
        key_event(held, 1, 0);
        return;
    }
}

static void process_sdl_events(void) {
    refresh();
    scripted_keys();
    while (SDL_PollEvent(&sdl_event)) {
        if (sdl_event.type == SDL_KEYDOWN || sdl_event.type == SDL_KEYUP) {
            key_event(sdl_event.key.keysym.sym, sdl_event.type == SDL_KEYDOWN, sdl_event.key.repeat);
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
            keytable_access_logged[i] = 1;
        }
    }
    
    // Return current mouse button state
    return mouse_buttons;
}

void relpos(int16_t *x, int16_t *y) {
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
    process_sdl_events(); // the game polls the timer while it waits
    // Return ticks at 18.2 Hz like DOS timer
    return (SDL_GetTicks() * 182) / 10000;
}

void delay(uint32_t ms) {
    SDL_Delay(ms);
}