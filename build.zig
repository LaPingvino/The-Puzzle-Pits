const std = @import("std");

// zig build                the game and the demo, with SDL2 (zig-out/bin)
// zig build run            play (from PITS95, where the game's data is)
// zig build download-data  the game's data from archive.org into PITS95
// zig build -Dsdl=false    without SDL2: compiles, shows and plays nothing
pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});
    const use_sdl = b.option(bool, "sdl", "Build with SDL2 (default true)") orelse true;
    // an SDL2 not installed for the system: a folder with include/ and lib/
    // (for Windows, the x86_64-w64-mingw32 folder of SDL2's mingw package)
    const sdl_prefix = b.option([]const u8, "sdl-prefix", "Where SDL2's include and lib folders are");

    const sources = [_][]const u8{
        "ENV.C", "FILE.C", "GFX.C", "LOAD.C", // SETMODE.C is DOS's (Watcom asm), unused
    };
    // the game was written for a 16-bit DOS compiler: char is unsigned
    // (WORD and LONG are sized in compat.h)
    const base = [_][]const u8{
        "-std=gnu11",
        "-I.",
        "-funsigned-char",
        "-fno-strict-aliasing",
        "-Wno-pointer-sign",
        "-Wno-unknown-pragmas",
        "-Wno-unused-variable",
        "-Wno-unused-but-set-variable",
    };
    // SDL_MAIN_HANDLED: main stays main (SDL renames it on Windows)
    const flags: []const []const u8 = if (use_sdl) &(base ++ [_][]const u8{ "-DUSE_SDL", "-D_GNU_SOURCE=1", "-DSDL_MAIN_HANDLED" }) else &base;

    const exes = [_][2][]const u8{ .{ "puzzle-pits", "PITS.C" }, .{ "puzzle-pits-demo", "DEMO.C" } };
    var game: ?*std.Build.Step.Compile = null;
    for (exes) |e| {
        const mod = b.createModule(.{ .target = target, .optimize = optimize, .link_libc = true });
        mod.addCSourceFiles(.{ .files = &sources, .flags = flags, .language = .c }); // .C is C here
        mod.addCSourceFile(.{ .file = b.path(e[1]), .flags = flags, .language = .c });
        if (use_sdl) { // sdl_compat.c plays the sounds DIG.C and DMA.C sent to a Sound Blaster (stubbed in minimal_dos_replacement.c)
            mod.addCSourceFile(.{ .file = b.path("sdl_compat.c"), .flags = flags });
            if (sdl_prefix) |p| {
                mod.addIncludePath(.{ .cwd_relative = b.pathJoin(&.{ p, "include" }) });
                mod.addLibraryPath(.{ .cwd_relative = b.pathJoin(&.{ p, "lib" }) });
                if (target.result.os.tag == .windows) { // SDL2.dll, not the static libSDL2.a beside it
                    mod.addObjectFile(.{ .cwd_relative = b.pathJoin(&.{ p, "lib", "libSDL2.dll.a" }) });
                } else {
                    mod.linkSystemLibrary("SDL2", .{ .use_pkg_config = .no });
                }
            } else {
                mod.linkSystemLibrary("SDL2", .{});
            }
        } else {
            mod.addCSourceFiles(.{ .files = &.{ "DIG.C", "DMA.C", "minimal_dos_replacement.c" }, .flags = flags, .language = .c });
        }
        const exe = b.addExecutable(.{ .name = e[0], .root_module = mod });
        b.installArtifact(exe);
        if (game == null) game = exe;
    }

    const run = b.addRunArtifact(game.?);
    run.setCwd(b.path("PITS95"));
    b.step("run", "Play The Puzzle Pits").dependOn(&run.step);

    // zig build web: the game for a browser (zig-out/web/index.html), with
    // Emscripten's emcc and its SDL2; Asyncify lets the game's own loops
    // give the page its turn
    const web = b.addSystemCommand(&.{ "emcc", "-O2", "-sUSE_SDL=2", "-sASYNCIFY", "-sALLOW_MEMORY_GROWTH", "-sEXIT_RUNTIME" });
    web.addArgs(&base);
    web.addArgs(&.{ "-DUSE_SDL", "-DSDL_MAIN_HANDLED" });
    web.addPrefixedDirectoryArg("-I", b.path("."));
    // emcc takes .C for C++ whatever -x says: copies named .c
    const lower = b.addWriteFiles();
    for (sources ++ [_][]const u8{"PITS.C"}) |src| {
        const name = b.fmt("{s}.c", .{src[0 .. src.len - 2]});
        web.addFileArg(lower.addCopyFile(b.path(src), name));
    }
    web.addFileArg(b.path("sdl_compat.c"));
    web.addArgs(&.{ "--preload-file", "PITS95@/", "--shell-file" });
    web.addFileArg(b.path("web/shell.html"));
    web.addArg("-o");
    const page = web.addOutputFileArg("index.html");
    web.setCwd(b.path("."));
    const install_web = b.addInstallDirectory(.{ .source_dir = page.dirname(), .install_dir = .prefix, .install_subdir = "web" });
    b.step("web", "The game for a browser, in zig-out/web").dependOn(&install_web.step);

    const download = b.addSystemCommand(&.{ "sh", "-c", "curl -L -o pits95.zip https://archive.org/download/puzzle-pits/pits95.zip && unzip -o pits95.zip" });
    b.step("download-data", "Download the game's data from archive.org").dependOn(&download.step);
}
