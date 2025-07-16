/* test_sdl.c
 * Test program specifically for SDL compatibility layer
 * This will help isolate if crashes are in SDL layer or game code
 */

#include "compat.h"
#include <stdio.h>

int main() {
    printf("Testing SDL compatibility layer...\n");
    
    // Test SDL initialization
    printf("Initializing SDL...\n");
    if (init_sdl_compat() < 0) {
        printf("ERROR: Failed to initialize SDL compatibility layer\n");
        return 1;
    }
    printf("SDL initialized successfully!\n");
    
    // Test video mode setting
    printf("Setting video mode 0x13 (320x200)...\n");
    setmode(0x13);
    printf("Video mode set.\n");
    
    // Test screen clearing
    printf("Testing screen operations...\n");
    char *test_screen = (char*)malloc(320 * 200);
    if (!test_screen) {
        printf("ERROR: Failed to allocate test screen buffer\n");
        cleanup_sdl_compat();
        return 1;
    }
    
    clearscreen(test_screen);
    printf("Screen cleared.\n");
    
    // Fill screen with test pattern
    for (int y = 0; y < 200; y++) {
        for (int x = 0; x < 320; x++) {
            test_screen[y * 320 + x] = (x + y) % 256;
        }
    }
    printf("Test pattern drawn.\n");
    
    // Test mouse initialization
    printf("Initializing mouse...\n");
    uint16_t mouse_available = initmouse();
    printf("Mouse available: %s\n", mouse_available ? "Yes" : "No");
    
    // Test keyboard initialization
    printf("Initializing keyboard...\n");
    setkb();
    printf("Keyboard initialized.\n");
    
    // Test timer
    printf("Testing timer...\n");
    uint32_t start_ticks = get_ticks();
    delay(100);  // 100ms delay
    uint32_t end_ticks = get_ticks();
    printf("Timer test: %u ticks elapsed during 100ms delay\n", end_ticks - start_ticks);
    
    // Simple render loop for a few frames
    printf("Starting render loop (5 seconds)...\n");
    uint32_t loop_start = get_ticks();
    uint32_t frame_count = 0;
    
    while ((get_ticks() - loop_start) < 91) {  // ~5 seconds at 18.2 Hz
        // Update test pattern
        for (int i = 0; i < 1000; i++) {
            int x = rand() % 320;
            int y = rand() % 200;
            test_screen[y * 320 + x] = (frame_count + x + y) % 256;
        }
        
        // Copy to screen
        screenswap(test_screen);
        
        // Check for events (quit, key press)
        if (kbhit()) {
            int key = getch();
            printf("Key pressed: %d (ASCII: %c)\n", key, (key >= 32 && key < 127) ? key : '?');
            if (key == 27) {  // ESC key
                printf("ESC pressed, exiting...\n");
                break;
            }
        }
        
        // Test mouse
        int mouse_dx, mouse_dy;
        relpos(&mouse_dx, &mouse_dy);
        uint16_t buttons = readmbutton();
        if (mouse_dx != 0 || mouse_dy != 0 || buttons != 0) {
            printf("Mouse: dx=%d, dy=%d, buttons=%u\n", mouse_dx, mouse_dy, buttons);
        }
        
        frame_count++;
        delay(55);  // ~18 FPS
    }
    
    printf("Render loop completed. Frames rendered: %u\n", frame_count);
    
    // Cleanup
    printf("Cleaning up...\n");
    free(test_screen);
    cleanup_sdl_compat();
    
    printf("SDL test completed successfully!\n");
    printf("If you see this message, the SDL compatibility layer is working.\n");
    
    return 0;
}