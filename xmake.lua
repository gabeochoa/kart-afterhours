


set_toolset("cxx", "clang")
set_toolset("ld", "clang++")
set_languages("c++23")

add_cxxflags([[
    -g
    -fmax-errors=10 
    -Wall -Wextra -Wpedantic
    -Wuninitialized -Wshadow -Wconversion
]]
)
add_cxxflags([[
    -Wno-deprecated-volatile -Wno-missing-field-initializers
    -Wno-c99-extensions -Wno-unused-function -Wno-sign-conversion 
    -Wno-implicit-int-float-conversion -Werror
]]
)

target("kart.exe")
    --
    set_kind("binary")
    --
    add_files("src/*.cpp")
    --
    add_includedirs("vendor")
    add_ldflags("-L.", "-Lvendor/", "$(shell pkg-config --libs raylib)")

    after_build(function(target)
        os.exec("xmake run kart.exe")
    end)
