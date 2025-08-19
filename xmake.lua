


if is_host("windows") then 
    set_toolset("cxx", "g++")
    set_toolset("ld", "g++")
else
    set_toolset("cxx", "clang++")
    set_toolset("ld", "clang++")
end
set_languages("c++23")

add_cxxflags([[
    -g
    -fmax-errors=10 
    -Wall -Wextra -Wpedantic
    -Wuninitialized -Wshadow -Wconversion
    -pipe
    -fno-stack-protector
    -fno-common
]]
)
add_cxxflags([[
    -Wno-deprecated-volatile -Wno-missing-field-initializers
    -Wno-c99-extensions -Wno-unused-function -Wno-sign-conversion 
    -Wno-implicit-int-float-conversion -Wno-implicit-float-conversion
]]
)

if is_host("macosx") then
    set_extension(".exe")
    add_cxxflags("-DBACKWARD")
end

target("kart")
    --
    set_kind("binary")
    set_targetdir("output")
    --
    add_files("src/*.cpp")
    --
    add_includedirs("vendor")

    add_ldflags("-L.", "-Lvendor/")
    if is_host("windows") then
        add_ldflags("F:/RayLib/lib/raylib.dll")
    else
        add_ldflags("$(shell pkg-config --libs raylib)")
    end

    -- after_build(function(target)
    --     os.exec("./output/kart.exe")
    -- end)
