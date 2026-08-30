#!/usr/bin/env python3
"""Generate placeholder sprite art (Memphis '93 style) into resources/images/placeholder.{png,json}.

Kept OUT of spritesheet.png on purpose: everything in this atlas is programmer
art waiting to be replaced by hand-drawn versions.

Style rules (see also the sprite spec in the design notes):
  - 3px solid ink outline on every shape
  - flat fills only, no gradients / no anti-aliasing / no soft edges
  - stripes instead of shading when a second tone is needed
  - palette below is the whole palette

Usage: python3 scripts/gen_placeholder_sprites.py [--preview /tmp/sprite_preview.png]
"""

import argparse
import json
import pathlib

from PIL import Image, ImageDraw

# --- palette -----------------------------------------------------------------
INK = "#120A2B"
WHITE = "#FFFFFF"
PLAYER = {
    "orchid": "#E06BDD",
    "sky": "#5BA8F0",
    "mint": "#4FD6A6",
    "butter": "#F0E85C",
    "coral": "#FF7A6B",
    "violet": "#A97BFF",
    "lime": "#B6E85C",
    "tangerine": "#FFA43C",
}
OUTLINE = 3
PREVIEW_BG = "#2E1B69"

KART_W, KART_H = 64, 96
WEAPON_W, WEAPON_H = 96, 72
BOX_W = BOX_H = 96


def new_sprite(w, h):
    img = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    return img, ImageDraw.Draw(img)


def stripes(d, box, color, gap=4, thick=4, vertical=False):
    """Second tone as stripes -- never gradients."""
    x0, y0, x1, y1 = box
    if vertical:
        for x in range(x0, x1, gap + thick):
            d.rectangle([x, y0, min(x + thick - 1, x1), y1], fill=color)
    else:
        for y in range(y0, y1, gap + thick):
            d.rectangle([x0, y, x1, min(y + thick - 1, y1)], fill=color)


# --- karts -------------------------------------------------------------------
def kart(color):
    img, d = new_sprite(KART_W, KART_H)
    # wheels first so the body chassis overlaps their inner edge
    for wy in (10, 60):
        for wx in (0, 48):
            d.rounded_rectangle([wx, wy, wx + 15, wy + 26], radius=4,
                                fill=INK, outline=INK, width=OUTLINE)
    # chassis
    d.rounded_rectangle([8, 2, KART_W - 9, KART_H - 3], radius=14,
                        fill=color, outline=INK, width=OUTLINE)
    # windshield band across the upper third
    d.rounded_rectangle([12, 20, KART_W - 13, 36], radius=5, fill=INK)
    # striped rear panel (white stripes over the body colour)
    panel = [16, 52, KART_W - 17, 84]
    d.rectangle(panel, fill=color)
    stripes(d, [panel[0] + OUTLINE, panel[1] + OUTLINE,
                panel[2] - OUTLINE, panel[3] - OUTLINE], WHITE, gap=4, thick=4)
    d.rectangle(panel, outline=INK, width=OUTLINE)
    # nose notch so "which end is the front" reads at a glance
    d.rectangle([26, 6, KART_W - 27, 14], fill=INK)
    return img


# --- weapons -----------------------------------------------------------------
def weapon_cannon():
    """Coral. Fat short barrel on a butter carriage wheel."""
    img, d = new_sprite(WEAPON_W, WEAPON_H)
    d.rectangle([68, 2, 92, 42], fill=PLAYER["tangerine"], outline=INK, width=OUTLINE)  # muzzle
    d.rounded_rectangle([8, 6, 74, 38], radius=8,
                        fill=PLAYER["coral"], outline=INK, width=OUTLINE)  # barrel
    stripes(d, [14, 12, 64, 32], WHITE, gap=6, thick=5)  # second tone, horizontal
    d.rounded_rectangle([8, 6, 74, 38], radius=8, outline=INK, width=OUTLINE)
    d.ellipse([18, 26, 62, 70], fill=PLAYER["butter"], outline=INK, width=OUTLINE)  # wheel
    d.ellipse([34, 42, 46, 54], fill=INK)  # hub
    return img


def weapon_sniper():
    """Sky. Long thin barrel, violet scope block, mint stock."""
    img, d = new_sprite(WEAPON_W, WEAPON_H)
    d.rectangle([4, 30, 82, 44], fill=PLAYER["sky"], outline=INK, width=OUTLINE)  # barrel
    d.rectangle([78, 24, 93, 50], fill=PLAYER["sky"], outline=INK, width=OUTLINE)  # muzzle brake
    d.rectangle([30, 8, 68, 32], fill=PLAYER["violet"], outline=INK, width=OUTLINE)  # scope
    stripes(d, [36, 14, 62, 26], WHITE, gap=5, thick=4, vertical=True)
    d.rectangle([30, 8, 68, 32], outline=INK, width=OUTLINE)
    d.polygon([(4, 36), (28, 36), (20, 66), (2, 66)], fill=PLAYER["mint"])  # stock / grip
    d.line([(4, 36), (28, 36), (20, 66), (2, 66), (4, 36)], fill=INK, width=OUTLINE)
    return img


def weapon_shotgun():
    """Mint. Two stacked barrels, tangerine stock, butter forend."""
    img, d = new_sprite(WEAPON_W, WEAPON_H)
    for y in (8, 28):  # double stacked barrels
        d.rectangle([16, y, 93, y + 18], fill=PLAYER["mint"], outline=INK, width=OUTLINE)
    d.rectangle([46, 42, 80, 56], fill=PLAYER["butter"], outline=INK, width=OUTLINE)  # forend
    stripes(d, [52, 46, 74, 52], WHITE, gap=5, thick=3, vertical=True)
    d.rectangle([46, 42, 80, 56], outline=INK, width=OUTLINE)
    d.polygon([(2, 32), (26, 26), (26, 62), (2, 56)], fill=PLAYER["tangerine"])  # stock
    d.line([(2, 32), (26, 26), (26, 62), (2, 56), (2, 32)], fill=INK, width=OUTLINE)
    return img


def weapon_machinegun():
    """Lime. Body, sky magazine, orchid vent block."""
    img, d = new_sprite(WEAPON_W, WEAPON_H)
    d.rectangle([6, 18, 93, 40], fill=PLAYER["lime"], outline=INK, width=OUTLINE)  # body
    d.rectangle([52, 4, 88, 20], fill=PLAYER["orchid"], outline=INK, width=OUTLINE)  # vent block
    for vx in range(58, 84, 7):  # vents = stripes, not shading
        d.rectangle([vx, 8, vx + 3, 16], fill=INK)
    d.rectangle([30, 34, 54, 68], fill=PLAYER["sky"], outline=INK, width=OUTLINE)  # magazine
    stripes(d, [36, 40, 48, 62], WHITE, gap=4, thick=4)
    d.rectangle([30, 34, 54, 68], outline=INK, width=OUTLINE)
    d.polygon([(6, 34), (26, 34), (20, 66), (4, 66)], fill=PLAYER["tangerine"])  # grip
    d.line([(6, 34), (26, 34), (20, 66), (4, 66), (6, 34)], fill=INK, width=OUTLINE)
    return img


# --- item box ----------------------------------------------------------------
def item_box():
    img, d = new_sprite(BOX_W, BOX_H)
    d.rounded_rectangle([2, 2, BOX_W - 3, BOX_H - 3], radius=12,
                        fill=PLAYER["butter"], outline=INK, width=OUTLINE)
    # "?" drawn from primitives so it stays flat and aliased (no font AA)
    d.arc([26, 12, 70, 56], start=180, end=90, fill=INK, width=11)
    d.rectangle([42, 46, 53, 68], fill=INK)   # stem
    d.rectangle([42, 74, 53, 86], fill=INK)   # dot
    return img


SPRITES = {
    **{f"kart_{name}": (lambda c=color: kart(c)) for name, color in PLAYER.items()},
    "weapon_cannon": weapon_cannon,
    "weapon_sniper": weapon_sniper,
    "weapon_shotgun": weapon_shotgun,
    "weapon_machinegun": weapon_machinegun,
    "item_box": item_box,
}


def pack(images, max_width=256, pad=2):
    """Shelf pack, tallest-first. ponytail: O(n) shelves is plenty for 13 sprites."""
    rects, x, y, shelf_h = {}, pad, pad, 0
    for name, img in sorted(images.items(), key=lambda kv: -kv[1].height):
        w, h = img.size
        if x + w + pad > max_width:
            x, y, shelf_h = pad, y + shelf_h + pad, 0
        rects[name] = {"x": x, "y": y, "w": w, "h": h}
        x += w + pad
        shelf_h = max(shelf_h, h)
    height = y + shelf_h + pad
    atlas = Image.new("RGBA", (max_width, height), (0, 0, 0, 0))
    for name, r in rects.items():
        atlas.paste(images[name], (r["x"], r["y"]))
    return atlas, rects


def contact_sheet(images, path, scale=2, cols=4, pad=12):
    cw = max(i.width for i in images.values()) * scale + pad
    ch = max(i.height for i in images.values()) * scale + pad
    rows = (len(images) + cols - 1) // cols
    sheet = Image.new("RGB", (cols * cw + pad, rows * ch + pad), PREVIEW_BG)
    for i, (name, img) in enumerate(images.items()):
        big = img.resize((img.width * scale, img.height * scale), Image.NEAREST)
        sheet.paste(big, (pad + (i % cols) * cw, pad + (i // cols) * ch), big)
    sheet.save(path)
    return sheet.size


def main():
    root = pathlib.Path(__file__).resolve().parent.parent
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", default=str(root / "resources" / "images" / "placeholder.png"))
    ap.add_argument("--preview", default="/tmp/sprite_preview.png")
    args = ap.parse_args()

    images = {name: fn() for name, fn in SPRITES.items()}
    atlas, rects = pack(images)

    png = pathlib.Path(args.out)
    atlas.save(png)
    index = png.with_suffix(".json")
    index.write_text(json.dumps(rects, indent=2, sort_keys=True) + "\n")

    for name, r in rects.items():
        assert r["x"] + r["w"] <= atlas.width and r["y"] + r["h"] <= atlas.height, name
    assert set(rects) == set(SPRITES)

    preview = contact_sheet(images, args.preview)
    print(f"{png} {atlas.width}x{atlas.height}")
    print(f"{index} {len(rects)} sprites")
    print(f"{args.preview} {preview[0]}x{preview[1]}")


if __name__ == "__main__":
    main()
