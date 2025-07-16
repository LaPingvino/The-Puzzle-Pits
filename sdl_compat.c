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
    return 1;  // Mouse available
}

uint16_t readmbutton(void) {
    uint8_t buttons = 0;
    uint32_t state = SDL_GetMouseState(NULL, NULL);
    
    if (state & SDL_BUTTON(SDL_BUTTON_LEFT)) buttons |= 1;
    if (state & SDL_BUTTON(SDL_BUTTON_RIGHT)) buttons |= 2;
    if (state & SDL_BUTTON(SDL_BUTTON_MIDDLE)) buttons |= 4;
    
    return buttons;
}

void relpos(int *x, int *y) {
    int mx, my;
    SDL_GetRelativeMouseState(&mx, &my);
    
    // Scale down by 2 since window is scaled up
    *x = mx / 2;
    *y = my / 2;
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
    while (SDL_PollEvent(&sdl_event)) {
        if (sdl_event.type == SDL_KEYDOWN) {
            // Add to keyboard buffer
            int next_tail = (key_buffer_tail + 1) % 256;
            if (next_tail != key_buffer_head) {
                key_buffer[key_buffer_tail] = sdl_event.key.keysym.sym & 0xFF;
                key_buffer_tail = next_tail;
            }
        } else if (sdl_event.type == SDL_QUIT) {
            // Handle quit event
            exit(0);
        }
    }
    
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

// Timer functions
uint32_t get_ticks(void) {
    // Return ticks at 18.2 Hz like DOS timer
    return (SDL_GetTicks() * 182) / 10000;
}

void delay(uint32_t ms) {
    SDL_Delay(ms);
}