#!/usr/bin/env python3
import csv
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
BUILDS = ROOT / "tmp" / "hardware_validation" / "builds"
OUT = ROOT / "tmp" / "inline_granularity" / "codegen"
NM_RP = Path(r"C:\Users\Vindar\AppData\Local\Arduino15\packages\rp2040\tools\pqt-gcc\4.1.0-1aec55e\bin\arm-none-eabi-nm.exe")
OBJDUMP_RP = Path(r"C:\Users\Vindar\AppData\Local\Arduino15\packages\rp2040\tools\pqt-gcc\4.1.0-1aec55e\bin\arm-none-eabi-objdump.exe")

RUNS = {
    "pico2_baseline": "pico2_baseline_bench_pico2_run2",
    "pico2_vec_policy_default": "pico2_vec_policy_default_bench_pico2",
    "pico2_vec2_empty": "pico2_vec2_empty_pico2_bench_pico2",
    "pico2_vec3_empty": "pico2_vec3_empty_pico2_bench_pico2",
    "pico2_vec4_empty": "pico2_vec4_empty_pico2_bench_pico2",
    "pico2_color_empty": "pico2_color_empty_pico2_bench_pico2",
    "pico2_rgbf_empty": "pico2_rgbf_empty_pico2_bench_pico2",
    "pico2_math_empty": "pico2_math_empty_pico2_bench_pico2",
    "pico2_image_empty": "pico2_image_empty_pico2_bench_pico2",
}

HOT = [
    "_drawMesh", "_drawTriangle", "_drawTriangleClipped", "_drawQuad",
    "raster", "shader", "uber", "zdivide", "normalize_fast",
    "dotProduct", "crossProduct", "interpolateColors", "RGBf",
    "Vec2", "Vec3", "Vec4", "drawPixel", "readPixel",
    "Buddha", "R2D2", "Bunny",
]

def write_csv(path, rows):
    path.parent.mkdir(parents=True, exist_ok=True)
    fieldnames = []
    for row in rows:
        for key in row:
            if key not in fieldnames:
                fieldnames.append(key)
    with path.open("w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)

def find_elf(run_id):
    files = list((BUILDS / run_id).glob("*.elf"))
    return files[0] if files else None

def nm_symbols(elf):
    output = subprocess.check_output([str(NM_RP), "-C", "-S", "--size-sort", str(elf)], text=True, errors="replace")
    rows = []
    for line in output.splitlines():
        parts = line.split(maxsplit=3)
        if len(parts) < 4:
            continue
        addr_s, size_s, typ, name = parts
        try:
            addr = int(addr_s, 16)
            size = int(size_s, 16)
        except ValueError:
            continue
        rows.append({
            "address": addr,
            "address_hex": f"0x{addr:08x}",
            "size": size,
            "type": typ,
            "align16": addr % 16,
            "align32": addr % 32,
            "name": name,
        })
    return rows

def is_hot(name):
    return any(token in name for token in HOT)

def disassemble(elf, out_path):
    if not OBJDUMP_RP.exists():
        return
    text = subprocess.check_output([str(OBJDUMP_RP), "-Cd", str(elf)], text=True, errors="replace")
    out_path.write_text(text, encoding="utf-8")

def main():
    OUT.mkdir(parents=True, exist_ok=True)
    all_rows = []
    hot_rows = []
    for label, run_id in RUNS.items():
        elf = find_elf(run_id)
        if not elf:
            continue
        symbols = nm_symbols(elf)
        for row in symbols:
            out = dict(row)
            out.update({
                "label": label,
                "run_id": run_id,
                "elf": str(elf.relative_to(ROOT)),
            })
            all_rows.append(out)
            if is_hot(row["name"]):
                hot_rows.append(out)
        disassemble(elf, OUT / f"{label}.objdump.txt")

    write_csv(OUT / "pico2_symbols.csv", all_rows)
    write_csv(OUT / "pico2_hot_symbols.csv", hot_rows)

    by_label_name = {(r["label"], r["name"]): r for r in all_rows}
    baseline = {r["name"]: r for r in all_rows if r["label"] == "pico2_baseline"}
    diff_rows = []
    for row in hot_rows:
        if row["label"] == "pico2_baseline":
            continue
        base = baseline.get(row["name"])
        diff = dict(row)
        if base:
            diff["baseline_address_hex"] = base["address_hex"]
            diff["baseline_size"] = base["size"]
            diff["size_delta"] = row["size"] - base["size"]
            diff["address_delta"] = row["address"] - base["address"]
            diff["baseline_align16"] = base["align16"]
            diff["baseline_align32"] = base["align32"]
        else:
            diff["baseline_address_hex"] = ""
            diff["baseline_size"] = ""
            diff["size_delta"] = ""
            diff["address_delta"] = ""
            diff["baseline_align16"] = ""
            diff["baseline_align32"] = ""
        diff_rows.append(diff)
    write_csv(OUT / "pico2_hot_symbol_deltas.csv", diff_rows)

    summary = OUT.parent / "reports" / "codegen_summary.md"
    summary.parent.mkdir(parents=True, exist_ok=True)
    with summary.open("w", encoding="utf-8") as f:
        f.write("# Codegen Summary\n\n")
        f.write("Pico2 symbol extraction used `arm-none-eabi-nm -C -S --size-sort`.\n\n")
        for label, run_id in RUNS.items():
            elf = find_elf(run_id)
            if not elf:
                continue
            f.write(f"- {label}: `{elf.relative_to(ROOT)}` size {elf.stat().st_size} bytes\n")
        f.write("\nSee `tmp/inline_granularity/codegen/pico2_hot_symbols.csv` and `pico2_hot_symbol_deltas.csv`.\n")
    print("wrote codegen CSVs")

if __name__ == "__main__":
    main()
