#!/bin/bash

# Build script for The Puzzle Pits using zig cc (no SDL2)

# Exit on error
set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}Building The Puzzle Pits with zig cc (no SDL2)...${NC}"

# Compiler settings
CC="zig cc"
CFLAGS="-Wall -O2 -I. -x c"
LDFLAGS=""

# Source files
COMMON_SOURCES=(
    "DIG.C"
    "DMA.C"
    "ENV.C"
    "FILE.C"
    "GFX.C"
    "LOAD.C"
    "SETMODE.C"
    "gfxa_stub.c"
)

DEMO_SOURCES=(
    "DEMO.C"
)

PITS_SOURCES=(
    "PITS.C"
)

# Object files directory
OBJDIR="build"
mkdir -p "$OBJDIR"

# Compile each source file
echo -e "${GREEN}Compiling common source files...${NC}"
COMMON_OBJECTS=()
for src in "${COMMON_SOURCES[@]}"; do
    obj="$OBJDIR/$(basename ${src%.*}).o"
    echo "  Compiling $src -> $obj"
    $CC $CFLAGS -c "$src" -o "$obj"
    COMMON_OBJECTS+=("$obj")
done

echo -e "${GREEN}Compiling main program files...${NC}"
# Compile DEMO
for src in "${DEMO_SOURCES[@]}"; do
    obj="$OBJDIR/$(basename ${src%.*}).o"
    echo "  Compiling $src -> $obj"
    $CC $CFLAGS -c "$src" -o "$obj"
done

# Compile PITS
for src in "${PITS_SOURCES[@]}"; do
    obj="$OBJDIR/$(basename ${src%.*}).o"
    echo "  Compiling $src -> $obj"
    $CC $CFLAGS -c "$src" -o "$obj"
done

# Create output directory if it doesn't exist
mkdir -p compiled-version

# Link executables
echo -e "${GREEN}Linking executables...${NC}"

# Build DEMO executable
echo "  Linking demo..."
DEMO_OBJS=(
    "$OBJDIR/DEMO.o"
    "${COMMON_OBJECTS[@]}"
)
$CC $LDFLAGS "${DEMO_OBJS[@]}" -o "compiled-version/demo"

# Build PITS executable
echo "  Linking pits..."
PITS_OBJS=(
    "$OBJDIR/PITS.o"
    "${COMMON_OBJECTS[@]}"
)
$CC $LDFLAGS "${PITS_OBJS[@]}" -o "compiled-version/pits"

# Copy assets if they exist
echo -e "${GREEN}Checking for assets...${NC}"
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
echo "Note: This is a no-SDL build - graphics and input are stubbed out"
echo "Starting The Puzzle Pits..."
./pits
EOF
chmod +x compiled-version/run_pits.sh

cat > compiled-version/run_demo.sh << 'EOF'
#!/bin/bash
# Run The Puzzle Pits Demo
cd "$(dirname "$0")"
echo "Note: This is a no-SDL build - graphics and input are stubbed out"
echo "Starting The Puzzle Pits Demo..."
./demo
EOF
chmod +x compiled-version/run_demo.sh

echo -e "${GREEN}Build complete!${NC}"
echo ""
echo "Executables created:"
echo "  - compiled-version/pits"
echo "  - compiled-version/demo"
echo ""
echo -e "${YELLOW}Note: This is a no-SDL build with stubbed graphics/input${NC}"
echo -e "${YELLOW}      For full functionality, use the SDL2 build instead${NC}"
echo ""
echo "To run the game:"
echo "  cd compiled-version && ./run_pits.sh"
echo ""
echo "To run the demo:"
echo "  cd compiled-version && ./run_demo.sh"
echo ""
if [ ! -d "compiled-version/PITS95" ]; then
    echo -e "${YELLOW}Note: Game assets were not found. Make sure to copy the PITS95 directory${NC}"
    echo -e "${YELLOW}      with all game data files to compiled-version/PITS95${NC}"
fi