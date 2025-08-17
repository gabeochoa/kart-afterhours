


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
    -Wcast-qual -Wchar-subscripts
    -Wcomment -Wdisabled-optimization -Wformat=2
    -Wformat-nonliteral -Wformat-security -Wformat-y2k
    -Wimport -Winit-self -Winline -Winvalid-pch
    -Wlong-long -Wmissing-format-attribute
    -Wmissing-include-dirs
    -Wpacked -Wpointer-arith
    -Wreturn-type -Wsequence-point
    -Wstrict-overflow=5 -Wswitch -Wswitch-default
    -Wswitch-enum -Wtrigraphs 
    -Wunused-label -Wunused-parameter -Wunused-value
    -Wunused-variable -Wvariadic-macros -Wvolatile-register-var
    -Wwrite-strings -Warray-bounds
    -pipe
    -fno-stack-protector
    -fno-common
]]
)
add_cxxflags([[
    -Wno-deprecated-volatile -Wno-missing-field-initializers
    -Wno-c99-extensions -Wno-unused-function -Wno-sign-conversion 
    -Wno-implicit-int-float-conversion -Wno-implicit-float-conversion
    -Wno-format-nonliteral -Wno-format-security -Wno-format-y2k
    -Wno-import -Wno-inline -Wno-invalid-pch
    -Wno-long-long -Wno-missing-format-attribute
    -Wno-missing-noreturn -Wno-packed -Wno-redundant-decls
    -Wno-sequence-point -Wno-trigraphs -Wno-variadic-macros
    -Wno-volatile-register-var
]]
)

-- Suppress warnings from vendor directories (third-party code)
add_cxxflags("-isystem vendor")

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
    add_files("src/ui/*.cpp")
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
