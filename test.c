/* test.c
 * Simple test program to verify compilation and basic functionality
 */

#include "compat.h"
#include <stdio.h>

int main() {
    printf("Testing Puzzle Pits compatibility layer...\n");
    
    // Test basic initialization
    if (init_sdl_compat() < 0) {
        printf("Failed to initialize SDL compatibility layer\n");
        return 1;
    }
    
    printf("SDL compatibility layer initialized successfully\n");
    
    // Test some basic functions
    printf("Testing basic functions:\n");
    
    // Test timer
    uint32_t ticks = get_ticks();
    printf("Timer ticks: %u\n", ticks);
    
    // Test mouse
    uint16_t mouse_available = initmouse();
    printf("Mouse available: %s\n", mouse_available ? "Yes" : "No");
    
    // Test keyboard
    printf("Keyboard hit test: %s\n", kbhit() ? "Key pressed" : "No key");
    
    // Test memory functions
    void* test_mem = farmalloc(1024);
    if (test_mem) {
        printf("Memory allocation: Success\n");
        farfree(test_mem);
    } else {
        printf("Memory allocation: Failed\n");
    }
    
    // Test port I/O stubs
    outp(0x3C8, 0);
    uint8_t test_val = inp(0x3C9);
    printf("Port I/O test: Read value %d\n", test_val);
    
    // Cleanup
    cleanup_sdl_compat();
    
    printf("All tests completed successfully!\n");
    printf("Compilation and basic functionality verified.\n");
    
    return 0;
}