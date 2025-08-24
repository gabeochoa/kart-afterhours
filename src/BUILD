cc_library(
    name = "library",
    hdrs = ["library.h"],
    deps = [":std_include"],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "shader_types",
    hdrs = ["shader_types.h"],
    deps = [":std_include"],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "max_health",
    hdrs = ["max_health.h"],
    deps = [":std_include"],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "game_state_manager",
    hdrs = ["game_state_manager.h"],
    deps = [":std_include"],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "query",
    hdrs = ["query.h"],
    deps = [
        ":std_include",
        ":components",
    ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "sound_library",
    hdrs = ["sound_library.h"],
    deps = [
        ":std_include",
        ":rl",
        ":resources",
        ":library",
    ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "round_settings",
    hdrs = ["round_settings.h"],
    deps = [":std_include"],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "std_include",
    hdrs = ["std_include.h"],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "log",
    hdrs = [
        "log.h",
        "log/log.h",
        "log/log_fakelog.h",
        "log/log_level.h",
        "log/log_macros.h",
    ],
    srcs = ["log.h"],
    deps = [
        ":std_include",
        "@fmt//:fmt",
        "@magic_enum//:magic_enum",
    ],
    copts = select({
        "@bazel_tools//src/conditions:windows": [
            "/DNOMINMAX",
            "/DWIN32_LEAN_AND_MEAN",
        ],
        "//conditions:default": [],
    }),
    visibility = ["//visibility:public"],
)

cc_library(
    name = "rl",
    hdrs = ["rl.h"],
    deps = [
        ":std_include",
        "@raylib//:raylib",
        "//vendor:glfw_headers",
        "//vendor:afterhours_headers",
        "//vendor:raylib_op_overloads",
    ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "components",
    hdrs = [
        "components.h",
        "components_weapons.h",
        "tags.h",
    ],
    deps = [":std_include"],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "math_util",
    hdrs = ["math_util.h"],
    deps = [":std_include"],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "bitset_utils",
    hdrs = ["bitset_utils.h"],
    deps = [":std_include"],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "config",
    hdrs = ["config.h"],
    deps = [":std_include"],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "utils",
    hdrs = ["utils.h"],
    deps = [":std_include"],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "input_mapping",
    hdrs = ["input_mapping.h"],
    deps = [":std_include"],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "resources",
    hdrs = ["resources.h"],
    srcs = ["resources.cpp"],
    deps = [
        ":std_include",
        ":log",
        ":rl",
    ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "music_library",
    hdrs = ["music_library.h"],
    deps = [
        ":std_include",
        ":library",
        ":rl",
    ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "shader_library",
    hdrs = ["shader_library.h"],
    deps = [
        ":std_include",
        ":library",
        ":rl",
    ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "texture_library",
    hdrs = ["texture_library.h"],
    deps = [
        ":std_include",
        ":library",
        ":rl",
    ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "settings",
    hdrs = ["settings.h"],
    srcs = ["settings.cpp"],
    deps = [
        ":std_include",
        ":log",
        ":config",
        ":rl",
        ":library",
        ":music_library",
        ":round_settings",
        ":components",
        ":shader_types",
        ":input_mapping",
        ":math_util",
        ":max_health",
        ":sound_library",
        ":resources",
        ":bitset_utils",
        ":weapons",
        "@nlohmann_json//:nlohmann_json",
        "//vendor:afterhours_headers",
        "@raylib//:raylib",
    ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "makers",
    hdrs = ["makers.h"],
    srcs = ["makers.cpp"],
    deps = [
        ":std_include",
        ":components",
        ":resources",
        ":settings",
        ":rl",
        ":log",
    ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "systems",
    hdrs = ["systems.h"],
    deps = [
        ":std_include",
        ":components",
        ":rl",
        ":log",
        ":math_util",
        ":utils",
        ":resources",
        ":sound_library",
    ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "systems_ai",
    hdrs = ["systems_ai.h"],
    deps = [
        ":systems",
        ":components",
        ":rl",
        ":math_util",
    ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "systems_roundtypes",
    hdrs = ["systems_roundtypes.h"],
    deps = [
        ":systems",
        ":components",
        ":rl",
        ":math_util",
        ":resources",
        ":settings",
        ":makers",
    ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "map_system",
    hdrs = ["map_system.h"],
    srcs = ["map_system.cpp"],
    deps = [
        ":std_include",
        ":rl",
        ":log",
        ":resources",
        ":settings",
        ":library",
        ":makers",
        ":components",
        ":shader_types",
        ":input_mapping",
        ":math_util",
        ":max_health",
        ":sound_library",
        ":bitset_utils",
        ":weapons",
        ":round_settings",
        ":query",
    ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "sound_systems",
    hdrs = ["sound_systems.h"],
    deps = [
        ":std_include",
        ":rl",
        ":log",
        ":resources",
        ":settings",
        ":systems",
        "//vendor:afterhours_headers",
    ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "weapons",
    hdrs = ["weapons.h"],
    deps = [
        ":std_include",
        ":components",
        ":rl",
        ":log",
        ":math_util",
        ":resources",
    ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "navigation",
    hdrs = ["navigation.h"],
    srcs = ["navigation.cpp"],
    deps = [
        ":std_include",
        ":rl",
        ":log",
        ":resources",
        ":settings",
        ":systems",
        ":library",
        ":game_state_manager",
        ":shader_types",
        ":input_mapping",
        ":max_health",
        ":sound_library",
        ":bitset_utils",
        ":round_settings",
        ":weapons",
        ":query",
    ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "preload",
    hdrs = ["preload.h"],
    srcs = ["preload.cpp"],
    deps = [
        ":std_include",
        ":rl",
        ":log",
        ":resources",
        ":settings",
        ":sound_systems",
        ":map_system",
        ":music_library",
        ":navigation",
        ":game_state_manager",
        ":shader_library",
        ":texture_library",
    ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "intro",
    hdrs = ["intro.h"],
    deps = [
        ":std_include",
        ":rl",
        ":log",
        ":resources",
        ":settings",
        ":navigation",
    ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "ui_systems",
    hdrs = ["ui_systems.h"],
    deps = [
        ":std_include",
        ":rl",
        ":log",
        ":resources",
        ":settings",
        ":systems",
        ":input_mapping",
        ":navigation",
    ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "game",
    hdrs = ["game.h"],
    deps = [
        ":std_include",
        ":rl",
        ":log",
        ":resources",
        ":settings",
        ":systems",
        ":systems_ai",
        ":systems_roundtypes",
        ":sound_systems",
        ":weapons",
        ":navigation",
        ":ui_systems",
        ":makers",
        ":map_system",
        ":preload",
        ":config",
        ":math_util",
        ":utils",
        ":bitset_utils",
        ":input_mapping",
        "//vendor:argh",
        "//vendor:backward_headers",
    ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "car_affectors",
    hdrs = ["car_affectors.h"],
    deps = [":std_include"],
    visibility = ["//visibility:public"],
)

# ui_animations now lives in subpackage //src/ui

cc_binary(
    name = "kart",
    srcs = ["main.cpp"],
    deps = [
        ":game",
        ":settings",
        ":preload",
        ":intro",
        ":car_affectors",
        "//src/ui:ui_animations",
        "//src/ui:ui_systems",
        "//vendor:argh",
        "@raylib//:raylib",
    ],
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
        "@bazel_tools//src/conditions:linux": ["-ldl", "-lm", "-lpthread"],
        "//conditions:default": [],
    }),
    visibility = ["//visibility:public"],
)
