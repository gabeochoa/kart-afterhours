workspace(name = "kart_afterhours")

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

# Raylib dependency
http_archive(
	name = "raylib",
	url = "https://github.com/raysan5/raylib/archive/refs/tags/5.0.zip",
	strip_prefix = "raylib-5.0",
	build_file_content = """
cc_library(
	name = "raylib",
	srcs = [
		"src/rcore.c",
		"src/rshapes.c",
		"src/rtextures.c",
		"src/rtext.c",
		"src/rmodels.c",
		"src/raudio.c",
		"src/rglfw.c",
		"src/utils.c",
	],
	hdrs = [
		"src/raylib.h",
		"src/raymath.h",
		"src/rlgl.h",
		"src/rgestures.h",
		"src/rcamera.h",
		"src/utils.h",
	],
	includes = [
		"src",
		"src/external/glfw/include",
	],
	copts = select({
		"@bazel_tools//src/conditions:windows": [
			"/D_CRT_SECURE_NO_WARNINGS",
			"/EHsc",
			"/DNOMINMAX",
			"/DWIN32_LEAN_AND_MEAN",
			"/DPLATFORM_DESKTOP",
			"/D_GLFW_WIN32",
			"/DGRAPHICS_API_OPENGL_33",
		],
		"@bazel_tools//src/conditions:darwin": ["-D_GLFW_COCOA"],
		"@bazel_tools//src/conditions:linux": ["-D_GLFW_X11"],
		"//conditions:default": [],
	}),
	linkopts = select({
		"@bazel_tools//src/conditions:windows": [
			"opengl32.lib",
			"gdi32.lib",
			"user32.lib",
			"shell32.lib",
			"winmm.lib",
			"kernel32.lib",
			"advapi32.lib",
			"ole32.lib",
			"uuid.lib",
			"imm32.lib",
		],
		"@bazel_tools//src/conditions:darwin": [
			"-framework", "Cocoa",
			"-framework", "OpenGL",
			"-framework", "IOKit",
			"-framework", "CoreAudio",
			"-framework", "CoreVideo",
		],
		"@bazel_tools//src/conditions:linux": [
			"-ldl",
			"-lm",
			"-lpthread",
			"-lGL",
			"-lX11",
			"-lXrandr",
			"-lXinerama",
			"-lXi",
			"-lXxf86vm",
			"-lXcursor",
		],
		"//conditions:default": [],
	}),
	visibility = ["//visibility:public"],
)
""",
)

# nlohmann/json dependency
http_archive(
	name = "nlohmann_json",
	url = "https://github.com/nlohmann/json/archive/refs/tags/v3.11.2.zip",
	strip_prefix = "json-3.11.2",
	build_file_content = """
cc_library(
	name = "nlohmann_json",
	hdrs = ["single_include/nlohmann/json.hpp"],
	includes = ["single_include"],
	visibility = ["//visibility:public"],
)
""",
)

# fmt library dependency
http_archive(
	name = "fmt",
	url = "https://github.com/fmtlib/fmt/archive/refs/tags/10.1.1.zip",
	strip_prefix = "fmt-10.1.1",
	build_file_content = """
cc_library(
	name = "fmt",
	hdrs = glob(["include/fmt/*.h"]),
	includes = ["include"],
	visibility = ["//visibility:public"],
)
""",
)

# magic_enum dependency
http_archive(
	name = "magic_enum",
	url = "https://github.com/Neargye/magic_enum/archive/refs/tags/v0.9.5.zip",
	strip_prefix = "magic_enum-0.9.5",
	build_file_content = """
cc_library(
	name = "magic_enum",
	hdrs = ["include/magic_enum/magic_enum.hpp"],
	includes = ["include"],
	visibility = ["//visibility:public"],
)
""",
)
