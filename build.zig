const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});
    
    // Build options
    const use_sdl = b.option(bool, "sdl", "Build with SDL2 support") orelse true;
    
    // Common source files
    const common_sources = [_][]const u8{
        "DIG.C",
        "DMA.C", 
        "ENV.C",
        "FILE.C",
        "GFX.C",
        "LOAD.C",
        "SB.C",
        "SETMODE.C",
    };
    
    // Common C flags
    const base_cflags = [_][]const u8{
        "-Wall",
        "-O2",
        "-I.",
        "-Wno-pointer-sign",
        "-fno-strict-aliasing", 
        "-Wno-format-zero-length",
        "-std=gnu99",
        "-x", "c", // Force treat .C files as C code
    };
    
    // Simplified C flags for debug test
    const debug_cflags = [_][]const u8{
        "-Wall",
        "-O0",
        "-g",
        "-I.",
        "-Wno-pointer-sign",
        "-Wno-sign-conversion",
        "-Wno-unknown-pragmas",
        "-Wno-unused-variable",
        "-Wno-unused-but-set-variable",
        "-fno-strict-aliasing",
    };
    
    // SDL-specific flags
    const sdl_cflags = [_][]const u8{
        "-DUSE_SDL",
        "-D_GNU_SOURCE=1",
        "-D_REENTRANT",
    };
    
    // Create executables
    const demo_exe = b.addExecutable(.{
        .name = "puzzle-pits-demo",
        .target = target,
        .optimize = optimize,
    });
    
    const pits_exe = b.addExecutable(.{
        .name = "puzzle-pits",
        .target = target,
        .optimize = optimize,
    });
    
    // Add common sources to both executables
    for (common_sources) |src| {
        demo_exe.addCSourceFile(.{ .file = b.path(src), .flags = &base_cflags });
        pits_exe.addCSourceFile(.{ .file = b.path(src), .flags = &base_cflags });
    }
    
    // Add main source files
    if (use_sdl) {
        const sdl_flags = base_cflags ++ sdl_cflags;
        demo_exe.addCSourceFile(.{ .file = b.path("DEMO.C"), .flags = &sdl_flags });
        pits_exe.addCSourceFile(.{ .file = b.path("PITS.C"), .flags = &sdl_flags });
        
        // Add SDL compatibility layer
        demo_exe.addCSourceFile(.{ .file = b.path("sdl_compat.c"), .flags = &sdl_flags });
        pits_exe.addCSourceFile(.{ .file = b.path("sdl_compat.c"), .flags = &sdl_flags });
        
        // Link with SDL2
        demo_exe.linkSystemLibrary("SDL2");
        pits_exe.linkSystemLibrary("SDL2");
        
        // SDL2 include paths will be found automatically by the system
        // In Nix environments, pkg-config handles this
    } else {
        demo_exe.addCSourceFile(.{ .file = b.path("DEMO.C"), .flags = &base_cflags });
        pits_exe.addCSourceFile(.{ .file = b.path("PITS.C"), .flags = &base_cflags });
    }
    
    // Link with system libraries
    demo_exe.linkLibC();
    pits_exe.linkLibC();
    
    // Create test executable for debugging
    const test_exe = b.addTest(.{
        .root_source_file = b.path("tests.zig"),
        .target = target,
        .optimize = optimize,
    });
    
    // Add all C sources to test executable so we can call C functions
    for (common_sources) |src| {
        if (use_sdl) {
            const sdl_flags = base_cflags ++ sdl_cflags;
            test_exe.addCSourceFile(.{ .file = b.path(src), .flags = &sdl_flags });
        } else {
            test_exe.addCSourceFile(.{ .file = b.path(src), .flags = &base_cflags });
        }
    }
    
    // Add compatibility layer to tests
    if (use_sdl) {
        const sdl_flags = base_cflags ++ sdl_cflags;
        test_exe.addCSourceFile(.{ .file = b.path("sdl_compat.c"), .flags = &sdl_flags });
        test_exe.linkSystemLibrary("SDL2");
    }
    
    test_exe.linkLibC();
    
    // Create debug test executable (C-based)
    const debug_test_exe = b.addExecutable(.{
        .name = "debug-test",
        .target = target,
        .optimize = optimize,
    });
    
    // Add C source file for debug test
    if (use_sdl) {
        const debug_sdl_flags = debug_cflags ++ sdl_cflags;
        debug_test_exe.addCSourceFile(.{ .file = b.path("debug_test.c"), .flags = &debug_sdl_flags });
        
        // Add all common sources
        for (common_sources) |src| {
            debug_test_exe.addCSourceFile(.{ .file = b.path(src), .flags = &debug_sdl_flags });
        }
        
        // Add SDL compatibility layer
        debug_test_exe.addCSourceFile(.{ .file = b.path("sdl_compat.c"), .flags = &debug_sdl_flags });
        debug_test_exe.linkSystemLibrary("SDL2");
    } else {
        debug_test_exe.addCSourceFile(.{ .file = b.path("debug_test.c"), .flags = &debug_cflags });
        
        // Add all common sources
        for (common_sources) |src| {
            debug_test_exe.addCSourceFile(.{ .file = b.path(src), .flags = &debug_cflags });
        }
    }
    
    debug_test_exe.linkLibC();
    
    // Install executables
    const demo_install = b.addInstallArtifact(demo_exe, .{});
    const pits_install = b.addInstallArtifact(pits_exe, .{});
    
    // Create install step
    const install_step = b.getInstallStep();
    install_step.dependOn(&demo_install.step);
    install_step.dependOn(&pits_install.step);
    
    // Create run steps for testing
    const demo_run = b.addRunArtifact(demo_exe);
    const pits_run = b.addRunArtifact(pits_exe);
    
    const demo_run_step = b.step("run-demo", "Run the demo");
    demo_run_step.dependOn(&demo_run.step);
    
    const pits_run_step = b.step("run-pits", "Run the main game");
    pits_run_step.dependOn(&pits_run.step);
    
    // Debug test run step
    const debug_test_run = b.addRunArtifact(debug_test_exe);
    const debug_test_run_step = b.step("debug-test", "Run comprehensive debug tests to isolate segfault");
    debug_test_run_step.dependOn(&debug_test_run.step);
    
    // Create test steps
    const test_step = b.step("test", "Run all debug tests");
    test_step.dependOn(&test_exe.step);
    
    const test_run = b.addRunArtifact(test_exe);
    const test_run_step = b.step("test-run", "Run debug tests with output");
    test_run_step.dependOn(&test_run.step);
    
    // Individual test steps for specific debugging
    const test_memory = b.addSystemCommand(&[_][]const u8{
        "zig", "test", "tests.zig", "--test-filter", "memory"
    });
    const test_memory_step = b.step("test-memory", "Run memory-related tests only");
    test_memory_step.dependOn(&test_memory.step);
    
    const test_sdl = b.addSystemCommand(&[_][]const u8{
        "zig", "test", "tests.zig", "--test-filter", "sdl"
    });
    const test_sdl_step = b.step("test-sdl", "Run SDL-related tests only");
    test_sdl_step.dependOn(&test_sdl.step);
    
    const test_graphics = b.addSystemCommand(&[_][]const u8{
        "zig", "test", "tests.zig", "--test-filter", "graphics"
    });
    const test_graphics_step = b.step("test-graphics", "Run graphics-related tests only");
    test_graphics_step.dependOn(&test_graphics.step);
    
    const test_fadein = b.addSystemCommand(&[_][]const u8{
        "zig", "test", "tests.zig", "--test-filter", "fadein"
    });
    const test_fadein_step = b.step("test-fadein", "Run FadeIn-specific tests only");
    test_fadein_step.dependOn(&test_fadein.step);
    
    // Create a step to download game data
    const download_data_step = b.step("download-data", "Download game data from archive.org");
    const download_cmd = b.addSystemCommand(&[_][]const u8{
        "curl", "-L", "-o", "pits95.zip", 
        "https://archive.org/download/puzzle-pits/pits95.zip"
    });
    const extract_cmd = b.addSystemCommand(&[_][]const u8{
        "unzip", "-o", "pits95.zip"
    });
    extract_cmd.step.dependOn(&download_cmd.step);
    
    // Create lowercase data files
    const create_lowercase_cmd = b.addSystemCommand(&[_][]const u8{
        "sh", "-c", 
        "cd PITS95 && " ++
        "cp TILES1.DAT ../tiles1.dat && " ++
        "cp TILES2.DAT ../tiles2.dat && " ++
        "cp TILES3.DAT ../tiles3.dat && " ++
        "cp PLASMA1.FNT ../plasma1.fnt && " ++
        "cp LOGO.DAT ../logo.dat && " ++
        "cp TITLE.DAT ../title.dat"
    });
    create_lowercase_cmd.step.dependOn(&extract_cmd.step);
    
    download_data_step.dependOn(&create_lowercase_cmd.step);
    
    // Create a clean step
    const clean_step = b.step("clean", "Clean build artifacts and downloaded data");
    const clean_cmd = b.addSystemCommand(&[_][]const u8{
        "rm", "-rf", "zig-cache", "zig-out", "PITS95", "pits95.zip", 
        "tiles1.dat", "tiles2.dat", "tiles3.dat", "plasma1.fnt", "logo.dat", "title.dat"
    });
    clean_step.dependOn(&clean_cmd.step);
    
    // Print build information
    const info_step = b.step("info", "Show build information");
    const info_cmd = b.addSystemCommand(&[_][]const u8{"echo"});
    if (use_sdl) {
        info_cmd.addArg("Building with SDL2 support");
    } else {
        info_cmd.addArg("Building without SDL2 (stub mode)");
    }
    info_step.dependOn(&info_cmd.step);
    
    // Make info run before build
    demo_exe.step.dependOn(&info_cmd.step);
    
    // Create an all-in-one step
    const setup_step = b.step("setup", "Download data and build everything");
    setup_step.dependOn(download_data_step);
    setup_step.dependOn(install_step);
    
    // Print usage information
    const usage_step = b.step("help", "Show usage information");
    const usage_cmd = b.addSystemCommand(&[_][]const u8{
        "echo",
        "The Puzzle Pits Build System\n" ++
        "\n" ++
        "Usage:\n" ++
        "  zig build                    # Build with SDL2 support\n" ++
        "  zig build -Dsdl=false        # Build without SDL2 (stub mode)\n" ++
        "  zig build setup              # Download data and build everything\n" ++
        "  zig build download-data      # Download game data from archive.org\n" ++
        "  zig build run-demo           # Run the demo\n" ++
        "  zig build run-pits           # Run the main game\n" ++
        "  zig build clean              # Clean build artifacts\n" ++
        "  zig build info               # Show build configuration\n" ++
        "\n" ++
        "Debug Commands:\n" ++
        "  zig build test               # Run all debug tests\n" ++
        "  zig build test-run           # Run debug tests with detailed output\n" ++
        "  zig build test-memory        # Test memory allocation and bounds\n" ++
        "  zig build test-sdl           # Test SDL initialization\n" ++
        "  zig build test-graphics      # Test graphics system\n" ++
        "  zig build test-fadein        # Test FadeIn function components\n" ++
        "\n" ++
        "Requirements:\n" ++
        "  - SDL2 development libraries (for SDL build)\n" ++
        "  - curl (for downloading game data)\n" ++
        "  - unzip (for extracting game data)\n" ++
        "\n" ++
        "Install SDL2 on Ubuntu/Debian: sudo apt install libsdl2-dev\n" ++
        "Install SDL2 on macOS: brew install sdl2\n" ++
        "Install SDL2 on Arch: sudo pacman -S sdl2\n"
    });
    usage_step.dependOn(&usage_cmd.step);
}