const std = @import("std");

// Mirrors the makefile's compile/link. Everything else (count, cppcheck, deps,
// prof/leak/alloc, screenshot baselines) stays in make -- see port_to_zig_build.md.
//
// Sources are discovered by walking the tree, so adding a .cpp never needs a
// build.zig edit. That's the one thing the notes said build.zig couldn't do.

const NOFLAGS = [_][]const u8{
    "-Wno-deprecated-volatile",
    "-Wno-missing-field-initializers",
    "-Wno-c99-extensions",
    "-Wno-unused-function",
    "-Wno-sign-conversion",
    "-Wno-implicit-int-float-conversion",
};

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const e2e = b.option(bool, "e2e", "Enable e2e testing hooks") orelse false;
    const mcp = b.option(bool, "mcp", "Enable the MCP server") orelse false;

    const mod = b.createModule(.{
        .target = target,
        .optimize = optimize,
        .link_libcpp = true,
    });

    var flags: std.ArrayList([]const u8) = .empty;
    flags.appendSlice(b.allocator, &.{
        "-std=c++23",
        "-Wmost",
        "-g",
        "-ftime-trace",
        "-Ivendor/",
        "-Isrc/",
        // Must be a build flag, not a #define in a header. It used to live in
        // src/log.h, so any TU that pulled <fmt/format.h> without going
        // through log.h -- ui_systems.cpp -- got the non-header-only fmt and
        // referenced an extern fmt::vformat that nothing defines. -O0 kept the
        // symbol alive; any optimised build failed to link.
        "-DFMT_HEADER_ONLY",
        "-DAFTER_HOURS_USE_RAYLIB",
        "-DAFTER_HOURS_UI_SINGLE_COLLECTION",
    }) catch @panic("OOM");
    flags.appendSlice(b.allocator, &NOFLAGS) catch @panic("OOM");
    if (e2e) flags.append(b.allocator, "-DAFTER_HOURS_ENABLE_E2E_TESTING") catch @panic("OOM");
    if (mcp) flags.append(b.allocator, "-DAFTER_HOURS_ENABLE_MCP") catch @panic("OOM");

    var sources: std.ArrayList([]const u8) = .empty;
    collectCpp(b, &sources, "src", .recursive);
    collectCpp(b, &sources, "vendor/afterhours/src/plugins", .flat);
    // Walk order is filesystem order; sort so the build cache key is stable.
    std.mem.sort([]const u8, sources.items, {}, lessThan);

    mod.addCSourceFiles(.{
        .files = sources.items,
        .flags = flags.items,
        .language = .cpp,
    });

    if (target.result.os.tag == .windows) {
        mod.addIncludePath(b.path("vendor/raylib"));
        mod.addObjectFile(b.path("vendor/raylib/libraylibdll.a"));
        // ole32/shell32: sago's SHGetKnownFolderPath + CoTaskMemFree, used by
        // the files plugin to find the save directory.
        for ([_][]const u8{ "opengl32", "gdi32", "winmm", "ole32", "shell32" }) |lib| {
            mod.linkSystemLibrary(lib, .{});
        }
    } else {
        mod.linkSystemLibrary("raylib", .{ .use_pkg_config = .yes });
        mod.linkFramework("OpenGL", .{});
    }

    const exe = b.addExecutable(.{ .name = "kart", .root_module = mod });
    b.installArtifact(exe);

    const run_cmd = b.addRunArtifact(exe);
    run_cmd.step.dependOn(b.getInstallStep());
    // The game loads resources/ relative to cwd, and cwd for a RunArtifact is
    // the cache dir, not the repo.
    run_cmd.setCwd(b.path("."));
    if (b.args) |args| run_cmd.addArgs(args);
    b.step("run", "Run the game").dependOn(&run_cmd.step);
}

fn lessThan(_: void, a: []const u8, c: []const u8) bool {
    return std.mem.lessThan(u8, a, c);
}

fn collectCpp(
    b: *std.Build,
    list: *std.ArrayList([]const u8),
    sub_path: []const u8,
    depth: enum { flat, recursive },
) void {
    const io = b.graph.io;
    var dir = b.build_root.handle.openDir(io, sub_path, .{ .iterate = true }) catch |err|
        std.debug.panic("cannot open {s}: {t} (submodules checked out?)", .{ sub_path, err });
    defer dir.close(io);

    switch (depth) {
        .flat => {
            var it = dir.iterate();
            while (it.next(io) catch @panic("iterate failed")) |entry| {
                if (entry.kind != .file or !std.mem.endsWith(u8, entry.name, ".cpp")) continue;
                list.append(b.allocator, b.pathJoin(&.{ sub_path, entry.name })) catch @panic("OOM");
            }
        },
        .recursive => {
            var walker = dir.walk(b.allocator) catch @panic("OOM");
            defer walker.deinit();
            while (walker.next(io) catch @panic("walk failed")) |entry| {
                if (entry.kind != .file or !std.mem.endsWith(u8, entry.basename, ".cpp")) continue;
                // entry.path is only valid until the next next(); pathJoin dupes it.
                list.append(b.allocator, b.pathJoin(&.{ sub_path, entry.path })) catch @panic("OOM");
            }
        },
    }
}
