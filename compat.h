#ifndef COMPAT_H
#define COMPAT_H

// DOS conio.h compatibility
#define conio_h_included

/* Common compatibility header
 * 
 * This header provides a compatibility layer for DOS-specific code.
 * It can be compiled with or without SDL2 support.
 * 
 * To compile with SDL2:
 *   zig cc -DUSE_SDL ...
 * 
 * To compile without SDL2 (stub implementation):
 *   zig cc ...
 * 
 * To disable mouse support (keyboard only):
 *   zig cc -DDISABLE_MOUSE ...
 */

// Mouse support configuration
#ifndef DISABLE_MOUSE
  #define MOUSE_ENABLED 1
#else
  #define MOUSE_ENABLED 0
#endif

#ifdef USE_SDL
  /* Use real SDL2 implementation */
  #include "sdl_compat.h"
#else
  /* Use stub implementation for testing without SDL2 */
  #include "nosdl_compat.h"
#endif



// DOS type definitions for compatibility
typedef unsigned char BYTE;
typedef unsigned char UBYTE;
typedef int WORD;
typedef unsigned int UWORD;
typedef long LONG;
typedef unsigned long ULONG;
typedef char CHAR;
#ifndef INT8
    #define INT8 signed char
#endif


// Minimal function declarations for essential DOS replacements
int init_minimal_dos(void);
void cleanup_minimal_dos(void);

// Master initialization function
static inline int init_all_compat(void) {
    return init_minimal_dos();
}

// Master cleanup function
static inline void cleanup_all_compat(void) {
    cleanup_minimal_dos();
}

#endif // COMPAT_H