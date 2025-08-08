# The Puzzle Pits - Unified Build System
# All builds delegate to the Zig build system for consistency

.PHONY: all sdl nosdl clean help setup download-data run test debug info
.DEFAULT_GOAL := help

# Default build (with SDL2 - requires Nix environment)
all: sdl

# Build with SDL2 support (requires Nix dev environment or system SDL2)
sdl:
	@echo "Building with SDL2 support..."
	zig build

# Build without SDL2 (stub mode - works anywhere)
nosdl:
	@echo "Building without SDL2 (stub mode)..."
	zig build -Dsdl=false

# Complete setup: download game data and build
setup:
	@echo "Setting up complete environment..."
	zig build setup

# Download game data from archive.org
download-data:
	@echo "Downloading game data..."
	zig build download-data

# Clean all build artifacts
clean:
	@echo "Cleaning build artifacts..."
	zig build clean

# Run the main game
run: sdl
	@echo "Running The Puzzle Pits..."
	zig build run-pits

# Run the demo
run-demo: sdl  
	@echo "Running demo..."
	zig build run-demo

# Run debug tests
test:
	@echo "Running tests..."
	zig build test-run

# Run comprehensive debug tests
debug:
	@echo "Running debug tests..."
	zig build debug-test

# Show build configuration info
info:
	@echo "Build system information:"
	zig build info

# Development targets for Nix users
nix-sdl:
	@echo "Building with Nix + SDL2..."
	nix develop --command zig build

nix-run:
	@echo "Running with Nix environment..."
	nix develop --command zig build run-pits

# Package builds using Nix
nix-build-sdl:
	@echo "Building SDL package with Nix..."
	nix build .#puzzle-pits-sdl

nix-build-nosdl:
	@echo "Building no-SDL package with Nix..."
	nix build .#puzzle-pits-nosdl

# Show help
help:
	@echo "The Puzzle Pits - Unified Build System"
	@echo ""
	@echo "RECOMMENDED USAGE:"
	@echo "  make setup          - Download game data and build with SDL2"
	@echo "  make run            - Build and run the game"
	@echo ""
	@echo "BUILD TARGETS:"
	@echo "  make sdl            - Build with SDL2 (requires Nix or system SDL2)"
	@echo "  make nosdl          - Build without SDL2 (stub mode, works anywhere)"
	@echo "  make setup          - Download data and build everything"
	@echo "  make clean          - Clean all build artifacts"
	@echo ""
	@echo "RUN TARGETS:"
	@echo "  make run            - Run the main game"
	@echo "  make run-demo       - Run the demo"
	@echo "  make test           - Run tests"
	@echo "  make debug          - Run debug tests"
	@echo ""
	@echo "NIX TARGETS (recommended):"
	@echo "  make nix-sdl        - Build with Nix environment"
	@echo "  make nix-run        - Run with Nix environment"
	@echo "  make nix-build-sdl  - Build SDL package with Nix"
	@echo ""
	@echo "DEVELOPMENT:"
	@echo "  make download-data  - Download game assets from archive.org"
	@echo "  make info           - Show build configuration"
	@echo "  make help           - Show this help"
	@echo ""
	@echo "REQUIREMENTS:"
	@echo "  - Zig compiler"
	@echo "  - For SDL builds: Nix environment (recommended) or system SDL2"
	@echo "  - curl and unzip (for downloading game data)"
	@echo ""
	@echo "QUICK START:"
	@echo "  1. nix develop       # Enter Nix environment (recommended)"
	@echo "  2. make setup        # Download data and build"
	@echo "  3. make run          # Play the game!"
	@echo ""
	@echo "  OR without Nix:"
	@echo "  1. make nosdl        # Build stub version"
	@echo "  2. make download-data # Get game assets"
	@echo ""
	@echo "All builds use the Zig build system for consistency."