const std = @import("std");
const testing = std.testing;
const print = std.debug.print;

// Import C functions - these will be linked from the C code
extern fn farmalloc(size: usize) ?*anyopaque;
extern fn farfree(ptr: ?*anyopaque) void;
extern fn init_sdl_compat() c_int;
extern fn cleanup_sdl_compat() void;
extern fn setmode(mode: c_int) void;
extern fn gfxInit() void;
extern fn LoadShapes() c_int;
extern fn LoadScreen(n: c_int) void;
extern fn SetPalette(n: c_int) void;

// File handling functions for case-insensitive tests
extern fn fopen_case_insensitive(fname: [*c]u8, mode: [*c]const u8) ?*anyopaque;
extern fn set_file_debug(enable: c_int) void;
extern fn Exists(fname: [*c]const u8) c_int;

// External variables we need to check
extern var logical: [*c]u8;
extern var physical: [*c]u8;
extern var vga_memory_buffer: [*c]u8;
extern var mx: u16;
extern var my: u16;
extern var moffx: u16;
extern var moffy: u16;
extern var mouseimage: u16;
extern var mouseavailable: u16;

// Test helper functions
fn printMemoryInfo() void {
    print("\n=== Memory Information ===\n", .{});
    print("logical pointer: {any}\n", .{logical});
    print("physical pointer: {any}\n", .{physical});
    print("vga_memory_buffer: {any}\n", .{vga_memory_buffer});
    print("mx: {}, my: {}\n", .{ mx, my });
    print("moffx: {}, moffy: {}\n", .{ moffx, moffy });
    print("mouseimage: {}\n", .{mouseimage});
    print("mouseavailable: {}\n", .{mouseavailable});
    print("===========================\n", .{});
}

fn isValidPointer(ptr: ?*anyopaque) bool {
    return ptr != null and @intFromPtr(ptr) > 0x1000; // Basic sanity check
}

fn isValidScreenPointer(ptr: [*c]u8) bool {
    return ptr != null and @intFromPtr(ptr) > 0x1000;
}

fn checkBounds(ptr: [*c]u8, base: [*c]u8, size: usize) bool {
    if (ptr == null or base == null) return false;
    const ptr_addr = @intFromPtr(ptr);
    const base_addr = @intFromPtr(base);
    return ptr_addr >= base_addr and ptr_addr < base_addr + size;
}

// Test 1: Memory allocation and basic initialization
test "memory allocation" {
    print("\n=== Testing Memory Allocation ===\n", .{});
    
    // Test basic memory allocation
    const test_mem = farmalloc(1024);
    try testing.expect(isValidPointer(test_mem));
    farfree(test_mem);
    
    // Test larger allocation similar to what the game uses
    const large_mem = farmalloc(64000);
    try testing.expect(isValidPointer(large_mem));
    farfree(large_mem);
    
    print("Memory allocation tests passed!\n", .{});
}

// Test 2: SDL initialization
test "sdl initialization" {
    print("\n=== Testing SDL Initialization ===\n", .{});
    
    const result = init_sdl_compat();
    try testing.expect(result == 0);
    
    // Check if vga_memory_buffer was allocated
    try testing.expect(isValidScreenPointer(vga_memory_buffer));
    
    print("SDL initialization tests passed!\n", .{});
    
    // Clean up
    cleanup_sdl_compat();
}

// Test 3: Graphics system initialization
test "graphics system initialization" {
    print("\n=== Testing Graphics System Initialization ===\n", .{});
    
    // Initialize SDL first
    const sdl_result = init_sdl_compat();
    try testing.expect(sdl_result == 0);
    
    // Initialize graphics system
    gfxInit();
    
    // Check that all pointers are valid
    printMemoryInfo();
    
    try testing.expect(isValidScreenPointer(logical));
    try testing.expect(isValidScreenPointer(physical));
    try testing.expect(isValidScreenPointer(vga_memory_buffer));
    
    // Check mouse initialization
    try testing.expect(mx > 0 and mx < 320);
    try testing.expect(my > 0 and my < 200);
    try testing.expect(moffx < 16);
    try testing.expect(moffy < 16);
    
    print("Graphics system initialization tests passed!\n", .{});
    
    cleanup_sdl_compat();
}

// Test 4: Memory bounds checking
test "memory bounds checking" {
    print("\n=== Testing Memory Bounds Checking ===\n", .{});
    
    const sdl_result = init_sdl_compat();
    try testing.expect(sdl_result == 0);
    
    gfxInit();
    
    // Test bounds checking for screen buffers
    try testing.expect(checkBounds(logical, logical, 64000));
    try testing.expect(checkBounds(physical, physical, 64000));
    
    // Test invalid bounds
    const invalid_ptr = logical + 65000;
    try testing.expect(!checkBounds(invalid_ptr, logical, 64000));
    
    print("Memory bounds checking tests passed!\n", .{});
    
    cleanup_sdl_compat();
}

// Test 5: Mouse position bounds checking
test "mouse position bounds checking" {
    print("\n=== Testing Mouse Position Bounds Checking ===\n", .{});
    
    const sdl_result = init_sdl_compat();
    try testing.expect(sdl_result == 0);
    
    gfxInit();
    
    // Test mouse position calculations
    const mouse_x = mx;
    const mouse_y = my;
    const offset_x = moffx;
    const offset_y = moffy;
    
    // Check if mouse rectangle is within screen bounds
    try testing.expect(mouse_x >= offset_x);
    try testing.expect(mouse_y >= offset_y);
    try testing.expect(mouse_x - offset_x + 16 <= 320);
    try testing.expect(mouse_y - offset_y + 16 <= 200);
    
    // Test pointer calculations for mouse functions
    const mouse_ptr = logical + (mouse_y - offset_y) * 320 + (mouse_x - offset_x);
    try testing.expect(checkBounds(mouse_ptr, logical, 64000));
    
    print("Mouse position bounds checking tests passed!\n", .{});
    
    cleanup_sdl_compat();
}

// Test 6: Shape loading (if shapes are available)
test "shape loading" {
    print("\n=== Testing Shape Loading ===\n", .{});
    
    const sdl_result = init_sdl_compat();
    try testing.expect(sdl_result == 0);
    
    gfxInit();
    
    // Try to load shapes (this might fail if data files aren't available)
    const shapes_result = LoadShapes();
    if (shapes_result == 0) {
        print("Shape loading successful!\n", .{});
    } else {
        print("Shape loading failed (expected if data files not available): {}\n", .{shapes_result});
    }
    
    cleanup_sdl_compat();
}

// Test 7: Screen loading
test "screen loading" {
    print("\n=== Testing Screen Loading ===\n", .{});
    
    const sdl_result = init_sdl_compat();
    try testing.expect(sdl_result == 0);
    
    gfxInit();
    
    // Load shapes first
    const shapes_result = LoadShapes();
    if (shapes_result != 0) {
        print("Skipping screen loading test - shapes not available\n", .{});
        cleanup_sdl_compat();
        return;
    }
    
    // Try to load screen 0
    LoadScreen(0);
    print("Screen loading completed without crash!\n", .{});
    
    cleanup_sdl_compat();
}

// Test 8: Palette setting
test "palette setting" {
    print("\n=== Testing Palette Setting ===\n", .{});
    
    const sdl_result = init_sdl_compat();
    try testing.expect(sdl_result == 0);
    
    gfxInit();
    
    // Try to set palette
    SetPalette(2);
    print("Palette setting completed without crash!\n", .{});
    
    cleanup_sdl_compat();
}

// Test 9: Individual FadeIn components
test "fadein components" {
    print("\n=== Testing FadeIn Components ===\n", .{});
    
    const sdl_result = init_sdl_compat();
    try testing.expect(sdl_result == 0);
    
    gfxInit();
    
    // Test individual components of FadeIn
    print("Testing pointer arithmetic for FadeIn loop...\n", .{});
    
    // Simulate the FadeIn loop conditions
    var i: u16 = 0;
    while (i < 320) : (i += 17) {
        if (i >= 320) break;
        
        const src = logical + i;
        const dest = physical + i;
        
        // Check bounds
        try testing.expect(checkBounds(src, logical, 64000));
        try testing.expect(checkBounds(dest, physical, 64000));
        
        // Test inner loop bounds
        var y: u16 = 0;
        while (y < 200) : (y += 1) {
            const inner_src = src + y * 320;
            const inner_dest = dest + y * 320;
            
            try testing.expect(checkBounds(inner_src, logical, 64000));
            try testing.expect(checkBounds(inner_dest, physical, 64000));
        }
    }
    
    print("FadeIn components test passed!\n", .{});
    
    cleanup_sdl_compat();
}

// Test 10: Full initialization sequence
test "full initialization sequence" {
    print("\n=== Testing Full Initialization Sequence ===\n", .{});
    
    const sdl_result = init_sdl_compat();
    try testing.expect(sdl_result == 0);
    
    print("Step 1: SDL initialization - OK\n", .{});
    
    gfxInit();
    printMemoryInfo();
    print("Step 2: Graphics initialization - OK\n", .{});
    
    const shapes_result = LoadShapes();
    if (shapes_result == 0) {
        print("Step 3: Shape loading - OK\n", .{});
        
        LoadScreen(0);
        print("Step 4: Screen loading - OK\n", .{});
        
        SetPalette(2);
        print("Step 5: Palette setting - OK\n", .{});
        
        print("Full initialization sequence completed without crash!\n", .{});
    } else {
        print("Step 3: Shape loading - SKIPPED (data not available)\n", .{});
    }
    
    cleanup_sdl_compat();
}

// Test 11: Stress test for memory operations
test "memory stress test" {
    print("\n=== Memory Stress Test ===\n", .{});
    
    const sdl_result = init_sdl_compat();
    try testing.expect(sdl_result == 0);
    
    gfxInit();
    
    // Stress test: repeatedly access memory locations that FadeIn would use
    var test_count: u32 = 0;
    var i: u16 = 0;
    while (i < 320) : (i += 17) {
        if (i >= 320) break;
        
        const src = logical + i;
        const dest = physical + i;
        
        var y: u16 = 0;
        while (y < 200) : (y += 1) {
            const inner_src = src + y * 320;
            const inner_dest = dest + y * 320;
            
            // Simulate the memory operation from FadeIn
            if (checkBounds(inner_src, logical, 64000) and 
                checkBounds(inner_dest, physical, 64000)) {
                inner_dest[0] = inner_src[0];
                test_count += 1;
            }
        }
    }
    
    print("Memory stress test completed: {} operations\n", .{test_count});
    
    cleanup_sdl_compat();
}

// Test 12: File case-insensitive opening
test "file case insensitive opening" {
    print("\n=== Testing Case-Insensitive File Opening ===\n", .{});
    
    // Enable debug output for file operations
    set_file_debug(1);
    
    // Create a test file with known content
    const test_filename = "TestFile.txt";
    const test_content = "test content for case insensitive file test\n";
    
    // Create the file using standard C fopen
    const std_fopen = @extern(*const fn ([*c]const u8, [*c]const u8) callconv(.C) ?*anyopaque, .{ .name = "fopen" });
    const fclose = @extern(*const fn (?*anyopaque) callconv(.C) c_int, .{ .name = "fclose" });
    const fprintf = @extern(*const fn (?*anyopaque, [*c]const u8, ...) callconv(.C) c_int, .{ .name = "fprintf" });
    
    const create_file = std_fopen(test_filename, "w");
    try testing.expect(create_file != null);
    _ = fprintf(create_file, "%s", test_content.ptr);
    _ = fclose(create_file);
    
    print("Created test file: {s}\n", .{test_filename});
    
    // Test 1: Open with exact case
    var exact_case_name = [_]u8{0} ** 256;
    @memcpy(exact_case_name[0..test_filename.len], test_filename);
    const exact_file = fopen_case_insensitive(&exact_case_name, "r");
    try testing.expect(exact_file != null);
    if (exact_file) |f| _ = fclose(f);
    print("✓ Opened with exact case\n", .{});
    
    // Test 2: Open with all lowercase
    var lower_case_name = [_]u8{0} ** 256;
    const lower_name = "testfile.txt";
    @memcpy(lower_case_name[0..lower_name.len], lower_name);
    const lower_file = fopen_case_insensitive(&lower_case_name, "r");
    try testing.expect(lower_file != null);
    if (lower_file) |f| _ = fclose(f);
    print("✓ Opened with lowercase variation\n", .{});
    
    // Test 3: Open with all uppercase
    var upper_case_name = [_]u8{0} ** 256;
    const upper_name = "TESTFILE.TXT";
    @memcpy(upper_case_name[0..upper_name.len], upper_name);
    const upper_file = fopen_case_insensitive(&upper_case_name, "r");
    try testing.expect(upper_file != null);
    if (upper_file) |f| _ = fclose(f);
    print("✓ Opened with uppercase variation\n", .{});
    
    // Test 4: Test Exists function
    try testing.expect(Exists(test_filename) != 0);
    try testing.expect(Exists("testfile.txt") != 0);
    try testing.expect(Exists("TESTFILE.TXT") != 0);
    try testing.expect(Exists("NonExistentFile.xyz") == 0);
    print("✓ Exists function works with case variations\n", .{});
    
    // Disable debug output
    set_file_debug(0);
    
    // Clean up
    const unlink = @extern(*const fn ([*c]const u8) callconv(.C) c_int, .{ .name = "unlink" });
    _ = unlink(test_filename);
    
    print("File case-insensitive opening tests passed!\n", .{});
}

// Test 13: Game file extensions case handling
test "game file extensions case handling" {
    print("\n=== Testing Game File Extensions Case Handling ===\n", .{});
    
    // Test with common game file extensions
    const test_files = [_]struct {
        create_name: []const u8,
        test_names: []const []const u8,
    }{
        .{
            .create_name = "tiles1.DAT",
            .test_names = &[_][]const u8{ "tiles1.dat", "TILES1.DAT", "Tiles1.Dat" },
        },
        .{
            .create_name = "level01.PIT",
            .test_names = &[_][]const u8{ "level01.pit", "LEVEL01.PIT", "Level01.Pit" },
        },
        .{
            .create_name = "font.FNT",
            .test_names = &[_][]const u8{ "font.fnt", "FONT.FNT", "Font.Fnt" },
        },
    };
    
    const std_fopen = @extern(*const fn ([*c]const u8, [*c]const u8) callconv(.C) ?*anyopaque, .{ .name = "fopen" });
    const fclose = @extern(*const fn (?*anyopaque) callconv(.C) c_int, .{ .name = "fclose" });
    const fprintf = @extern(*const fn (?*anyopaque, [*c]const u8, ...) callconv(.C) c_int, .{ .name = "fprintf" });
    const unlink = @extern(*const fn ([*c]const u8) callconv(.C) c_int, .{ .name = "unlink" });
    
    for (test_files) |test_case| {
        // Create the file
        const create_file = std_fopen(test_case.create_name.ptr, "w");
        try testing.expect(create_file != null);
        _ = fprintf(create_file, "test data for %s\n", test_case.create_name.ptr);
        _ = fclose(create_file);
        
        print("Testing file: {s}\n", .{test_case.create_name});
        
        // Test all case variations
        for (test_case.test_names) |test_name| {
            var name_buffer = [_]u8{0} ** 256;
            @memcpy(name_buffer[0..test_name.len], test_name);
            
            const file = fopen_case_insensitive(&name_buffer, "r");
            try testing.expect(file != null);
            if (file) |f| _ = fclose(f);
            
            // Also test with Exists function
            try testing.expect(Exists(test_name.ptr) != 0);
        }
        
        // Clean up
        _ = unlink(test_case.create_name.ptr);
        print("✓ All case variations work for {s}\n", .{test_case.create_name});
    }
    
    print("Game file extensions case handling tests passed!\n", .{});
}

// Test 14: Path component case handling
test "path component case handling" {
    print("\n=== Testing Path Component Case Handling ===\n", .{});
    
    const mkdir = @extern(*const fn ([*c]const u8, c_int) callconv(.C) c_int, .{ .name = "mkdir" });
    const rmdir = @extern(*const fn ([*c]const u8) callconv(.C) c_int, .{ .name = "rmdir" });
    const std_fopen = @extern(*const fn ([*c]const u8, [*c]const u8) callconv(.C) ?*anyopaque, .{ .name = "fopen" });
    const fclose = @extern(*const fn (?*anyopaque) callconv(.C) c_int, .{ .name = "fclose" });
    const fprintf = @extern(*const fn (?*anyopaque, [*c]const u8, ...) callconv(.C) c_int, .{ .name = "fprintf" });
    const unlink = @extern(*const fn ([*c]const u8) callconv(.C) c_int, .{ .name = "unlink" });
    
    // Create test directory
    _ = mkdir("test_levels", 0o755);
    
    // Create a test file in the directory
    const test_path = "test_levels/Level01.pit";
    const create_file = std_fopen(test_path, "w");
    try testing.expect(create_file != null);
    _ = fprintf(create_file, "level data\n");
    _ = fclose(create_file);
    
    print("Created test file: {s}\n", .{test_path});
    
    // Test various case combinations for the filename part
    const test_paths = [_][]const u8{
        "test_levels/level01.pit",
        "test_levels/LEVEL01.PIT", 
        "test_levels/Level01.Pit",
    };
    
    for (test_paths) |path| {
        var path_buffer = [_]u8{0} ** 256;
        @memcpy(path_buffer[0..path.len], path);
        
        const file = fopen_case_insensitive(&path_buffer, "r");
        try testing.expect(file != null);
        if (file) |f| _ = fclose(f);
        
        print("✓ Opened: {s}\n", .{path});
    }
    
    // Clean up
    _ = unlink(test_path);
    _ = rmdir("test_levels");
    
    print("Path component case handling tests passed!\n", .{});
}

// Test runner function
pub fn runAllTests() void {
    print("=== Starting Puzzle Pits Debug Tests ===\n", .{});
    
    // Individual test calls would go here in a real test runner
    print("Run with: zig test tests.zig\n", .{});
}