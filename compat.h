#ifndef COMPAT_H
#define COMPAT_H

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
 */

#ifdef USE_SDL
  /* Use real SDL2 implementation */
  #include "sdl_compat.h"
#else
  /* Use stub implementation for testing without SDL2 */
  #include "nosdl_compat.h"
#endif

#endif /* COMPAT_H */