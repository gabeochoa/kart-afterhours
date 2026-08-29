#!/usr/bin/env python3
"""Pack the control-prompt PNGs into one texture atlas.

They were loaded as 332 individual textures at startup -- 332 file opens and
332 GPU textures, none of which can batch with each other. They are all 64x64,
so a plain grid pack is optimal and there is no reason to bin-pack.

Emits:
  resources/images/controls_atlas.png   one texture
  resources/images/controls_atlas.json  name -> {x,y,w,h}

Run: python3 scripts/pack_atlas.py
"""

import json
import math
import sys
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parent.parent
SRC_DIRS = [
    ROOT / "resources/images/controls/keyboard_default",
    ROOT / "resources/images/controls/xbox_default",
]
OUT_PNG = ROOT / "resources/images/controls_atlas.png"
OUT_JSON = ROOT / "resources/images/controls_atlas.json"


def main() -> int:
    pngs = sorted(p for d in SRC_DIRS for p in d.glob("*.png"))
    if not pngs:
        print("no source pngs found", file=sys.stderr)
        return 1

    sizes = {Image.open(p).size for p in pngs}
    if len(sizes) != 1:
        print(f"expected uniform tiles, got {sizes}", file=sys.stderr)
        return 1
    tile_w, tile_h = sizes.pop()

    # Square-ish grid, rounded up to a power of two so older GL paths are happy.
    cols = math.ceil(math.sqrt(len(pngs)))
    rows = math.ceil(len(pngs) / cols)
    side = 1
    while side < max(cols * tile_w, rows * tile_h):
        side *= 2

    atlas = Image.new("RGBA", (side, side), (0, 0, 0, 0))
    index = {}
    for i, p in enumerate(pngs):
        x = (i % cols) * tile_w
        y = (i // cols) * tile_h
        atlas.paste(Image.open(p).convert("RGBA"), (x, y))
        # Key on the stem, which is what TextureLibrary used as the name.
        index[p.stem] = {"x": x, "y": y, "w": tile_w, "h": tile_h}

    atlas.save(OUT_PNG, optimize=True)
    OUT_JSON.write_text(json.dumps(index, indent=0, sort_keys=True) + "\n")

    before = sum(p.stat().st_size for p in pngs)
    after = OUT_PNG.stat().st_size
    print(f"packed {len(pngs)} tiles of {tile_w}x{tile_h} into {side}x{side}")
    print(f"  textures: {len(pngs)} -> 1")
    print(f"  on disk:  {before/1024:.0f}KB -> {after/1024:.0f}KB")
    print(f"  {OUT_PNG.relative_to(ROOT)}")
    print(f"  {OUT_JSON.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
