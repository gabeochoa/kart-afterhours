#!/usr/bin/env python3
"""CORRECT guards (see CORRECT.md). Run: python3 scripts/check_correct.py [src_dir]"""
import pathlib, re, sys
src = pathlib.Path(sys.argv[1] if len(sys.argv)>1 else "src")
bad=[]
def read(p): return p.read_text(errors="ignore") if p.exists() else ""
systems = read(src/"systems/systems.h")
_sprite = systems.split("struct RenderSpritesWithShaders",1)[-1].split("\nstruct ",1)[0] if "struct RenderSpritesWithShaders" in systems else ""
if "shader_batches" in _sprite and re.search(r"virtual void once\s*\(", _sprite):
    bad.append("CORRECT-K1 systems/systems.h: sprite batch (raw component pointers) rendered from once(); once() runs BEFORE the entity loop - use after() (53fa0e9, e2e 14 segfault)")
main = read(src/"main.cpp")
if "has_failed() ? 1 : 0" not in main:
    bad.append("CORRECT-K2 main.cpp: e2e result not returned as exit code (has_failed() ? 1 : 0); a failing suite would exit 0")
if "Settings::autosave_enabled = false" not in main or "load_defaults" not in main:
    bad.append("CORRECT-K2 main.cpp: --e2e must set Settings::autosave_enabled=false and load_defaults (never read/write the player save file, c77151e)")
settings = read(src/"settings.cpp")
if "if (!Settings::autosave_enabled)" not in settings:
    bad.append("CORRECT-K2 settings.cpp: write_save_file/save_if_changed must early-return when autosave_enabled is false")
print("\n".join(bad) if bad else "check_correct: OK")
sys.exit(1 if bad else 0)
