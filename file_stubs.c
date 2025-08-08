/* file_stubs.c
 * Simple stub implementations for missing FILE.C dependencies
 * These allow FILE.C to compile independently for testing
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// Stub implementation for Pause function
// In the real game, this would be a timing function
void Pause(long n) {
    // Convert from game ticks to milliseconds (assuming 18.2 Hz timer)
    // n ticks * (1000 ms / 18.2 ticks) = n * 54.95 ms
    if (n > 0) {
        usleep((unsigned int)(n * 54945)); // usleep takes microseconds
    }
}

// Stub implementation for cleanExit function
// In the real game, this would clean up graphics and restore DOS state
void cleanExit(int retval) {
    printf("cleanExit called with code %d\n", retval);
    exit(retval);
}