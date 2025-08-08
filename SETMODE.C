/* SETMODE.C
 * Video mode handling functions
 * Replaces DOS BIOS interrupt calls with modern C implementation
 */

#include <stdio.h>
#include <stdint.h>
#include "compat.h"

// Global variable to track current video mode
static int current_video_mode = 3;  // Default to text mode

/*
 * getvmode - Get current video mode
 * Replaces DOS BIOS interrupt 10h, function 0Fh
 * Returns: Current video mode number
 */
int getvmode(void) {
    return current_video_mode;
}

/*
 * setvmode - Set video mode
 * Replaces DOS BIOS interrupt 10h, function 00h
 * Parameters: mode - Video mode to set
 * Returns: 0 on success, -1 on failure
 */
int setvmode(int mode) {
    // Validate mode
    if (mode < 0 || mode > 0x13) {
        return -1;  // Invalid mode
    }
    
    // Store the new mode
    int old_mode = current_video_mode;
    current_video_mode = mode;
    
    // Handle mode-specific initialization
    switch (mode) {
        case 0:     // 40x25 text mode
        case 1:     // 40x25 text mode with color
        case 2:     // 80x25 text mode
        case 3:     // 80x25 text mode with color
            // Text modes - clean up graphics if needed
            #ifdef USE_SDL
            extern void cleanup_sdl_compat(void);
            if (old_mode == 0x13) {
                cleanup_sdl_compat();
            }
            #endif
            break;
            
        case 0x13:  // VGA 320x200x256 graphics mode
            // Initialize graphics mode
            #ifdef USE_SDL
            extern int init_sdl_compat(void);
            if (init_sdl_compat() < 0) {
                // Failed to initialize SDL graphics
                current_video_mode = old_mode;  // Restore old mode
                return -1;
            }
            #else
            // Stub mode - allocate video buffer
            extern uint8_t *vga_memory_buffer;
            if (!vga_memory_buffer) {
                vga_memory_buffer = (uint8_t*)malloc(320 * 200);
                if (!vga_memory_buffer) {
                    current_video_mode = old_mode;
                    return -1;
                }
                memset(vga_memory_buffer, 0, 320 * 200);
            }
            #endif
            break;
            
        default:
            // Unsupported mode
            current_video_mode = old_mode;
            return -1;
    }
    
    // Call the main setmode function for compatibility
    extern void setmode(int mode);
    setmode(mode);
    
    return 0;  // Success
}

/*
 * Additional utility functions for video mode management
 */

/*
 * is_graphics_mode - Check if current mode is a graphics mode
 * Returns: 1 if graphics mode, 0 if text mode
 */
int is_graphics_mode(void) {
    return (current_video_mode == 0x13);
}

/*
 * get_screen_width - Get screen width for current mode
 * Returns: Screen width in pixels or characters
 */
int get_screen_width(void) {
    switch (current_video_mode) {
        case 0:
        case 1:
            return 40;  // 40 column text
        case 2:
        case 3:
            return 80;  // 80 column text
        case 0x13:
            return 320; // VGA graphics
        default:
            return 80;  // Default
    }
}

/*
 * get_screen_height - Get screen height for current mode
 * Returns: Screen height in pixels or characters
 */
int get_screen_height(void) {
    switch (current_video_mode) {
        case 0:
        case 1:
        case 2:
        case 3:
            return 25;  // 25 row text
        case 0x13:
            return 200; // VGA graphics
        default:
            return 25;  // Default
    }
}

/*
 * get_screen_colors - Get number of colors for current mode
 * Returns: Number of colors supported
 */
int get_screen_colors(void) {
    switch (current_video_mode) {
        case 0:
        case 2:
            return 16;  // Monochrome/16 color text
        case 1:
        case 3:
            return 16;  // 16 color text
        case 0x13:
            return 256; // 256 color VGA
        default:
            return 16;  // Default
    }
}

/*
 * reset_video_mode - Reset to default text mode
 * Useful for cleanup on exit
 */
void reset_video_mode(void) {
    setvmode(3);  // 80x25 color text mode
}