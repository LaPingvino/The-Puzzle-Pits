/* timer_compat.c
 * Comprehensive timer and timing compatibility system
 * Provides DOS-compatible timing functions for modern systems
 */

#define USE_FULL_COMPAT_IMPL

#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>
#ifndef _WIN32
#include <sched.h>
#endif
#ifdef USE_SDL
#include <SDL2/SDL.h>
#endif
#include "compat.h"

// DOS timer constants
#define DOS_TIMER_FREQ 18.2         // DOS timer frequency in Hz
#define DOS_TICK_MS 54.945          // Milliseconds per DOS tick (1000/18.2)
#define FRAMEDELAY 3                // Frame delay in DOS ticks
#define MAX_TIMER_VALUE 216262      // Maximum timer value before rollover

// Static variables for timing
static uint32_t timer_start_time = 0;
static uint32_t timer_initialized = 0;
static uint32_t frame_target_time = 0;
static uint32_t last_frame_time = 0;

// Performance timing
static uint32_t total_frames = 0;
static uint32_t last_fps_time = 0;
static float current_fps = 0.0f;

/*
 * Internal timing functions
 */

// Get current time in milliseconds
static uint32_t get_current_time_ms(void) {
    #ifdef USE_SDL
    return SDL_GetTicks();
    #else
    // Fallback to system clock
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
        return (uint32_t)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
    }
    // Fallback to standard clock
    return (uint32_t)(clock() * 1000 / CLOCKS_PER_SEC);
    #endif
}

// Convert milliseconds to DOS ticks
static uint32_t ms_to_ticks(uint32_t ms) {
    return (uint32_t)(ms / DOS_TICK_MS);
}

// Convert DOS ticks to milliseconds
static uint32_t ticks_to_ms(uint32_t ticks) {
    return (uint32_t)(ticks * DOS_TICK_MS);
}

/*
 * Timer initialization and cleanup
 */

// Initialize timer system
void init_timer_compat(void) {
    if (!timer_initialized) {
        timer_start_time = get_current_time_ms();
        frame_target_time = 0;
        last_frame_time = 0;
        total_frames = 0;
        last_fps_time = timer_start_time;
        current_fps = 0.0f;
        timer_initialized = 1;
    }
}

// Cleanup timer system
void cleanup_timer_compat(void) {
    timer_initialized = 0;
}

/*
 * Core timing functions
 */

// get_ticks - Get current timer ticks (18.2 Hz like DOS)
uint32_t get_ticks(void) {
    if (!timer_initialized) {
        init_timer_compat();
    }
    
    uint32_t current_ms = get_current_time_ms();
    uint32_t elapsed_ms = current_ms - timer_start_time;
    uint32_t ticks = ms_to_ticks(elapsed_ms);
    
    // Handle timer rollover like DOS
    if (ticks > MAX_TIMER_VALUE) {
        // Reset timer start time
        timer_start_time = current_ms;
        ticks = 0;
    }
    
    return ticks;
}

// Timer - Main timer function used throughout the game
ULONG Timer(void) {
    return (ULONG)get_ticks();
}

// delay - Delay for specified milliseconds
void delay(uint32_t ms) {
    #ifdef USE_SDL
    SDL_Delay(ms);
    #else
    // Fallback delay implementation
    uint32_t start = get_current_time_ms();
    uint32_t target = start + ms;
    
    while (get_current_time_ms() < target) {
        // Busy wait - not ideal but works for compatibility
        #ifdef _WIN32
        Sleep(1);
        #else
        struct timespec ts = {0, 1000000}; // 1ms
        nanosleep(&ts, NULL);
        #endif
    }
    #endif
}

// Pause - Pause for specified DOS ticks
void Pause(LONG n) {
    if (n <= 0) return;
    
    uint32_t ticks = (uint32_t)n;
    uint32_t start_ticks = get_ticks();
    uint32_t target_ticks = start_ticks + ticks;
    
    // Handle potential rollover
    if (target_ticks > MAX_TIMER_VALUE) {
        target_ticks = target_ticks - MAX_TIMER_VALUE;
        // Wait for rollover first
        while (get_ticks() >= start_ticks) {
            delay(1);
        }
    }
    
    // Wait until target time
    while (get_ticks() < target_ticks) {
        delay(1);
    }
}

/*
 * Frame timing functions
 */

// WasteTime - Frame rate limiting function
void WasteTime(void) {
    if (!timer_initialized) {
        init_timer_compat();
    }
    
    uint32_t current_time = get_ticks();
    
    // Wait until enough time has passed since last frame
    while (current_time < frame_target_time && 
           (frame_target_time - current_time) <= FRAMEDELAY) {
        
        // Process input/events during wait
        extern void CheckMouse(void);
        CheckMouse();
        
        // Small delay to prevent busy waiting
        delay(1);
        current_time = get_ticks();
    }
    
    // Set target for next frame
    frame_target_time = current_time + FRAMEDELAY;
    
    // Handle timer rollover
    if (frame_target_time > MAX_TIMER_VALUE) {
        frame_target_time = FRAMEDELAY;
    }
    
    last_frame_time = current_time;
    total_frames++;
    
    // Update FPS counter every second
    if (current_time - last_fps_time >= ms_to_ticks(1000)) {
        uint32_t elapsed_ticks = current_time - last_fps_time;
        float elapsed_seconds = elapsed_ticks * DOS_TICK_MS / 1000.0f;
        current_fps = total_frames / elapsed_seconds;
        total_frames = 0;
        last_fps_time = current_time;
    }
}

// Get current frame rate
float get_fps(void) {
    return current_fps;
}

// Reset frame timing
void reset_frame_timing(void) {
    frame_target_time = get_ticks() + FRAMEDELAY;
    last_frame_time = get_ticks();
}

/*
 * High-precision timing functions
 */

// Get high-precision timer (microseconds)
uint64_t get_precise_time_us(void) {
    #ifdef USE_SDL
    return SDL_GetPerformanceCounter() * 1000000 / SDL_GetPerformanceFrequency();
    #else
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
        return (uint64_t)ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
    }
    return get_current_time_ms() * 1000;
    #endif
}

// Precise delay in microseconds
void delay_precise_us(uint64_t microseconds) {
    uint64_t start = get_precise_time_us();
    uint64_t target = start + microseconds;
    
    // Use system sleep for large delays
    if (microseconds > 10000) { // 10ms
        delay((uint32_t)(microseconds / 1000));
        return;
    }
    
    // Busy wait for small delays
    while (get_precise_time_us() < target) {
        // Yield CPU briefly
        #ifdef _WIN32
        SwitchToThread();
        #else
        #ifndef __APPLE__
        sched_yield();
        #else
        usleep(1);
        #endif
        #endif
    }
}

/*
 * Performance measurement functions
 */

// Performance timer structure
typedef struct {
    uint64_t start_time;
    uint64_t total_time;
    uint32_t call_count;
    const char *name;
} perf_timer_t;

// Performance timers
static perf_timer_t perf_timers[16];
static int num_perf_timers = 0;

// Start performance measurement
int start_perf_timer(const char *name) {
    if (num_perf_timers >= 16) return -1;
    
    int timer_id = num_perf_timers++;
    perf_timers[timer_id].name = name;
    perf_timers[timer_id].start_time = get_precise_time_us();
    perf_timers[timer_id].total_time = 0;
    perf_timers[timer_id].call_count = 0;
    
    return timer_id;
}

// End performance measurement
void end_perf_timer(int timer_id) {
    if (timer_id < 0 || timer_id >= num_perf_timers) return;
    
    uint64_t end_time = get_precise_time_us();
    perf_timers[timer_id].total_time += end_time - perf_timers[timer_id].start_time;
    perf_timers[timer_id].call_count++;
}

// Print performance statistics
void print_perf_stats(void) {
    printf("\n=== Performance Statistics ===\n");
    for (int i = 0; i < num_perf_timers; i++) {
        perf_timer_t *timer = &perf_timers[i];
        if (timer->call_count > 0) {
            double avg_time = (double)timer->total_time / timer->call_count;
            printf("%s: %u calls, %.2f μs avg, %.2f ms total\n",
                   timer->name, timer->call_count, avg_time, 
                   timer->total_time / 1000.0);
        }
    }
    printf("Current FPS: %.1f\n", current_fps);
    printf("=============================\n\n");
}

/*
 * Legacy DOS compatibility functions
 */

// DOS-style timer interrupt simulation
static volatile uint32_t timer_interrupt_counter = 0;

void timer_interrupt_handler(void) {
    timer_interrupt_counter++;
}

uint32_t get_timer_interrupt_count(void) {
    return timer_interrupt_counter;
}

// Simulate DOS timer interrupt at 18.2 Hz
void update_timer_interrupt(void) {
    static uint32_t last_interrupt_time = 0;
    uint32_t current_time = get_current_time_ms();
    
    if (current_time - last_interrupt_time >= DOS_TICK_MS) {
        timer_interrupt_handler();
        last_interrupt_time = current_time;
    }
}

/*
 * Debug and utility functions
 */

// Get timing statistics
void get_timing_stats(uint32_t *current_ticks, uint32_t *frame_time, float *fps) {
    if (current_ticks) *current_ticks = get_ticks();
    if (frame_time) *frame_time = last_frame_time;
    if (fps) *fps = current_fps;
}

// Print current timing state
void print_timing_debug(void) {
    uint32_t ticks = get_ticks();
    uint32_t ms = get_current_time_ms();
    
    printf("Timer Debug: ticks=%u, ms=%u, fps=%.1f, target=%u\n",
           ticks, ms, current_fps, frame_target_time);
}

// Check if timer system is healthy
int check_timer_health(void) {
    static uint32_t last_check_time = 0;
    uint32_t current_time = get_ticks();
    
    // Check if timer is advancing
    if (current_time == last_check_time) {
        return 0; // Timer not advancing
    }
    
    last_check_time = current_time;
    return 1; // Timer is healthy
}

/*
 * Initialization and cleanup wrappers
 */

// Initialize all timing systems
int init_all_timing(void) {
    init_timer_compat();
    reset_frame_timing();
    return timer_initialized ? 0 : -1;
}

// Cleanup all timing systems
void cleanup_all_timing(void) {
    cleanup_timer_compat();
    
    // Print final performance stats in debug builds
    #ifdef DEBUG
    print_perf_stats();
    #endif
}