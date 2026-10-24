const std = @import("std");

// zig build                the game and the demo, with SDL2 (zig-out/bin)
// zig build run            play (from PITS95, where the game's data is)
// zig build download-data  the game's data from archive.org into PITS95
// zig build -Dsdl=false    without SDL2: compiles, shows and plays nothing
pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});
    const use_sdl = b.option(bool, "sdl", "Build with SDL2 (default true)") orelse true;

    const sources = [_][]const u8{
        "DIG.C",     "DMA.C",                     "ENV.C", "FILE.C", "GFX.C", "LOAD.C",
        "SETMODE.C", "minimal_dos_replacement.c",
    };
    // the game was written for a 16-bit DOS compiler: char is unsigned
    // (WORD and LONG are sized in compat.h)
    const base = [_][]const u8{
        "-std=gnu99",
        "-I.",
        "-funsigned-char",
        "-fno-strict-aliasing",
        "-Wno-pointer-sign",
        "-Wno-unknown-pragmas",
        "-Wno-unused-variable",
        "-Wno-unused-but-set-variable",
    };
    const flags: []const []const u8 = if (use_sdl) &(base ++ [_][]const u8{ "-DUSE_SDL", "-D_GNU_SOURCE=1" }) else &base;

    const exes = [_][2][]const u8{ .{ "puzzle-pits", "PITS.C" }, .{ "puzzle-pits-demo", "DEMO.C" } };
    var game: ?*std.Build.Step.Compile = null;
    for (exes) |e| {
        const mod = b.createModule(.{ .target = target, .optimize = optimize, .link_libc = true });
        mod.addCSourceFiles(.{ .files = &sources, .flags = flags, .language = .c }); // .C is C here
        mod.addCSourceFile(.{ .file = b.path(e[1]), .flags = flags, .language = .c });
        if (use_sdl) {
            mod.addCSourceFile(.{ .file = b.path("sdl_compat.c"), .flags = flags });
            mod.linkSystemLibrary("SDL2", .{});
        }
        const exe = b.addExecutable(.{ .name = e[0], .root_module = mod });
        b.installArtifact(exe);
        if (game == null) game = exe;
    }

    const run = b.addRunArtifact(game.?);
    run.setCwd(b.path("PITS95"));
    b.step("run", "Play The Puzzle Pits").dependOn(&run.step);

    const download = b.addSystemCommand(&.{ "sh", "-c", "curl -L -o pits95.zip https://archive.org/download/puzzle-pits/pits95.zip && unzip -o pits95.zip" });
    b.step("download-data", "Download the game's data from archive.org").dependOn(&download.step);
}
