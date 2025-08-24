cc_library(
    name = "ui_animations",
    hdrs = [
        "animation_button_wiggle.h",
        "animation_key.h",
        "animation_slide_in.h",
    ],
    strip_include_prefix = ".",
    include_prefix = "src",
    deps = ["//src:std_include"],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "ui_systems",
    srcs = [
        "reusable_components.cpp",
        "systems_core.cpp",
    ],
    hdrs = [
        "reusable_components.h",
    ],
    strip_include_prefix = ".",
    include_prefix = "src",
    deps = [
        "//src:std_include",
        "//src:rl",
        "//src:log",
        "//src:resources",
        "//src:settings",
        "//src:systems",
        "//src:input_mapping",
        "//src:navigation",
        "//src:preload",
        "//src:ui_systems",
        "//src:game",
        "//src/ui:ui_animations",
    ],
    visibility = ["//visibility:public"],
)
