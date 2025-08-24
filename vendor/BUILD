cc_library(
    name = "argh",
    hdrs = ["argh.h"],
    strip_include_prefix = ".",
    include_prefix = "",
    visibility = ["//visibility:public"],
)

cc_library(
    name = "claylib",
    hdrs = ["claylib.h"],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "raylib_op_overloads",
    hdrs = ["RaylibOpOverloads.h"],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "sago_platform_folders",
    hdrs = ["sago/platform_folders.h"],
    includes = ["sago"],
    visibility = ["//visibility:public"],
)

# Header-only fmt from vendor directory (kept private to avoid conflicts)
cc_library(
    name = "fmt",
    hdrs = glob([
        "fmt/*.h",
    ]),
    includes = ["fmt"],
    visibility = ["//visibility:private"],
)

# Header-only magic_enum from vendor directory (kept private to avoid conflicts)
cc_library(
    name = "magic_enum",
    hdrs = glob([
        "magic_enum/*.hpp",
    ]),
    includes = ["magic_enum"],
    visibility = ["//visibility:private"],
)

# Header-only backward-cpp from vendor directory
cc_library(
    name = "backward_headers",
    hdrs = ["backward/backward.hpp"],
    includes = ["backward"],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "glfw_headers",
    hdrs = [
        "glfw/glfw3.h",
        "glfw/glfw3native.h",
    ],
    strip_include_prefix = "glfw",
    include_prefix = "GLFW",
    visibility = ["//visibility:public"],
)

cc_library(
    name = "afterhours_headers",
    hdrs = glob([
        "afterhours/**/*.h",
    ]),
    strip_include_prefix = "afterhours",
    include_prefix = "afterhours",
    visibility = ["//visibility:public"],
)
