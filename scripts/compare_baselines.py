#!/usr/bin/env python3
"""
Compare current screenshots against committed baselines.

Usage:
    python3 scripts/compare_baselines.py
    python3 scripts/compare_baselines.py <baseline_dir> <current_dir>
    python3 scripts/compare_baselines.py --threshold=2.0

Exit codes:
    0 - All screenshots match baselines
    1 - Some screenshots differ or are missing
"""

import argparse
import hashlib
import json
import sys
from pathlib import Path

try:
    from PIL import Image, ImageChops
    HAS_PIL = True
except ImportError:
    HAS_PIL = False

DEFAULT_BASELINE_DIR = "screenshot-baselines/screens"
DEFAULT_CURRENT_DIR = "/tmp/kart-screenshot-validate"
DEFAULT_THRESHOLD = 1.0
DEFAULT_FAILURES_DIR = "test-failures"


def compare_images_pil(baseline: Path, current: Path) -> float:
    img1 = Image.open(baseline).convert("RGB")
    img2 = Image.open(current).convert("RGB")

    if img1.size != img2.size:
        return 100.0

    diff = ImageChops.difference(img1, img2)
    diff_pixels = list(diff.getdata())
    total_diff = sum(sum(p) for p in diff_pixels)
    max_diff = len(diff_pixels) * 255 * 3
    return (total_diff / max_diff) * 100


def compare_images_hash(baseline: Path, current: Path) -> float:
    h1 = hashlib.sha256(baseline.read_bytes()).hexdigest()
    h2 = hashlib.sha256(current.read_bytes()).hexdigest()
    return 0.0 if h1 == h2 else 100.0


def compare_images(baseline: Path, current: Path) -> float:
    if HAS_PIL:
        return compare_images_pil(baseline, current)
    return compare_images_hash(baseline, current)


def create_diff_image(baseline: Path, current: Path, output: Path):
    if not HAS_PIL:
        return
    img1 = Image.open(baseline).convert("RGB")
    img2 = Image.open(current).convert("RGB")
    if img1.size != img2.size:
        img2 = img2.resize(img1.size, Image.Resampling.LANCZOS)
    diff = ImageChops.difference(img1, img2)
    diff = diff.point(lambda x: min(255, x * 5))
    output.parent.mkdir(parents=True, exist_ok=True)
    diff.save(str(output), "PNG")


def load_manifest(baseline_dir: Path) -> dict:
    manifest_path = baseline_dir.parent / "manifest.json"
    if manifest_path.exists():
        with open(manifest_path) as f:
            return json.load(f)
    return {}


def main():
    parser = argparse.ArgumentParser(description="Compare screenshots against baselines")
    parser.add_argument("baseline_dir", nargs="?", default=DEFAULT_BASELINE_DIR)
    parser.add_argument("current_dir", nargs="?", default=DEFAULT_CURRENT_DIR)
    parser.add_argument("--threshold", type=float, default=DEFAULT_THRESHOLD)
    parser.add_argument("--json", type=str, default=None)
    parser.add_argument("--save-diffs", action="store_true", default=True)
    parser.add_argument("--failures-dir", type=str, default=DEFAULT_FAILURES_DIR)
    args = parser.parse_args()

    baseline_dir = Path(args.baseline_dir)
    current_dir = Path(args.current_dir)
    failures_dir = Path(args.failures_dir)

    if not baseline_dir.exists():
        print(f"ERROR: Baseline directory not found: {baseline_dir}")
        print("Run 'make update-baselines' to generate baselines first.")
        sys.exit(1)

    if not current_dir.exists():
        print(f"ERROR: Current screenshots directory not found: {current_dir}")
        sys.exit(1)

    manifest = load_manifest(baseline_dir)
    overrides = manifest.get("overrides", {})
    default_tolerance = manifest.get("default_tolerance", args.threshold)

    baselines = sorted(baseline_dir.glob("*.png"))
    if not baselines:
        print(f"No baselines found in {baseline_dir}")
        sys.exit(1)

    if not HAS_PIL:
        print("Warning: PIL not found. Using hash comparison (exact match only).")
        print("Install with: pip3 install Pillow")

    passed = 0
    failed = 0
    missing = 0
    failures = []

    for baseline in baselines:
        current = current_dir / baseline.name
        screen_name = baseline.stem
        threshold = overrides.get(screen_name, default_tolerance)

        if not current.exists():
            print(f"  MISSING  {baseline.name}")
            missing += 1
            failures.append({"name": screen_name, "reason": "missing"})
            continue

        diff_pct = compare_images(baseline, current)

        if diff_pct <= threshold:
            print(f"  PASS     {baseline.name} ({diff_pct:.4f}%)")
            passed += 1
        else:
            print(f"  FAIL     {baseline.name} ({diff_pct:.4f}% > {threshold}%)")
            failed += 1
            failures.append({
                "name": screen_name,
                "diff_percent": round(diff_pct, 4),
                "threshold": threshold,
                "baseline": str(baseline),
                "current": str(current),
            })

            if args.save_diffs:
                diff_path = failures_dir / f"{screen_name}_diff.png"
                create_diff_image(baseline, current, diff_path)
                failures[-1]["diff"] = str(diff_path)

    current_names = {p.name for p in current_dir.glob("*.png")}
    baseline_names = {p.name for p in baselines}
    new_screens = sorted(current_names - baseline_names)
    if new_screens:
        print(f"\n  NEW (no baseline): {', '.join(new_screens)}")
        print("  Run 'make update-baselines' to add them.")

    print(f"\n{passed} passed, {failed} failed, {missing} missing")

    if failures:
        print("\nFailed screenshots:")
        for f in failures:
            reason = f.get("reason", f"{f.get('diff_percent', '?')}%")
            print(f"  - {f['name']}: {reason}")

    if args.json:
        summary = {
            "passed": passed,
            "failed": failed,
            "missing": missing,
            "total": passed + failed + missing,
            "threshold": args.threshold,
            "failures": failures,
        }
        json_path = Path(args.json)
        json_path.parent.mkdir(parents=True, exist_ok=True)
        with open(json_path, "w") as f:
            json.dump(summary, f, indent=2)
        print(f"\nSummary written to: {args.json}")

    sys.exit(1 if (failed > 0 or missing > 0) else 0)


if __name__ == "__main__":
    main()
