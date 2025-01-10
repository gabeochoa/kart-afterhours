
RAYLIB_FLAGS := `pkg-config --cflags raylib`
RAYLIB_LIB := `pkg-config --libs raylib`

FLAGS = -Wall -Wextra -Wpedantic -Wuninitialized -Wshadow \
		-Wconversion -g 
NOFLAGS = -Wno-deprecated-volatile -Wno-missing-field-initializers \
		  -Wno-c99-extensions -Wno-unused-function -Wno-sign-conversion \
		  -Wno-implicit-int-float-conversion -Werror

INCLUDES = -Ivendor/ -Isrc/
LIBS = -L. -Lvendor/ $(RAYLIB_LIB)

H_FILES := $(wildcard src/**/*.h src/**/*.hpp)
SRC_FILES := $(wildcard src/*.cpp src/**/*.cpp)
OBJ_DIR := ./output
OBJ_FILES := $(SRC_FILES:%.cpp=$(OBJ_DIR)/%.o)

DEPENDS := $(patsubst %.cpp,%.d,$(SOURCES))
-include $(DEPENDS)

OUTPUT_FOLDER:= output
OUTPUT_EXE := $(OUTPUT_FOLDER)/kart.exe

ifeq ($(OS),Windows_NT)
	RAYLIB_FLAGS := -IF:/RayLib/include
	RAYLIB_LIB := F:/RayLib/lib/raylib.dll

	# CXX := g++ -std=c++23
	CXX := F:\llvm-mingw-20241217-msvcrt-x86_64\bin\g++.exe  -std=c++23

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
	# CXX := clang++ -std=c++23 -Wmost -fsanitize=address
	CXX := g++-14 -fmax-errors=10 -std=c++23 -DBACKWARD
	FLAGS = -g $(RAYLIB_FLAGS) 
endif


.PHONY: all clean output count countall new

all:
	$(CXX) $(FLAGS) $(INCLUDES) $(LIBS) src/main.cpp src/settings.cpp src/preload.cpp src/makers.cpp -o $(OUTPUT_EXE) $(sign_cmd) && $(run_cmd)


new: 
	xmake && xmake r

output:
	$(mkdir_cmd)
	$(cp_lib_cmd)
	$(cp_resources_cmd)

run: 
	$(mkdir_cmd)
	$(cp_resources_cmd)
	$(run_cmd)

brawlhalla: 
	cp $(OUTPUT_EXE) F:\SteamLibrary\steamapps\common\Brawlhalla\Brawlhalla.exe


count: 
	git ls-files | grep "src" | grep -v "resources" | grep -v "vendor" | xargs wc -l | sort -rn | pr -2 -t -w 100
	make -C vendor/afterhours

countall: 
	git ls-files | xargs wc -l | sort -rn

cppcheck:
	cppcheck src/ -Ivendor/afterhours --enable=all --std=c++23 --language=c++ --suppress=noConstructor --suppress=noExplicitConstructor --suppress=useStlAlgorithm --suppress=unusedStructMember --suppress=useInitializationList --suppress=duplicateCondition --suppress=nullPointerRedundantCheck --suppress=cstyleCast

xm:
	xmake create -l c++ -t module.binary kart.exe

