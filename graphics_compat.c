/* graphics_compat.c
 * Graphics compatibility layer for missing drawing functions
 * Implements DOS graphics functions for SDL compatibility
 */

#include "compat.h"
#include "GFX.H"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// External variables from the main graphics system
extern char *logical;
extern char *physical;
extern char *shapes[300];  // MAXSHAPE from GFX.H
extern uint8_t *vga_memory_buffer;

// Graphics drawing functions for DOS compatibility

// Blit8 - 8-bit sprite blitting function
void Blit8(int bmap, int x, int y, int image) {
    if (!logical) {
        printf("WARNING: Blit8 called with NULL logical buffer\n");
        return;
    }
    
    if (image >= 300 || !shapes[image]) {
        printf("WARNING: Blit8 called with invalid image %d\n", image);
        return;
    }
    
    // Basic bounds checking
    if (x < 0 || y < 0 || x >= 320 || y >= 200) {
        return;  // Off-screen, skip
    }
    
    // For now, implement as a simple filled rectangle
    // This is a stub implementation that can be improved later
    char *dest = logical + (y * 320 + x);
    int width = 16;  // Default sprite width
    int height = 16; // Default sprite height
    
    // Simple pattern fill for testing
    for (int row = 0; row < height && (y + row) < 200; row++) {
        for (int col = 0; col < width && (x + col) < 320; col++) {
            dest[row * 320 + col] = (image % 256);  // Use image number as color
        }
    }
}

// Blit16 - 16-bit sprite blitting
void Blit16(WORD bmap, WORD x, WORD y, WORD image) {
    Blit8(bmap, x, y, image);  // Delegate to Blit8 for now
}

// Blit24 - 24-bit sprite blitting  
void Blit24(WORD bmap, WORD x, WORD y, WORD image) {
    Blit8(bmap, x, y, image);  // Delegate to Blit8 for now
}

// Blit - General sprite blitting
void Blit(WORD bmap, WORD x, WORD y, WORD image) {
    Blit8(bmap, x, y, image);  // Delegate to Blit8 for now
}

// BlitMask16 - Masked 16-bit sprite blitting
void BlitMask16(WORD bmap, WORD x, WORD y, WORD image) {
    // For masked blitting, we would skip transparent pixels
    // For now, just call regular blit
    Blit16(bmap, x, y, image);
}

// BlitMask24 - Masked 24-bit sprite blitting
void BlitMask24(WORD bmap, WORD x, WORD y, WORD image) {
    BlitMask16(bmap, x, y, image);
}

// BlitMask - General masked sprite blitting
void BlitMask(WORD bmap, WORD x, WORD y, WORD image) {
    BlitMask16(bmap, x, y, image);
}

// LittleTile - Small tile blitting
void LittleTile(WORD bmap, WORD x, WORD y, WORD image) {
    if (!logical) return;
    
    // Little tiles are typically 8x8 pixels
    char *dest = logical + (y * 320 + x);
    int width = 8;
    int height = 8;
    
    // Bounds checking
    if (x >= 320 || y >= 200) return;
    
    // Simple pattern for little tiles
    for (int row = 0; row < height && (y + row) < 200; row++) {
        for (int col = 0; col < width && (x + col) < 320; col++) {
            dest[row * 320 + col] = ((image + row + col) % 256);
        }
    }
}

// OwnBlit - Take ownership of blitting operations
void OwnBlit(void) {
    // This would typically set up custom blitting routines
    // For SDL compatibility, this is a no-op
}

// DisownBlit - Release ownership of blitting operations  
void DisownBlit(void) {
    // This would typically restore standard blitting routines
    // For SDL compatibility, this is a no-op
}

// Plot - Plot a single pixel
void Plot(WORD x, WORD y, BYTE c, char *vscreen) {
    if (!vscreen) return;
    if (x >= 320 || y >= 200) return;
    
    vscreen[y * 320 + x] = c;
}

// Additional graphics utility functions

// Frame - Draw a rectangular frame
void Frame(WORD x, WORD y, WORD width, WORD height, WORD c1, WORD c2) {
    if (!logical) return;
    
    // Draw top and bottom lines
    for (WORD i = 0; i < width; i++) {
        if (x + i < 320) {
            if (y < 200) logical[y * 320 + x + i] = c1;
            if (y + height - 1 < 200) logical[(y + height - 1) * 320 + x + i] = c2;
        }
    }
    
    // Draw left and right lines
    for (WORD i = 0; i < height; i++) {
        if (y + i < 200) {
            if (x < 320) logical[(y + i) * 320 + x] = c1;
            if (x + width - 1 < 320) logical[(y + i) * 320 + x + width - 1] = c2;
        }
    }
}

// FilledFrame - Draw a filled rectangular frame
void FilledFrame(WORD x, WORD y, WORD width, WORD height, WORD c1, WORD c2, WORD c3) {
    if (!logical) return;
    
    // Fill the interior
    for (WORD row = 1; row < height - 1; row++) {
        if (y + row < 200) {
            for (WORD col = 1; col < width - 1; col++) {
                if (x + col < 320) {
                    logical[(y + row) * 320 + x + col] = c3;
                }
            }
        }
    }
    
    // Draw the frame
    Frame(x, y, width, height, c1, c2);
}

// Boxf - Draw a filled box
void Boxf(WORD x, WORD y, WORD width, WORD height, WORD c1) {
    if (!logical) return;
    
    for (WORD row = 0; row < height; row++) {
        if (y + row < 200) {
            for (WORD col = 0; col < width; col++) {
                if (x + col < 320) {
                    logical[(y + row) * 320 + x + col] = c1;
                }
            }
        }
    }
}

// Clear - Clear a screen buffer
void Clear(WORD n) {
    char *buffer = (n == 0) ? logical : physical;  // 0 = LOGIC, 1 = PHYSIC
    if (buffer) {
        memset(buffer, 0, 320 * 200);
    }
}

// ScreenSwap - Swap logical and physical screens
void ScreenSwap(void) {
    if (logical && physical) {
        // Copy logical to physical, then swap via SDL
        memcpy(physical, logical, 320 * 200);
        screenswap(physical);  // Call SDL function
    }
}

// ScreenCopy - Copy logical to physical
void ScreenCopy(void) {
    if (logical && physical) {
        memcpy(physical, logical, 320 * 200);
    }
}

// ScreenCopySidebar - Copy logical to physical (sidebar version)
void ScreenCopySidebar(void) {
    // This would typically copy just a sidebar region
    // For now, implement as full screen copy
    ScreenCopy();
}

// FreeShapes - Free shape memory
void FreeShapes(void) {
    // This would free dynamically allocated shape data
    // For now, this is a no-op since shapes are handled elsewhere
    printf("DEBUG: FreeShapes called\n");
}

// PlaySound - Play a sound effect
void PlaySound(int n) {
    // Sound effects would be played through SDL_mixer
    // For now, this is a stub
    printf("DEBUG: PlaySound(%d) called\n", n);
}

// Text drawing function
void Txt(WORD x, WORD y, const CHAR *str, WORD len) {
    // This would draw text using a font
    // For now, this is a stub that draws simple rectangles for characters
    if (!logical || !str) return;
    
    for (WORD i = 0; i < len && str[i] && x + i * 8 < 320; i++) {
        // Draw a simple 8x8 rectangle for each character
        for (int row = 0; row < 8 && y + row < 200; row++) {
            for (int col = 0; col < 8 && x + i * 8 + col < 320; col++) {
                logical[(y + row) * 320 + x + i * 8 + col] = 255;  // White color
            }
        }
    }
}

// TxtLength - Calculate text length in pixels
WORD TxtLength(const CHAR *str, WORD len) {
    // Simple calculation: 8 pixels per character
    return len * 8;
}

// SetColor - Set drawing color
void SetColor(WORD n) {
    // This would set the current drawing color
    // For now, this is a stub
    static WORD current_color = 255;
    current_color = n;
}