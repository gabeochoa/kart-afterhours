
FLAGS = -Wall -Wextra -Wpedantic -Wuninitialized -Wshadow \
		-Wconversion -g
NOFLAGS = -Wno-deprecated-volatile -Wno-missing-field-initializers \
		  -Wno-c99-extensions -Wno-unused-function -Wno-sign-conversion \
		  -Wno-implicit-int-float-conversion

ifdef MCP
	MCP_FLAGS = -DAFTER_HOURS_ENABLE_MCP
else
	MCP_FLAGS =
endif

ifdef E2E
	E2E_FLAGS = -DAFTER_HOURS_ENABLE_E2E_TESTING
else
	E2E_FLAGS =
endif

INCLUDES = -Ivendor/ -Isrc/ -DAFTER_HOURS_USE_RAYLIB -DAFTER_HOURS_UI_SINGLE_COLLECTION

H_FILES := $(wildcard src/**/*.h src/**/*.hpp)
SRC_FILES := $(wildcard src/*.cpp src/**/*.cpp vendor/afterhours/src/plugins/*.cpp)

# zig c++ cross-compiles to any target from any host. For Apple clang +
# AddressSanitizer (zig ships no macOS asan runtime), override on the CLI:
#   make CXX="clang++ -std=c++23 -Wmost -fsanitize=address"
# (?= would not fire here: make ships a built-in CXX default.)
ifeq ($(origin CXX),default)
CXX := zig c++ -std=c++23 -Wmost
endif

# TARGET=windows cross-compiles a .exe from macOS. Default: native.
ifeq ($(TARGET),windows)
	OUTPUT_FOLDER := output-win
	RAYLIB_FLAGS := -Ivendor/raylib
	RAYLIB_LIB := vendor/raylib/libraylibdll.a -lopengl32 -lgdi32 -lwinmm
	FLAGS = -target x86_64-windows-gnu -g $(RAYLIB_FLAGS)
	sign_cmd :=
	run_cmd := @echo "built $(OUTPUT_EXE), run it on Windows"
else
	OUTPUT_FOLDER := output
	RAYLIB_FLAGS := $(shell pkg-config --cflags raylib)
	RAYLIB_LIB := $(shell pkg-config --libs raylib) -framework OpenGL
	FLAGS = -g $(RAYLIB_FLAGS) -ftime-trace
	sign_cmd := && codesign -s - -f --verbose --entitlements ent.plist $(OUTPUT_EXE)
	run_cmd := ./${OUTPUT_EXE}
endif

LIBS = -L. -Lvendor/ $(RAYLIB_LIB)
OBJ_DIR := ./$(OUTPUT_FOLDER)
OUTPUT_EXE := $(OUTPUT_FOLDER)/kart.exe
OBJ_FILES := $(SRC_FILES:%.cpp=$(OBJ_DIR)/%.o)

# The .d files the compile rule emits were never included, so header edits
# didn't trigger rebuilds.
-include $(OBJ_FILES:.o=.d)

mkdir_cmd = mkdir -p $(OUTPUT_FOLDER)/resources/ $(sort $(dir $(OBJ_FILES)))
cp_lib_cmd = cp vendor/raylib/*.dll $(OUTPUT_FOLDER)/
cp_resources_cmd = cp -r resources/* $(OUTPUT_FOLDER)/resources/


.PHONY: all clean output count countall old clean xmake e2e clean-screenshots windows
.PHONY: update-baselines validate-screenshots ci


$(info SRC_FILES: $(SRC_FILES))
$(info OBJ_FILES: $(OBJ_FILES))


all: $(OUTPUT_EXE)

xmake:
	xmake build

old: $(OUTPUT_EXE)

$(OUTPUT_EXE): $(H_FILES) $(OBJ_FILES)
	$(CXX) $(FLAGS) $(LEAKFLAGS) $(NOFLAGS) $(MCP_FLAGS) $(E2E_FLAGS) $(INCLUDES) $(LIBS) $(OBJ_FILES) -o $(OUTPUT_EXE)

dirs:
	$(mkdir_cmd)

$(OBJ_DIR)/%.o: %.cpp makefile | dirs
	$(CXX) $(FLAGS) $(NOFLAGS) $(MCP_FLAGS) $(E2E_FLAGS) $(INCLUDES) -c $< -o $@ -MMD -MF $(@:.o=.d)

%.d: %.cpp
  $(MAKEDEPEND)

clean:
	rm -rf $(OBJ_DIR)/src/ $(OBJ_DIR)/vendor/
	$(mkdir_cmd)

windows:
	$(MAKE) TARGET=windows

clean-screenshots:
	@mkdir -p screenshots
	rm -f screenshots/*.png

e2e: clean clean-screenshots
	@mkdir -p screenshots
	$(MAKE) E2E=1 $(OUTPUT_EXE)
	./$(OUTPUT_EXE) --e2e --headless

BASELINE_DIR := screenshot-baselines/screens
VALIDATE_DIR := /tmp/kart-screenshot-validate

update-baselines: clean
	$(MAKE) E2E=1 $(OUTPUT_EXE)
	@mkdir -p $(BASELINE_DIR)
	@rm -f $(BASELINE_DIR)/*.png
	./$(OUTPUT_EXE) --e2e --headless
	@cp screenshots/*.png $(BASELINE_DIR)/ 2>/dev/null || true
	@echo ""
	@echo "Baselines updated in $(BASELINE_DIR)/."
	@echo "Review changes with: git diff --stat screenshot-baselines/"
	@echo "Then commit: git add screenshot-baselines/ and git commit -m 'update screenshot baselines'"

validate-screenshots: clean
	$(MAKE) E2E=1 $(OUTPUT_EXE)
	@mkdir -p $(VALIDATE_DIR)
	@rm -f $(VALIDATE_DIR)/*.png
	./$(OUTPUT_EXE) --e2e --headless
	@cp screenshots/*.png $(VALIDATE_DIR)/ 2>/dev/null || true
	@echo ""
	@echo "Comparing against baselines..."
	python3 scripts/compare_baselines.py $(BASELINE_DIR) $(VALIDATE_DIR)

ci: validate-screenshots
	@echo "CI passed."

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

