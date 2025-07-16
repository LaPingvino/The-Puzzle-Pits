/* debug_test.c
 * Comprehensive debugging test suite for The Puzzle Pits segfault issues
 * Systematically tests each component that leads to the FadeIn() crash
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <setjmp.h>

// Include game headers
#include "compat.h"
#include "GFX.H"
#include "LOAD.H"

// Function declarations for mouse functions
extern void copymousebg(char *screen);
extern void drawmouse(char *screen);
extern void erasemouse(char *screen);

// Global variables for crash handling
static jmp_buf crash_env;
static int test_crashed = 0;

// Signal handler for segfaults
void crash_handler(int sig) {
    printf("CRASH: Segmentation fault detected in test!\n");
    test_crashed = 1;
    longjmp(crash_env, 1);
}

// Test result tracking
typedef struct {
    const char* name;
    int passed;
    int crashed;
    const char* error;
} TestResult;

#define MAX_TESTS 20
static TestResult test_results[MAX_TESTS];
static int test_count = 0;

// Helper macros
#define RUN_TEST(test_func) run_test(#test_func, test_func)
#define ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            printf("ASSERTION FAILED: %s\n", message); \
            return 0; \
        } \
    } while(0)

#define ASSERT_PTR_VALID(ptr, name) \
    ASSERT(ptr != NULL && (uintptr_t)ptr > 0x1000, name " pointer is invalid")

#define ASSERT_BOUNDS(ptr, base, size, name) \
    ASSERT(ptr >= base && ptr < base + size, name " pointer out of bounds")

// External variables we need to test
extern char *logical;
extern char *physical;
extern uint8_t *vga_memory_buffer;
extern WORD mx, my, moffx, moffy, mouseimage, mouseavailable;

// Test runner function
int run_test(const char* test_name, int (*test_func)(void)) {
    printf("\n=== Running Test: %s ===\n", test_name);
    
    // Set up crash handler
    signal(SIGSEGV, crash_handler);
    test_crashed = 0;
    
    int result = 0;
    if (setjmp(crash_env) == 0) {
        result = test_func();
        if (result) {
            printf("✓ PASSED: %s\n", test_name);
        } else {
            printf("✗ FAILED: %s\n", test_name);
        }
    } else {
        printf("✗ CRASHED: %s\n", test_name);
        result = 0;
    }
    
    // Record result
    if (test_count < MAX_TESTS) {
        test_results[test_count].name = test_name;
        test_results[test_count].passed = result && !test_crashed;
        test_results[test_count].crashed = test_crashed;
        test_results[test_count].error = test_crashed ? "Segmentation fault" : 
                                        (result ? "None" : "Assertion failed");
        test_count++;
    }
    
    // Reset signal handler
    signal(SIGSEGV, SIG_DFL);
    
    return result;
}

// Test 1: Basic memory allocation
int test_memory_allocation(void) {
    printf("Testing farmalloc/farfree...\n");
    
    void* small_mem = farmalloc(1024);
    ASSERT_PTR_VALID(small_mem, "small_mem");
    farfree(small_mem);
    
    void* large_mem = farmalloc(64000);
    ASSERT_PTR_VALID(large_mem, "large_mem");
    farfree(large_mem);
    
    printf("Memory allocation tests passed\n");
    return 1;
}

// Test 2: SDL initialization
int test_sdl_initialization(void) {
    printf("Testing SDL initialization...\n");
    
    int result = init_sdl_compat();
    ASSERT(result == 0, "SDL initialization failed");
    
    ASSERT_PTR_VALID(vga_memory_buffer, "vga_memory_buffer");
    
    printf("SDL initialization successful\n");
    cleanup_sdl_compat();
    return 1;
}

// Test 3: Graphics system initialization
int test_graphics_initialization(void) {
    printf("Testing graphics system initialization...\n");
    
    int sdl_result = init_sdl_compat();
    ASSERT(sdl_result == 0, "SDL initialization failed");
    
    gfxInit();
    
    printf("Checking pointer validity...\n");
    printf("logical = %p\n", logical);
    printf("physical = %p\n", physical);
    printf("vga_memory_buffer = %p\n", vga_memory_buffer);
    
    ASSERT_PTR_VALID(logical, "logical");
    ASSERT_PTR_VALID(physical, "physical");
    ASSERT_PTR_VALID(vga_memory_buffer, "vga_memory_buffer");
    
    printf("Checking mouse initialization...\n");
    printf("mx=%d, my=%d, moffx=%d, moffy=%d\n", mx, my, moffx, moffy);
    
    ASSERT(mx > 0 && mx < 320, "mx out of range");
    ASSERT(my > 0 && my < 200, "my out of range");
    ASSERT(moffx < 16, "moffx too large");
    ASSERT(moffy < 16, "moffy too large");
    
    printf("Graphics initialization successful\n");
    cleanup_sdl_compat();
    return 1;
}

// Test 4: Memory bounds checking
int test_memory_bounds(void) {
    printf("Testing memory bounds checking...\n");
    
    int sdl_result = init_sdl_compat();
    ASSERT(sdl_result == 0, "SDL initialization failed");
    
    gfxInit();
    
    // Test valid bounds
    char* test_ptr = logical + 1000;
    ASSERT_BOUNDS(test_ptr, logical, 64000, "test_ptr");
    
    // Test edge cases
    char* edge_ptr = logical + 63999;
    ASSERT_BOUNDS(edge_ptr, logical, 64000, "edge_ptr");
    
    printf("Memory bounds checking successful\n");
    cleanup_sdl_compat();
    return 1;
}

// Test 5: Mouse position bounds
int test_mouse_bounds(void) {
    printf("Testing mouse position bounds...\n");
    
    int sdl_result = init_sdl_compat();
    ASSERT(sdl_result == 0, "SDL initialization failed");
    
    gfxInit();
    
    // Check current mouse position bounds
    ASSERT(mx >= moffx, "mx less than moffx");
    ASSERT(my >= moffy, "my less than moffy");
    ASSERT(mx - moffx + 16 <= 320, "mouse extends beyond right edge");
    ASSERT(my - moffy + 16 <= 200, "mouse extends beyond bottom edge");
    
    // Test mouse pointer calculation
    char* mouse_ptr = logical + (my - moffy) * 320 + (mx - moffx);
    ASSERT_BOUNDS(mouse_ptr, logical, 64000, "mouse_ptr");
    
    printf("Mouse bounds checking successful\n");
    cleanup_sdl_compat();
    return 1;
}

// Test 6: Shape loading (if available)
int test_shape_loading(void) {
    printf("Testing shape loading...\n");
    
    int sdl_result = init_sdl_compat();
    ASSERT(sdl_result == 0, "SDL initialization failed");
    
    gfxInit();
    
    LoadShapes("gametiles", 0);
    printf("Shape loading completed (success/failure depends on data availability)\n");
    cleanup_sdl_compat();
    return 1;
}

// Test 7: Screen loading
int test_screen_loading(void) {
    printf("Testing screen loading...\n");
    
    int sdl_result = init_sdl_compat();
    ASSERT(sdl_result == 0, "SDL initialization failed");
    
    gfxInit();
    
    LoadShapes("gametiles", 0);
    printf("Shape loading completed\n");
    
    LoadScreen(0);
    printf("Screen loading completed without crash\n");
    
    cleanup_sdl_compat();
    return 1;
}

// Test 8: Palette setting
int test_palette_setting(void) {
    printf("Testing palette setting...\n");
    
    int sdl_result = init_sdl_compat();
    ASSERT(sdl_result == 0, "SDL initialization failed");
    
    gfxInit();
    
    SetPalette(2);
    printf("Palette setting completed without crash\n");
    
    cleanup_sdl_compat();
    return 1;
}

// Test 9: FadeIn pointer arithmetic
int test_fadein_arithmetic(void) {
    printf("Testing FadeIn pointer arithmetic...\n");
    
    int sdl_result = init_sdl_compat();
    ASSERT(sdl_result == 0, "SDL initialization failed");
    
    gfxInit();
    
    // Simulate the FadeIn loop calculations
    printf("Testing FadeIn loop pointer calculations...\n");
    
    for (int i = 0; i < 320; i += 17) {
        if (i >= 320) break;
        
        char* src = logical + i;
        char* dest = physical + i;
        
        printf("  i=%d: src=%p, dest=%p\n", i, src, dest);
        
        ASSERT_BOUNDS(src, logical, 64000, "src");
        ASSERT_BOUNDS(dest, physical, 64000, "dest");
        
        // Test inner loop calculations
        for (int y = 0; y < 200; y++) {
            char* inner_src = src + y * 320;
            char* inner_dest = dest + y * 320;
            
            if (y % 50 == 0) {
                printf("    y=%d: inner_src=%p, inner_dest=%p\n", y, inner_src, inner_dest);
            }
            
            ASSERT_BOUNDS(inner_src, logical, 64000, "inner_src");
            ASSERT_BOUNDS(inner_dest, physical, 64000, "inner_dest");
        }
    }
    
    printf("FadeIn pointer arithmetic test successful\n");
    cleanup_sdl_compat();
    return 1;
}

// Test 10: Mouse functions individually
int test_mouse_functions(void) {
    printf("Testing mouse functions individually...\n");
    
    int sdl_result = init_sdl_compat();
    ASSERT(sdl_result == 0, "SDL initialization failed");
    
    gfxInit();
    
    printf("Testing copymousebg...\n");
    copymousebg(logical);
    
    printf("Testing drawmouse...\n");
    drawmouse(logical);
    
    printf("Testing erasemouse...\n");
    erasemouse(logical);
    
    printf("Mouse functions test successful\n");
    cleanup_sdl_compat();
    return 1;
}

// Test 11: Full sequence up to FadeIn
int test_full_sequence(void) {
    printf("Testing full sequence up to FadeIn...\n");
    
    int sdl_result = init_sdl_compat();
    ASSERT(sdl_result == 0, "SDL initialization failed");
    printf("✓ SDL initialization\n");
    
    gfxInit();
    printf("✓ Graphics initialization\n");
    
    LoadShapes("gametiles", 0);
    printf("✓ Shape loading\n");
    
    LoadScreen(0);
    printf("✓ Screen loading\n");
    
    SetPalette(2);
    printf("✓ Palette setting\n");
    
    printf("All steps before FadeIn completed successfully\n");
    cleanup_sdl_compat();
    return 1;
}

// Test 12: Minimal FadeIn simulation
int test_fadein_simulation(void) {
    printf("Testing minimal FadeIn simulation...\n");
    
    int sdl_result = init_sdl_compat();
    ASSERT(sdl_result == 0, "SDL initialization failed");
    
    gfxInit();
    
    // Minimal simulation of FadeIn without the actual call
    printf("Simulating copymousebg(logical)...\n");
    copymousebg(logical);
    
    printf("Simulating drawmouse(logical)...\n");
    drawmouse(logical);
    
    printf("Simulating main copy loop...\n");
    for (int i = 0; i < 320; i += 17) {
        if (i >= 320) break;
        
        char* src = logical + i;
        char* dest = physical + i;
        
        for (int y = 0; y < 200; y++) {
            char* inner_src = src + y * 320;
            char* inner_dest = dest + y * 320;
            
            // Perform the actual memory copy that FadeIn does
            *inner_dest = *inner_src;
        }
    }
    
    printf("Simulating erasemouse(logical)...\n");
    erasemouse(logical);
    
    printf("FadeIn simulation completed successfully\n");
    cleanup_sdl_compat();
    return 1;
}

// Print test summary
void print_test_summary(void) {
    printf("\n==================================================\n");
    printf("TEST SUMMARY\n");
    printf("==================================================\n");
    
    int passed = 0, failed = 0, crashed = 0;
    
    for (int i = 0; i < test_count; i++) {
        char status = test_results[i].passed ? '+' : 
                     test_results[i].crashed ? 'X' : '!';
        
        printf("[%c] %s - %s\n", status, test_results[i].name, test_results[i].error);
        
        if (test_results[i].passed) passed++;
        else if (test_results[i].crashed) crashed++;
        else failed++;
    }
    
    printf("\nResults: %d passed, %d failed, %d crashed\n", passed, failed, crashed);
    
    if (crashed > 0) {
        printf("\nWARNING CRITICAL: %d tests crashed with segmentation faults!\n", crashed);
        printf("The first crashing test indicates where the segfault occurs.\n");
    }
}

// Main test runner
int main(int argc, char* argv[]) {
    printf("The Puzzle Pits Debug Test Suite\n");
    printf("================================\n");
    printf("Systematically testing components to isolate segfault...\n");
    
    // Run all tests in order of complexity
    RUN_TEST(test_memory_allocation);
    RUN_TEST(test_sdl_initialization);
    RUN_TEST(test_graphics_initialization);
    RUN_TEST(test_memory_bounds);
    RUN_TEST(test_mouse_bounds);
    RUN_TEST(test_shape_loading);
    RUN_TEST(test_screen_loading);
    RUN_TEST(test_palette_setting);
    RUN_TEST(test_fadein_arithmetic);
    RUN_TEST(test_mouse_functions);
    RUN_TEST(test_full_sequence);
    RUN_TEST(test_fadein_simulation);
    
    print_test_summary();
    
    return 0;
}