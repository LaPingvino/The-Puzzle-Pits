#!/bin/bash
# build.sh - DEPRECATED
# This script has been replaced by the unified Zig build system

echo "================================================================"
echo "⚠️  DEPRECATED: build.sh has been replaced!"
echo "================================================================"
echo ""
echo "This project now uses a unified build system for consistency."
echo "Please use one of these commands instead:"
echo ""
echo "🎯 RECOMMENDED:"
echo "  make setup          # Download data and build with SDL2"
echo "  make run            # Build and run the game"
echo ""
echo "🔧 BUILD OPTIONS:"
echo "  make sdl            # Build with SDL2 (full graphics/audio)"
echo "  make nosdl          # Build without SDL2 (stub mode)"
echo ""
echo "🚀 NIX USERS (best experience):"
echo "  nix develop         # Enter development environment"
echo "  make setup          # Then build everything"
echo ""
echo "📋 OTHER COMMANDS:"
echo "  make test           # Run tests"
echo "  make clean          # Clean artifacts"
echo "  make help           # Show all options"
echo ""
echo "================================================================"
echo "All builds now use the Zig build system for consistency."
echo "This provides better cross-platform support and fewer conflicts."
echo "================================================================"
echo ""

# If user passed arguments, try to map them to new system
if [ "$#" -gt 0 ]; then
    case "$1" in
        --sdl|--SDL)
            echo "🔄 Auto-redirecting: $0 $* → make sdl"
            echo ""
            exec make sdl
            ;;
        --no-sdl|--nosdl|--NO-SDL)
            echo "🔄 Auto-redirecting: $0 $* → make nosdl"
            echo ""
            exec make nosdl
            ;;
        --help|-h|help)
            echo "🔄 Auto-redirecting: $0 $* → make help"
            echo ""
            exec make help
            ;;
        --clean|clean)
            echo "🔄 Auto-redirecting: $0 $* → make clean"
            echo ""
            exec make clean
            ;;
        *)
            echo "❌ Unknown option: $1"
            echo "💡 Try: make help"
            exit 1
            ;;
    esac
else
    echo "💡 For help: make help"
    echo "🎮 To get started: make setup"
fi

exit 0