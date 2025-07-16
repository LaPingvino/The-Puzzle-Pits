#!/bin/bash

# Build script for The Puzzle Pits using zig cc with optional SDL2

# Exit on error
set -e

# Default to no-SDL build (stub compatibility layer)
USE_SDL=false

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --sdl)
            USE_SDL=true
            shift
            ;;
        --no-sdl)
            USE_SDL=false
            shift
            ;;
        -h|--help)
            echo "Usage: $0 [--sdl|--no-sdl]"
            echo ""
            echo "Options:"
            echo "  --sdl     Build with SDL2 support (requires SDL2 development libraries)"
            echo "  --no-sdl  Build with stub compatibility layer (default)"
            echo "  -h|--help Show this help message"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            echo "Use --help for usage information"
            exit 1
            ;;
    esac
done

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

if [ "$USE_SDL" = true ]; then
    echo -e "${GREEN}Building The Puzzle Pits with zig cc and SDL2...${NC}"
else
    echo -e "${GREEN}Building The Puzzle Pits with zig cc (stub compatibility layer)...${NC}"
fi

# Compiler settings
CC="zig cc"
CFLAGS="-xc -Wall -O2 -I."
LDFLAGS=""

# Set up SDL2 flags if needed
if [ "$USE_SDL" = true ]; then
    CFLAGS="$CFLAGS -DUSE_SDL"
    
    # Get SDL2 flags from pkg-config
    if command -v pkg-config >/dev/null 2>&1; then
        SDL2_CFLAGS=$(pkg-config --cflags sdl2)
        SDL2_LIBS=$(pkg-config --libs sdl2)
        echo -e "${GREEN}Found SDL2 via pkg-config${NC}"
    else
        echo -e "${YELLOW}Warning: pkg-config not found, using default SDL2 flags${NC}"
        SDL2_CFLAGS="-I/usr/include/SDL2"
        SDL2_LIBS="-lSDL2"
    fi
    CFLAGS="$CFLAGS $SDL2_CFLAGS"
    LDFLAGS="$LDFLAGS $SDL2_LIBS"
else
    echo -e "${GREEN}Using stub compatibility layer (no SDL2 required)${NC}"
    SDL2_CFLAGS=""
    SDL2_LIBS=""
fi

# Source files
COMMON_SOURCES=(
    "DEMO.C"
    "DIG.C"
    "DMA.C"
    "ENV.C"
    "FILE.C"
    "GFX.C"
    "LOAD.C"
    "PITS.C"
    "gfxa_stub.c"
)

# Conditional source files
if [ "$USE_SDL" = true ]; then
    SOURCES=("${COMMON_SOURCES[@]}" "sdl_compat.c")
    COMPAT_OBJ="$OBJDIR/sdl_compat.o"
else
    SOURCES=("${COMMON_SOURCES[@]}")
    COMPAT_OBJ=""
fi

# Object files directory
OBJDIR="build"
mkdir -p "$OBJDIR"

# Compile each source file
echo -e "${GREEN}Compiling source files...${NC}"
OBJECTS=()
for src in "${SOURCES[@]}"; do
    obj="$OBJDIR/$(basename ${src%.*}).o"
    echo "  Compiling $src -> $obj"
    $CC $CFLAGS -c "$src" -o "$obj"
    OBJECTS+=("$obj")
done

# Create output directory if it doesn't exist
mkdir -p compiled-version

# Link executables
echo -e "${GREEN}Linking executables...${NC}"

# Build DEMO executable
echo "  Linking demo..."
DEMO_OBJS=(
    "$OBJDIR/DEMO.o"
    "$OBJDIR/DIG.o"
    "$OBJDIR/DMA.o"
    "$OBJDIR/ENV.o"
    "$OBJDIR/FILE.o"
    "$OBJDIR/GFX.o"
    "$OBJDIR/LOAD.o"
    "$OBJDIR/gfxa_stub.o"
)
if [ -n "$COMPAT_OBJ" ]; then
    DEMO_OBJS+=("$COMPAT_OBJ")
fi
$CC "${DEMO_OBJS[@]}" $LDFLAGS -o "compiled-version/demo"

# Build PITS executable
echo "  Linking pits..."
PITS_OBJS=(
    "$OBJDIR/PITS.o"
    "$OBJDIR/DIG.o"
    "$OBJDIR/DMA.o"
    "$OBJDIR/ENV.o"
    "$OBJDIR/FILE.o"
    "$OBJDIR/GFX.o"
    "$OBJDIR/LOAD.o"
    "$OBJDIR/gfxa_stub.o"
)
if [ -n "$COMPAT_OBJ" ]; then
    PITS_OBJS+=("$COMPAT_OBJ")
fi
$CC "${PITS_OBJS[@]}" $LDFLAGS -o "compiled-version/pits"

# Copy assets if they exist
echo -e "${GREEN}Copying assets...${NC}"
if [ -d "compiled-version/PITS95" ]; then
    echo "  Assets already exist in compiled-version/PITS95"
else
    echo -e "${YELLOW}  Warning: No assets found in compiled-version/PITS95${NC}"
    echo -e "${YELLOW}  You may need to copy game assets manually${NC}"
fi

# Create a simple run script
cat > compiled-version/run_pits.sh << 'EOF'
#!/bin/bash
# Run The Puzzle Pits
cd "$(dirname "$0")"
./pits
EOF
chmod +x compiled-version/run_pits.sh

cat > compiled-version/run_demo.sh << 'EOF'
#!/bin/bash
# Run The Puzzle Pits Demo
cd "$(dirname "$0")"
./demo
EOF
chmod +x compiled-version/run_demo.sh

echo -e "${GREEN}Build complete!${NC}"
echo ""
echo "Build configuration:"
if [ "$USE_SDL" = true ]; then
    echo "  - SDL2 support: ENABLED"
    echo "  - Graphics: Real SDL2 implementation"
    echo "  - Audio: Real SDL2 implementation"
else
    echo "  - SDL2 support: DISABLED (using stub compatibility layer)"
    echo "  - Graphics: Stub implementation (no display)"
    echo "  - Audio: Stub implementation (no sound)"
fi
echo ""
echo "Executables created:"
echo "  - compiled-version/pits"
echo "  - compiled-version/demo"
echo ""
echo "To run the game:"
echo "  cd compiled-version && ./run_pits.sh"
echo ""
echo "To run the demo:"
echo "  cd compiled-version && ./run_demo.sh"
echo ""
if [ "$USE_SDL" = false ]; then
    echo -e "${YELLOW}Note: Built with stub compatibility layer. The game will run but won't${NC}"
    echo -e "${YELLOW}      display graphics or play sound. Use --sdl flag for full functionality.${NC}"
    echo ""
fi
if [ ! -d "compiled-version/PITS95" ]; then
    echo -e "${YELLOW}Note: Game assets were not found. Make sure to copy the PITS95 directory${NC}"
    echo -e "${YELLOW}      with all game data files to compiled-version/PITS95${NC}"
fi