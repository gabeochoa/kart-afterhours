
RAYLIB_FLAGS := `pkg-config --cflags raylib`
RAYLIB_LIB := `pkg-config --libs raylib`

FLAGS = -Wall -Wextra -Wpedantic -Wuninitialized -Wshadow \
		-Wconversion -g 
NOFLAGS = -Wno-deprecated-volatile -Wno-missing-field-initializers \
		  -Wno-c99-extensions -Wno-unused-function -Wno-sign-conversion \
		  -Wno-implicit-int-float-conversion

INCLUDES = -Ivendor/ -Isrc/
LIBS = -L. -Lvendor/ $(RAYLIB_LIB)

H_FILES := $(wildcard src/**/*.h src/**/*.hpp)
SRC_FILES := $(wildcard src/*.cpp src/**/*.cpp vendor/afterhours/src/plugins/*.cpp)
OBJ_DIR := ./output
OBJ_FILES := $(SRC_FILES:%.cpp=$(OBJ_DIR)/%.o)

DEPENDS := $(patsubst %.cpp,%.d,$(SOURCES))
-include $(DEPENDS)

OUTPUT_FOLDER:= output
OUTPUT_EXE := $(OUTPUT_FOLDER)/kart.exe

ifeq ($(OS),Windows_NT)
	RAYLIB_FLAGS := -IF:/RayLib/include
	RAYLIB_LIB := F:/RayLib/lib/raylib.dll

	CXX := g++ -std=c++23
	# CXX := F:\llvm-mingw-20241217-msvcrt-x86_64\bin\g++.exe  -std=c++23

	mkdir_cmd = powershell -command "& {&'New-Item' -Path .\ -Name output\resources -ItemType directory -ErrorAction SilentlyContinue}";
	cp_lib_cmd = powershell -command  "& {&'Copy-Item' .\vendor\raylib\*.dll output -ErrorAction SilentlyContinue}";
	cp_resources_cmd = powershell -command  "& {&'Copy-Item' .\resources\* output\resources -ErrorAction SilentlyContinue}";
	run_cmd := powershell -command "& {&'$(OUTPUT_FOLDER)/kart.exe'}";
	sign_cmd:=
	COMPILE = $(CXX) $(FLAGS) $(INCLUDES) $(LIBS) src/main.cpp src/settings.cpp src/preload.cpp src/makers.cpp -o $(OUTPUT_EXE) $(sign_cmd) && $(run_cmd)
else
	mkdir_cmd = mkdir -p output/resources/
	cp_lib_cmd = cp vendor/raylib/*.dll output/
	cp_resources_cmd = cp -r resources/* output/resources/
	run_cmd := ./${OUTPUT_EXE}
	sign_cmd := && codesign -s - -f --verbose --entitlements ent.plist $(OUTPUT_EXE)
	# CXX := /Users/gabeochoa/homebrew/Cellar/gcc/14.2.0_1/bin/g++-14
	CXX := clang++ -std=c++23 -Wmost -fsanitize=address
	# CXX := g++-14 -fmax-errors=10 -std=c++23 -DBACKWARD
	FLAGS = -g $(RAYLIB_FLAGS) -ftime-trace 
	COMPILE = $(CXX) $(FLAGS) $(NOFLAGS) $(INCLUDES) $(LIBS) $(OBJ_FILES) -o $(OUTPUT_EXE)
endif


.PHONY: all clean output count countall old clean xmake


$(info SRC_FILES: $(SRC_FILES))
$(info OBJ_FILES: $(OBJ_FILES))


all: $(OUTPUT_EXE)

xmake:
	xmake build

old: $(OUTPUT_EXE)

$(OUTPUT_EXE): $(H_FILES) $(OBJ_FILES)
	$(CXX) $(FLAGS) $(LEAKFLAGS) $(NOFLAGS) $(INCLUDES) $(LIBS) $(OBJ_FILES) -o $(OUTPUT_EXE)

$(OBJ_DIR)/%.o: %.cpp makefile
	$(CXX) $(FLAGS) $(NOFLAGS) $(INCLUDES) -c $< -o $@ -MMD -MF $(@:.o=.d)

%.d: %.cpp
  $(MAKEDEPEND)

clean:
	rm -rf output/src/
	mkdir -p output/src/ui output/vendor/afterhours/src/plugins

output:
	$(mkdir_cmd)
	$(cp_lib_cmd)
	$(cp_resources_cmd)

sign:
	codesign -s - -f --verbose --entitlements ent.plist $(OUTPUT_EXE)

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

# ClangBuildAnalyzer integration
cba:
	@echo "Building with xmake to generate trace data..."
	xmake build
	@echo "Analyzing build performance..."
	ClangBuildAnalyzer --all build/.objs/kart/macosx/arm64/debug/src/ build-analysis.html
	ClangBuildAnalyzer --analyze build-analysis.html | tee build-analysis.txt
	@echo ""
	@echo "Top 5 slowest files to parse:"
	@head -15 build-analysis.txt | grep -A 10 "Files that took longest to parse" || true

clean-cba:
	rm -f build-analysis.html build-analysis.txt
	@echo "Analysis files cleaned"

prof:
	$(mkdir_cmd)
	$(cp_resources_cmd)
	codesign -s - -f --verbose --entitlements ent.plist $(OUTPUT_EXE)
	rm -rf recording.trace/
	xctrace record --template 'Time Profiler' --output 'recording.trace' --launch $(OUTPUT_EXE)

leak:
	$(mkdir_cmd)
	$(cp_resources_cmd)
	codesign -s - -f --verbose --entitlements ent.plist $(OUTPUT_EXE)
	rm -rf recording.trace/
	xctrace record --template 'Leaks' --output 'recording.trace' --launch $(OUTPUT_EXE)

alloc:
	$(mkdir_cmd)
	$(cp_resources_cmd)
	codesign -s - -f --verbose --entitlements ent.plist $(OUTPUT_EXE)
	rm -rf recording.trace/
	xctrace record --template 'Allocations' --output 'recording.trace' --launch $(OUTPUT_EXE)


getxm: 
	powershell -command  "Invoke-Expression (Invoke-Webrequest 'https://xmake.io/psget.text' -UseBasicParsing).Content"
xm:
	xmake create -l c++ -t module.binary kart.exe

.PHONY: deps deps-html deps-check deps-dot deps-svg cba clean-cba

deps:
	cd tools && make run

# Generate DOT files for visualization
deps-dot:
	cd tools && ./dependency_graph --src ../src --main ../src/main.cpp --outdir ../output

# Generate SVG files from DOT files (requires graphviz)
deps-svg:
	cd tools && ./dependency_graph --src ../src --main ../src/main.cpp --outdir ../output --svg

# Legacy Python target (commented out since Python tool doesn't exist)
# deps-python:
# 	python3 tools/dependency_graph.py --src src --main src/main.cpp --outdir build

# Requires graphviz 'dot' on PATH
deps-html:
	cd tools && ./dependency_graph --src ../src --main ../src/main.cpp --outdir ../output

# Create or update baseline: cp output/dependency_summary.json tools/dependency_baseline.json
# Fails if current summary differs from baseline
deps-check: deps
	@echo "Checking dependency graph against baseline..."
	@[ -f tools/dependency_baseline.json ] || (echo "No baseline found at tools/dependency_baseline.json" && exit 2)
	@diff -u tools/dependency_baseline.json output/dependency_summary.json || (echo "Dependency summary changed. Run 'make deps' and update baseline if intentional." && exit 1)

