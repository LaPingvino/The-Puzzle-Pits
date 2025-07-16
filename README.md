# The Puzzle Pits

A classic DOS puzzle game from 1995, modernized to compile and run on contemporary systems.

## About

The Puzzle Pits is a puzzle-platform game originally developed in 1995 for DOS. This repository contains the original source code along with a comprehensive compatibility layer that allows the game to be compiled and run on modern operating systems using the Zig compiler.

## Current Status

✅ **COMPILATION SUCCESS** - The game now compiles cleanly with modern tools!

### What Works
- ✅ Complete compilation with Zig CC
- ✅ DOS compatibility layer implemented
- ✅ Memory management abstractions (farmalloc/farfree → malloc/free)
- ✅ Port I/O and interrupt stubs
- ✅ Flexible build system (SDL or stub builds)
- ✅ All header dependencies resolved
- ✅ Function signature compatibility fixed

### In Progress
- 🔄 SDL2 graphics implementation (partial)
- 🔄 Runtime testing and debugging
- 🔄 Audio system integration

### Known Limitations
- Runtime may crash with stub implementation (expected behavior)
- SDL implementation incomplete - needs finishing for full functionality
- Game assets not included (requires original PITS95 directory)

## Quick Start

### Prerequisites
- [Zig compiler](https://ziglang.org/download/) (tested with 0.11+)
- Optional: SDL2 development libraries (for graphics/audio)

### Build Options

#### Option 1: Stub Build (No Dependencies)
```bash
git clone <repository>
cd The-Puzzle-Pits
./build.sh --no-sdl
```

This creates executables that run but don't display graphics (useful for testing compilation).

#### Option 2: SDL Build (Full Functionality - When Complete)
```bash
# Install SDL2 development libraries first
# Ubuntu/Debian: sudo apt install libsdl2-dev
# Fedora: sudo dnf install SDL2-devel
# macOS: brew install sdl2

./build.sh --sdl
```

### Running the Game
```bash
cd compiled-version
./run_pits.sh    # Main game
./run_demo.sh    # Demo version
```

## Development Environment

### Using Nix (Recommended)
```bash
nix develop                    # SDL development shell
nix develop .#nosdl           # No-SDL development shell
```

### Manual Setup
1. Install Zig compiler
2. Optionally install SDL2 development libraries
3. Run build script

## Project Structure

```
The-Puzzle-Pits/
├── *.C, *.H              # Original game source files
├── compat.h               # Main compatibility header
├── nosdl_compat.h         # Stub implementations (no SDL)
├── sdl_compat.h/.c        # SDL2 implementations (partial)
├── gfxa_stub.c            # Assembly function replacements
├── build.sh               # Build script
├── flake.nix              # Nix development environment
├── build/                 # Compiled object files
├── compiled-version/      # Final executables
└── compiled-version/PITS95/  # Game assets (not included)
```

## Technical Details

### Compatibility Layer
This project implements a comprehensive DOS-to-modern compatibility layer:

- **Memory Management**: `farmalloc`/`farfree` → `malloc`/`free`
- **Port I/O**: `inp`/`outp` → stubbed functions
- **DOS Interrupts**: interrupt handlers → stub implementations  
- **Video**: VGA mode 13h → SDL2 or stub buffer
- **Input**: DOS keyboard/mouse → SDL2 or stub events
- **File I/O**: DOS paths → POSIX paths with compatibility

### Build System
The `build.sh` script supports two build modes:

1. **Stub Mode** (`--no-sdl`): No external dependencies, functions execute but no I/O
2. **SDL Mode** (`--sdl`): Full SDL2 integration for graphics and audio

### Original Architecture
- Written in C for DOS (1995)
- Used DOS-specific libraries (`conio.h`, interrupt handlers)
- Direct VGA memory access
- Assembly routines for graphics primitives
- Sound Blaster audio support

## File Descriptions

### Core Game Files
- `PITS.C` - Main game logic and menu system
- `DEMO.C` - Demo mode implementation  
- `GFX.C` - Graphics and display routines
- `FILE.C` - File I/O and utilities
- `DIG.C` - Digital audio system
- `LOAD.C` - Asset loading routines
- `ENV.C` - Environment variable handling
- `DMA.C` - Direct Memory Access for audio

### Headers
- `GFX.H` - Graphics function declarations
- `FILE.H` - Core type definitions and file functions
- `TILEDEFS.H` - Game constants and tile definitions
- `OLDSOUND.H` - Audio system structures
- `ENV.H` - Environment handling
- `SB.H`, `DIG.H`, `DMA.H` - Audio subsystem headers

### Compatibility Layer
- `compat.h` - Main compatibility interface
- `nosdl_compat.h` - Stub implementations
- `sdl_compat.h/.c` - SDL2 implementations  
- `gfxa_stub.c` - Assembly function replacements

## Known Issues

### Compilation Warnings
- Unused variables (legacy code artifacts)
- Array subscript type warnings
- Unknown pragma directives from DOS compiler

### Runtime Issues
- Stub build executables may crash (expected - no real I/O)
- SDL build needs completion for stable execution
- Asset loading paths may need adjustment

### Missing Components
- Game asset files (levels, graphics, sounds)
- Complete SDL2 graphics implementation
- Audio mixing and sound effects
- Input mapping configuration

## Contributing

### Priority Tasks
1. **Complete SDL2 Implementation**
   - Finish graphics rendering pipeline
   - Implement audio mixing
   - Add input event handling

2. **Runtime Debugging**
   - Test execution with SDL build
   - Fix crashes and compatibility issues
   - Verify asset loading

3. **Code Cleanup**
   - Address compilation warnings
   - Optimize compatibility layer
   - Add error handling

### Development Workflow
1. Fork the repository
2. Create a feature branch
3. Test both build modes: `./build.sh --no-sdl` and `./build.sh --sdl`
4. Ensure no new compilation errors
5. Submit pull request

### Coding Standards
- Maintain compatibility with original C style
- Add comments for new compatibility functions
- Preserve original game logic wherever possible
- Test changes with both build configurations

## Historical Context

This is a preservation project for a 1995 DOS game. The original used:
- Borland C++ compiler
- DOS-specific libraries and system calls
- Direct hardware access (VGA, Sound Blaster)
- Assembly language optimizations
- 16-bit memory model with far pointers

The modernization maintains the original game logic while replacing system-dependent code with portable alternatives.

## License

Original game code copyright (c) 1995 Abe Pralle.
Compatibility layer and build system modifications for preservation and educational purposes.

## Acknowledgments

- Original game by Abe Pralle (1995)
- DOS-to-modern porting by compatibility layer development
- Community preservation efforts for classic games

---

**Status**: Compilation successful, runtime implementation in progress.  
**Last Updated**: December 2024