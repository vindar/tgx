#!/usr/bin/env python3
import csv
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
OUT = ROOT / "tmp" / "inline_granularity"
SRC = ROOT / "src"

MACROS = [
    "TGX_INLINE",
    "TGX_NOINLINE",
    "TGX_INLINE_ZDIVIDE",
    "TGX_SHADER_USE_INCREMENTAL_PIXEL_POINTERS",
    "TGX_USE_FAST_INV_TRICK",
    "TGX_USE_FAST_SQRT_TRICK",
    "TGX_USE_FAST_INV_SQRT_TRICK",
    "TGX_USE_FMA_MATH",
]

def rel(path):
    return str(path.relative_to(ROOT)).replace("\\", "/")

def classify_file(path, line_no, text):
    name = path.name
    if name == "Vec2.h":
        return "vec2", "per-vertex"
    if name == "Vec3.h":
        return "vec3", "per-vertex/lighting"
    if name == "Vec4.h":
        return "vec4", "per-vertex/clip"
    if name == "Color.h":
        if line_no >= 2500 or "RGBf" in text:
            return "rgbf", "lighting/color"
        return "pixel-color", "per-pixel"
    if name == "Misc.h":
        return "math", "mixed hot math"
    if name == "Shaders.h":
        return "shader", "per-pixel"
    if name == "Mat4.h":
        return "matrix", "per-vertex"
    if name.startswith("Renderer3D"):
        if any(s in text for s in ("_shade", "_clip", "drawPixelZbuf", "_powSpecular")):
            return "renderer-hot-helper", "per-vertex/per-pixel"
        if any(s in text for s in ("_update", "_setRuntime", "_precompute")):
            return "renderer-setup", "setup"
        return "renderer", "per-triangle/setup"
    if name.startswith("Image"):
        if any(s in text for s in ("drawPixel", "readPixel", "_drawPixel", "_drawFontPixel", "_bseg_update")):
            return "image-pixel", "per-pixel"
        if any(s in text for s in ("Decode", "JPEG", "PNG", "GIF")):
            return "image-codec", "cold"
        return "image", "mixed"
    if name == "bseg.h":
        return "bseg", "line-drawing"
    if name == "tgx_config.h":
        return "config", "configuration"
    return "other", "unknown"

def extract_function(text):
    stripped = " ".join(text.strip().split())
    m = re.search(r"(?:inline\s+)?(?:[\w:<>,&*\s]+\s+)?([~\w:]+)\s*\([^;{}]*\)", stripped)
    if m:
        return m.group(1)
    m = re.search(r"#\s*define\s+(\w+)", stripped)
    if m:
        return m.group(1)
    return ""

def tiny_guess(text):
    compact = " ".join(text.strip().split())
    if "{ return" in compact and len(compact) < 180:
        return "yes"
    if len(compact) < 120 and "{" in compact and "}" in compact:
        return "yes"
    if any(s in compact for s in ("interpolateColorsTriangle", "interpolateColorsBilinear", "normalize_fast", "fast_invsqrt", "zdivide")):
        return "no"
    return "maybe"

def likely_hot(category, path_type):
    if category in ("vec2", "vec3", "vec4", "rgbf", "pixel-color", "shader", "matrix", "renderer-hot-helper", "image-pixel", "math"):
        return "yes"
    if path_type in ("setup", "cold", "configuration"):
        return "no"
    return "maybe"

def risk_note(macro, category, tiny, text):
    if macro == "TGX_NOINLINE":
        return "keeps cold or large functions out of hot code"
    if macro == "TGX_INLINE_ZDIVIDE":
        return "already board-specific baseline; do not mix with generic vec4 tests"
    if macro == "TGX_INLINE" and tiny == "no" and category in ("rgbf", "pixel-color", "math", "renderer-hot-helper"):
        return "forced inline may alter code size/register pressure"
    if category in ("shader", "image-pixel"):
        return "per-pixel helper; local regressions matter more than global score"
    if category in ("vec2", "vec3", "vec4"):
        return "small object ABI/aliasing and inlining policy candidate"
    return ""

def gather_inline_usage():
    rows = []
    files = sorted([p for p in SRC.rglob("*") if p.suffix in (".h", ".inl", ".cpp", ".c")])
    pattern = re.compile("|".join(re.escape(m) for m in MACROS + ["always_inline", "noinline", "asm volatile"]))
    for path in files:
        try:
            lines = path.read_text(errors="ignore").splitlines()
        except UnicodeDecodeError:
            continue
        for i, line in enumerate(lines, start=1):
            if not pattern.search(line):
                continue
            macros = [m for m in MACROS if m in line]
            if "always_inline" in line and not macros:
                macros.append("always_inline")
            if "noinline" in line and not macros:
                macros.append("noinline")
            if "asm volatile" in line and not macros:
                macros.append("asm volatile")
            context = line
            if i < len(lines):
                context += " " + lines[i]
            category, path_type = classify_file(path, i, context)
            tiny = tiny_guess(context)
            for macro in macros:
                rows.append({
                    "file": rel(path),
                    "line": i,
                    "macro": macro,
                    "function_or_context": extract_function(context),
                    "category": category,
                    "tiny": tiny,
                    "likely_hot": likely_hot(category, path_type),
                    "path_type": path_type,
                    "risk_note": risk_note(macro, category, tiny, context),
                    "source": line.strip(),
                })
    return rows

def parse_config_inventory():
    path = SRC / "tgx_config.h"
    rows = []
    current = "initial"
    for i, line in enumerate(path.read_text(errors="ignore").splitlines(), start=1):
        stripped = line.strip()
        if stripped.startswith("#if") or stripped.startswith("#elif") or stripped.startswith("#else"):
            current = stripped
        m = re.match(r"#\s*define\s+(\w+)\s*(.*)$", stripped)
        if m and m.group(1) in MACROS + ["TGX_PROGMEM_DEFAULT_CACHE_SIZE"]:
            rows.append({
                "line": i,
                "condition": current,
                "macro": m.group(1),
                "value": m.group(2).strip(),
                "source": stripped,
            })
    return rows

def parse_macro_probe(board, telemetry_name):
    path = ROOT / "tmp" / "hardware_validation" / "telemetry" / telemetry_name
    data = {"board": board}
    if not path.exists():
        data["status"] = "missing"
        return data
    data["status"] = "probe_ok"
    for line in path.read_text(errors="ignore").splitlines():
        if "=" in line and not line.startswith("TGX_MACRO_PROBE"):
            k, v = line.split("=", 1)
            data[k.strip()] = v.strip()
    return data

def macro_matrix():
    probes = [
        ("Core2", "core2_macro_probe_kick1.txt"),
        ("CoreS3", "cores3_macro_probe_kick1.txt"),
        ("Pico2/RP2350", "pico2_macro_probe_kick1.txt"),
        ("Teensy4.1", "teensy41_macro_probe_kick1.txt"),
    ]
    rows = [parse_macro_probe(board, name) for board, name in probes]
    rows.append({
        "board": "PicoW/RP2040",
        "status": "upload_unavailable",
        "TGX_INLINE": "inferred empty from tgx_config.h",
        "TGX_INLINE_ZDIVIDE": "inferred empty from tgx_config.h",
        "TGX_SHADER_USE_INCREMENTAL_PIXEL_POINTERS": "inferred 0",
        "TGX_USE_FAST_INV_TRICK": "inferred 0",
        "TGX_USE_FAST_SQRT_TRICK": "inferred 0",
        "TGX_USE_FAST_INV_SQRT_TRICK": "inferred 0",
        "TGX_USE_FMA_MATH": "inferred 0",
    })
    rows.append({
        "board": "CPU",
        "status": "inferred",
        "TGX_INLINE": "empty",
        "TGX_INLINE_ZDIVIDE": "empty",
        "TGX_SHADER_USE_INCREMENTAL_PIXEL_POINTERS": "0",
        "TGX_USE_FAST_INV_TRICK": "0",
        "TGX_USE_FAST_SQRT_TRICK": "0",
        "TGX_USE_FAST_INV_SQRT_TRICK": "0",
        "TGX_USE_FMA_MATH": "0",
    })
    return rows

def write_csv(path, rows):
    path.parent.mkdir(parents=True, exist_ok=True)
    if not rows:
        path.write_text("")
        return
    fieldnames = []
    for row in rows:
        for key in row.keys():
            if key not in fieldnames:
                fieldnames.append(key)
    with path.open("w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        writer.writeheader()
        for row in rows:
            writer.writerow(row)

def main():
    write_csv(OUT / "inline_usage_inventory.csv", gather_inline_usage())
    write_csv(OUT / "config_macro_inventory.csv", parse_config_inventory())
    write_csv(OUT / "macro_matrix_baseline.csv", macro_matrix())
    print("wrote inline/config inventory under", OUT)

if __name__ == "__main__":
    main()
