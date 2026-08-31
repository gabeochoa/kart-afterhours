
# Compiling and linking live in build.zig. What's left here is shell glue:
# resources, screenshots, profiling, counting. Source discovery, incremental
# rebuilds and header dependencies are all zig's problem now.
#
# For Apple clang + AddressSanitizer (zig ships no macOS asan runtime), the
# makefile is no longer the place -- build that variant by hand.

ZIG_FLAGS :=

# Builds are Debug by default: -O0 plus zig's UBSan instrumentation. That is
# ~2.8x slower than release (e2e suite 35s vs 12s) and a 47MB binary vs 4.5MB.
# Use RELEASE=1 for anything where speed matters -- profiling, playtesting,
# shipping. Debug stays the default so asserts and UBSan keep catching things.
ifdef RELEASE
	ZIG_FLAGS += -Doptimize=ReleaseFast
endif

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


.PHONY: all build clean output count countall old e2e clean-screenshots windows shrink-baselines
.PHONY: update-baselines validate-screenshots ci dirs sign run cppcheck prof leak alloc


all: build

build:
	zig build $(ZIG_FLAGS)

old: build

dirs:
	$(mkdir_cmd)

clean:
	rm -rf zig-out .zig-cache output output-win

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

# Baselines go into git, so shrink them: drop the fully-opaque alpha channel
# and quantize to a 256-colour palette. compare_baselines.py converts both
# sides to RGB anyway, and the UI is flat colour -- measured error against the
# true capture is 0.0035% mean / 0.0123% worst, against a 1.0% threshold.
# 47MB -> ~15MB.
shrink-baselines:
	@python3 -c "from PIL import Image; import glob; [Image.open(f).convert('RGB').quantize(colors=256).save(f, optimize=True) for f in glob.glob('$(BASELINE_DIR)/*.png')]"

update-baselines:
	$(MAKE) E2E=1 build
	@mkdir -p $(BASELINE_DIR)
	@rm -f $(BASELINE_DIR)/*.png
	./$(OUTPUT_EXE) --e2e --headless
	@cp screenshots/*.png $(BASELINE_DIR)/ 2>/dev/null || true
	@$(MAKE) --no-print-directory shrink-baselines
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

output: build
	$(mkdir_cmd)
	$(cp_resources_cmd)
	cp $(OUTPUT_EXE) $(OUTPUT_FOLDER)/
ifeq ($(TARGET),windows)
	$(cp_lib_cmd)
endif

sign:
	codesign -s - -f --verbose --entitlements ent.plist $(OUTPUT_EXE)

run: build
	$(mkdir_cmd)
	$(cp_resources_cmd)
	$(run_cmd)

count:
	git ls-files | grep "src" | grep -v "resources" | grep -v "vendor" | xargs wc -l | sort -rn | pr -2 -t -w 100
	make -C vendor/afterhours

countall: 
	git ls-files | xargs wc -l | sort -rn

cppcheck:
	cppcheck src/ -Ivendor/afterhours --enable=all --std=c++23 --language=c++ --suppress=noConstructor --suppress=noExplicitConstructor --suppress=useStlAlgorithm --suppress=unusedStructMember --suppress=useInitializationList --suppress=duplicateCondition --suppress=nullPointerRedundantCheck --suppress=cstyleCast

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


.PHONY: deps deps-svg

# Ad-hoc analysis only. tools/Makefile builds the binary first, so both targets
# work on a clean checkout. deps-svg needs graphviz 'dot' on PATH.
deps:
	cd tools && $(MAKE) run

deps-svg:
	cd tools && $(MAKE) && ./dependency_graph --src ../src --main ../src/main.cpp --outdir ../output --svg

