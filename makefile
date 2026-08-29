
# Compiling and linking live in build.zig. What's left here is shell glue:
# resources, screenshots, profiling, counting. Source discovery, incremental
# rebuilds and header dependencies are all zig's problem now.
#
# For Apple clang + AddressSanitizer (zig ships no macOS asan runtime), the
# makefile is no longer the place -- build that variant by hand.

ZIG_FLAGS :=
ifdef MCP
	ZIG_FLAGS += -Dmcp=true
endif
ifdef E2E
	ZIG_FLAGS += -De2e=true
endif

# TARGET=windows cross-compiles a .exe from macOS. Default: native.
ifeq ($(TARGET),windows)
	OUTPUT_FOLDER := output-win
	OUTPUT_EXE := zig-out/bin/kart.exe
	ZIG_FLAGS += -Dtarget=x86_64-windows-gnu
	run_cmd = @echo "built $(OUTPUT_EXE), run it on Windows"
else
	OUTPUT_FOLDER := output
	OUTPUT_EXE := zig-out/bin/kart
	run_cmd = ./$(OUTPUT_EXE)
endif

mkdir_cmd = mkdir -p $(OUTPUT_FOLDER)/resources/
cp_lib_cmd = cp vendor/raylib/*.dll $(OUTPUT_FOLDER)/
cp_resources_cmd = cp -r resources/* $(OUTPUT_FOLDER)/resources/


.PHONY: all build clean output count countall old xmake e2e clean-screenshots windows
.PHONY: update-baselines validate-screenshots ci dirs sign run


all: build

build:
	zig build $(ZIG_FLAGS)

xmake:
	xmake build

old: build

dirs:
	$(mkdir_cmd)

clean:
	rm -rf zig-out .zig-cache
	$(mkdir_cmd)

windows:
	$(MAKE) TARGET=windows

clean-screenshots:
	@mkdir -p screenshots
	rm -f screenshots/*.png

e2e: clean-screenshots
	@mkdir -p screenshots
	$(MAKE) E2E=1 build
	./$(OUTPUT_EXE) --e2e --headless

BASELINE_DIR := screenshot-baselines/screens
VALIDATE_DIR := /tmp/kart-screenshot-validate

update-baselines:
	$(MAKE) E2E=1 build
	@mkdir -p $(BASELINE_DIR)
	@rm -f $(BASELINE_DIR)/*.png
	./$(OUTPUT_EXE) --e2e --headless
	@cp screenshots/*.png $(BASELINE_DIR)/ 2>/dev/null || true
	@echo ""
	@echo "Baselines updated in $(BASELINE_DIR)/."
	@echo "Review changes with: git diff --stat screenshot-baselines/"
	@echo "Then commit: git add screenshot-baselines/ and git commit -m 'update screenshot baselines'"

validate-screenshots:
	$(MAKE) E2E=1 build
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

run: build
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

