
RAYLIB_FLAGS := `pkg-config --cflags raylib`
RAYLIB_LIB := `pkg-config --libs raylib`

RELEASE_FLAGS = -std=c++2c $(RAYLIB_FLAGS)

FLAGS = -Wall -Wextra -Wpedantic -Wuninitialized -Wshadow \
		-Wconversion -g $(RAYLIB_FLAGS)

NOFLAGS = -Wno-deprecated-volatile -Wno-missing-field-initializers \
		  -Wno-c99-extensions -Wno-unused-function -Wno-sign-conversion \
		  -Wno-implicit-int-float-conversion -Werror
INCLUDES = -Ivendor/ -Isrc/
LIBS = -L. -Lvendor/ $(RAYLIB_LIB)

SRC_FILES := $(wildcard src/*.cpp src/**/*.cpp)
H_FILES := $(wildcard src/**/*.h src/**/*.hpp)
OBJ_DIR := ./output
OBJ_FILES := $(SRC_FILES:%.cpp=$(OBJ_DIR)/%.o)

DEPENDS := $(patsubst %.cpp,%.d,$(SOURCES))
-include $(DEPENDS)

OUTPUT_FOLDER:= output
OUTPUT_EXE := $(OUTPUT_FOLDER)/kart.exe

ifeq ($(OS),Windows_NT)
RAYLIB_FLAGS := -IF:/RayLib/include
RAYLIB_LIB := F:/RayLib/lib/raylib.dll
CXX := g++ -std=c++20

mkdir_cmd = powershell -command "& {&'New-Item' -Path .\ -Name output\resources -ItemType directory -ErrorAction SilentlyContinue}";
cp_lib_cmd = powershell -command  "& {&'Copy-Item' .\vendor\raylib\*.dll output -ErrorAction SilentlyContinue}";
cp_resources_cmd = powershell -command  "& {&'Copy-Item' .\resources\* output\resources -ErrorAction SilentlyContinue}";
run_cmd := powershell -command "& {&'$(OUTPUT_FOLDER)/kart.exe'}";
sign_cmd:=

else
mkdir_cmd = mkdir -p output/resources/
cp_lib_cmd = cp vendor/raylib/*.dll output/
cp_resources_cmd = cp resources/* output/resources/
run_cmd := ./${OUTPUT_EXE}
sign_cmd := && codesign -s - -f --verbose --entitlements ent.plist $(OUTPUT_EXE)
# CXX := /Users/gabeochoa/homebrew/Cellar/gcc/14.2.0_1/bin/g++-14
# CXX := clang++ -std=c++2a -Wmost #-fsanitize=undefined
CXX := g++-14 -fmax-errors=10 -std=c++2a -DBACKWARD


endif


.PHONY: all clean output count countall


all:
	$(CXX) $(FLAGS) $(INCLUDES) $(LIBS) src/main.cpp -o $(OUTPUT_EXE) $(sign_cmd) && $(run_cmd)

output:
	$(mkdir_cmd)
	$(cp_lib_cmd)
	$(cp_resources_cmd)

run: 
	$(mkdir_cmd)
	$(cp_resources_cmd)
	$(run_cmd)

count: 
	git ls-files | grep "src" | grep -v "resources" | grep -v "vendor" | xargs wc -l | sort -rn | pr -2 -t -w 100
	make -C vendor/afterhours

countall: 
	git ls-files | xargs wc -l | sort -rn
