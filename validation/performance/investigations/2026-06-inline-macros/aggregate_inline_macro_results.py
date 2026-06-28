#!/usr/bin/env python3
import csv
import re
import statistics
import subprocess
from pathlib import Path


ROOT = Path(__file__).resolve().parents[4]
INV = ROOT / "validation" / "performance" / "investigations" / "2026-06-inline-macros"
PARSED = ROOT / "validation" / "performance" / "parsed"
LOGS = ROOT / "validation" / "performance" / "logs"
BUILDS = ROOT / "validation" / "performance" / "builds"
HARDWARE = INV / "hardware"
CODEGEN = INV / "codegen"

BOARDS = ("teensy41", "core2", "cores3", "pico2")
VARIANTS = ("default", "uber_empty", "uber_noinline", "select_empty", "raster_empty", "shading_empty", "all_empty")

NM = {
    "teensy41": Path(r"C:\Users\Vindar\AppData\Local\Arduino15\packages\teensy\tools\teensy-compile\11.3.1\arm\bin\arm-none-eabi-nm.exe"),
    "pico2": Path(r"C:\Users\Vindar\AppData\Local\Arduino15\packages\rp2040\tools\pqt-gcc\4.1.0-1aec55e\bin\arm-none-eabi-nm.exe"),
    "core2": Path(r"C:\Users\Vindar\AppData\Local\Arduino15\packages\esp32\tools\esp-x32\2512\bin\xtensa-esp32-elf-nm.exe"),
    "cores3": Path(r"C:\Users\Vindar\AppData\Local\Arduino15\packages\esp32\tools\esp-x32\2512\bin\xtensa-esp32s3-elf-nm.exe"),
}


def read_csv(path):
    if not path.exists() or path.stat().st_size == 0:
        return []
    with path.open(newline="", encoding="utf-8-sig") as f:
        return list(csv.DictReader(f))


def write_csv(path, rows):
    path.parent.mkdir(parents=True, exist_ok=True)
    fields = []
    for row in rows:
        for key in row:
            if key not in fields:
                fields.append(key)
    with path.open("w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=fields)
        writer.writeheader()
        writer.writerows(rows)


def fnum(value):
    try:
        return float(str(value).replace(",", "."))
    except Exception:
        return None


def variant_from_middle(text, suffix):
    prefix = "inline_"
    if not text.startswith(prefix) or not text.endswith(suffix):
        return None
    variant = text[len(prefix):-len(suffix)]
    return variant if variant in VARIANTS else None


def collect_benchmark_globals():
    rows = []
    for board in BOARDS:
        for variant in VARIANTS:
            path = PARSED / f"{board}_inline_{variant}_benchmark_{board}_benchmark_global.csv"
            json_path = PARSED / f"{board}_inline_{variant}_benchmark_{board}.json"
            scores = []
            for row in read_csv(path):
                v = fnum(row.get("score"))
                if v is not None:
                    scores.append(v)
            status = ""
            if json_path.exists():
                import json
                status = json.loads(json_path.read_text(encoding="utf-8")).get("run_status", "")
            rows.append({
                "board": board,
                "variant": variant,
                "status": status,
                "score_count": len(scores),
                "score_1": scores[0] if len(scores) > 0 else "",
                "score_2": scores[1] if len(scores) > 1 else "",
                "score_3": scores[2] if len(scores) > 2 else "",
                "score_mean": statistics.mean(scores) if scores else "",
            })
    base = {(r["board"]): r for r in rows if r["variant"] == "default" and r["score_mean"] != ""}
    for row in rows:
        b = base.get(row["board"])
        if b and row["score_mean"] != "":
            row["delta_pct_vs_default_mean"] = (float(row["score_mean"]) / float(b["score_mean"]) - 1.0) * 100.0
        else:
            row["delta_pct_vs_default_mean"] = ""
    return rows


def collect_benchmark_subtests():
    rows = []
    by_default = {}
    for board in BOARDS:
        for variant in VARIANTS:
            path = PARSED / f"{board}_inline_{variant}_benchmark_{board}_benchmark.csv"
            for row in read_csv(path):
                fps = fnum(row.get("fps"))
                key = (board, row.get("renderer_size", ""), row.get("subtest", ""))
                out = {
                    "board": board,
                    "variant": variant,
                    "renderer_size": row.get("renderer_size", ""),
                    "subtest": row.get("subtest", ""),
                    "fps": fps if fps is not None else "",
                    "avg_us": row.get("avg_us", ""),
                }
                rows.append(out)
                if variant == "default" and fps is not None:
                    by_default[key] = fps
    for row in rows:
        key = (row["board"], row["renderer_size"], row["subtest"])
        base = by_default.get(key)
        if base and row["fps"] != "":
            row["delta_pct_vs_default"] = (float(row["fps"]) / base - 1.0) * 100.0
        else:
            row["delta_pct_vs_default"] = ""
    return rows


def summarize_subtests(subtests):
    values = {}
    for row in subtests:
        delta = row.get("delta_pct_vs_default")
        if delta == "":
            continue
        values.setdefault((row["board"], row["variant"]), []).append(float(delta))
    rows = []
    for (board, variant), vals in sorted(values.items()):
        rows.append({
            "board": board,
            "variant": variant,
            "subtests": len(vals),
            "mean_delta_pct": statistics.mean(vals),
            "median_delta_pct": statistics.median(vals),
            "worst_delta_pct": min(vals),
            "best_delta_pct": max(vals),
            "regress_lt_minus_1pct": sum(1 for v in vals if v <= -1.0),
            "improve_gt_1pct": sum(1 for v in vals if v >= 1.0),
        })
    return rows


def collect_examples():
    rows = []
    for board in BOARDS:
        for variant in VARIANTS:
            pattern = f"{board}_inline_{variant}_example_*_{board}_example.csv"
            for path in PARSED.glob(pattern):
                middle = path.name[len(f"{board}_inline_{variant}_example_"):-len(f"_{board}_example.csv")]
                vals = []
                scenes = []
                for row in read_csv(path):
                    fps = fnum(row.get("fps"))
                    if fps is not None:
                        vals.append(fps)
                        scenes.append(row.get("scene", ""))
                if vals:
                    rows.append({
                        "board": board,
                        "variant": variant,
                        "example": middle,
                        "rows": len(vals),
                        "avg_fps": statistics.mean(vals),
                        "median_fps": statistics.median(vals),
                        "min_fps": min(vals),
                        "max_fps": max(vals),
                        "scenes": ";".join(sorted(set(scenes))),
                    })
    base = {(r["board"], r["example"]): r for r in rows if r["variant"] == "default"}
    for row in rows:
        b = base.get((row["board"], row["example"]))
        if b:
            row["delta_pct_vs_default_median"] = (row["median_fps"] / b["median_fps"] - 1.0) * 100.0
            row["delta_pct_vs_default_avg"] = (row["avg_fps"] / b["avg_fps"] - 1.0) * 100.0
        else:
            row["delta_pct_vs_default_median"] = ""
            row["delta_pct_vs_default_avg"] = ""
    return sorted(rows, key=lambda r: (r["board"], r["example"], r["variant"]))


def summarize_examples(example_rows):
    grouped = {}
    for row in example_rows:
        delta = row["delta_pct_vs_default_avg"]
        if delta == "":
            continue
        grouped.setdefault((row["board"], row["variant"]), []).append(float(delta))
    out = []
    for (board, variant), vals in sorted(grouped.items()):
        out.append({
            "board": board,
            "variant": variant,
            "examples": len(vals),
            "mean_delta_pct": statistics.mean(vals),
            "median_delta_pct": statistics.median(vals),
            "worst_delta_pct": min(vals),
            "best_delta_pct": max(vals),
        })
    return out


def collect_sizes():
    rows = []
    for log in LOGS.glob("*inline_*_*.log"):
        name = log.name[:-4]
        m = re.match(r"(.+?)_inline_(.+?)_benchmark_(.+)$", name)
        if m:
            board, variant, suffix_board = m.groups()
            kind = "benchmark"
            subject = "benchmark"
        else:
            m = re.match(r"(.+?)_inline_(.+?)_example_(.+)_(.+)$", name)
            if not m:
                continue
            board, variant, subject, suffix_board = m.groups()
            kind = "example"
        if board != suffix_board or board not in BOARDS or variant not in VARIANTS:
            continue
        text = log.read_text(errors="replace")
        row = {"board": board, "variant": variant, "kind": kind, "subject": subject}
        mt = re.search(r"FLASH: code:(\d+), data:(\d+), headers:(\d+).*?RAM1: variables:(\d+), code:(\d+), padding:(\d+)\s+free for local variables:([-\d]+).*?RAM2: variables:(\d+)\s+free for malloc/new:(\d+)", text, re.S)
        if mt:
            row.update({
                "flash_code": int(mt.group(1)),
                "flash_data": int(mt.group(2)),
                "flash_headers": int(mt.group(3)),
                "ram1_variables": int(mt.group(4)),
                "ram1_code": int(mt.group(5)),
                "ram1_padding": int(mt.group(6)),
                "ram1_free": int(mt.group(7)),
                "ram2_variables": int(mt.group(8)),
                "ram2_free": int(mt.group(9)),
            })
        else:
            esp = re.search(r"Le croquis utilise\s+(\d+)\s+octets.*?Les variables globales utilisent\s+(\d+)\s+octets", text, re.S)
            pico = re.search(r"Sketch uses\s+(\d+)\s+bytes.*?Global variables use\s+(\d+)\s+bytes", text, re.S)
            if esp:
                row.update({"sketch_bytes": int(esp.group(1)), "global_bytes": int(esp.group(2))})
            elif pico:
                row.update({"sketch_bytes": int(pico.group(1)), "global_bytes": int(pico.group(2))})
        rows.append(row)
    return sorted(rows, key=lambda r: (r["board"], r["kind"], r["subject"], r["variant"]))


def collect_codegen():
    rows = []
    for board in BOARDS:
        nm = NM[board]
        if not nm.exists():
            continue
        for variant in ("default", "uber_empty", "uber_noinline", "all_empty"):
            build = BUILDS / f"{board}_inline_{variant}_example_pointlight_textured_meshes_{board}"
            elf_files = list(build.glob("*.elf"))
            if not elf_files:
                continue
            elf = elf_files[0]
            try:
                output = subprocess.check_output([str(nm), "-C", "-S", "--size-sort", str(elf)], text=True, errors="replace")
            except Exception:
                continue
            for line in output.splitlines():
                parts = line.split(maxsplit=3)
                if len(parts) < 4:
                    continue
                addr, size, typ, sym = parts
                if not any(token in sym for token in ("uber_shader", "shader_select", "rasterizeTriangle")):
                    continue
                try:
                    size_i = int(size, 16)
                except ValueError:
                    continue
                kind = "other"
                if "uber_shader_inline" in sym:
                    kind = "uber_shader_inline"
                elif "uber_shader<" in sym:
                    kind = "uber_shader"
                elif "shader_select" in sym:
                    kind = "shader_select"
                elif "rasterizeTriangle" in sym:
                    kind = "rasterizeTriangle"
                rows.append({
                    "board": board,
                    "variant": variant,
                    "example": "pointlight_textured_meshes",
                    "kind": kind,
                    "type": typ,
                    "size": size_i,
                    "symbol": sym,
                })
    summary = []
    grouped = {}
    for row in rows:
        grouped.setdefault((row["board"], row["variant"], row["kind"]), []).append(row["size"])
    for (board, variant, kind), vals in sorted(grouped.items()):
        summary.append({
            "board": board,
            "variant": variant,
            "kind": kind,
            "count": len(vals),
            "total_size": sum(vals),
            "max_size": max(vals),
        })
    return rows, summary


def main():
    HARDWARE.mkdir(parents=True, exist_ok=True)
    CODEGEN.mkdir(parents=True, exist_ok=True)

    benchmark_globals = collect_benchmark_globals()
    benchmark_subtests = collect_benchmark_subtests()
    benchmark_subtest_summary = summarize_subtests(benchmark_subtests)
    examples = collect_examples()
    example_summary = summarize_examples(examples)
    sizes = collect_sizes()
    codegen_rows, codegen_summary = collect_codegen()

    write_csv(HARDWARE / "benchmark_global_summary.csv", benchmark_globals)
    write_csv(HARDWARE / "benchmark_subtests.csv", benchmark_subtests)
    write_csv(HARDWARE / "benchmark_subtest_summary.csv", benchmark_subtest_summary)
    write_csv(HARDWARE / "example_fps_summary.csv", examples)
    write_csv(HARDWARE / "example_variant_summary.csv", example_summary)
    write_csv(HARDWARE / "compile_size_summary.csv", sizes)
    write_csv(CODEGEN / "hot_symbol_rows.csv", codegen_rows)
    write_csv(CODEGEN / "hot_symbol_summary.csv", codegen_summary)

    print(HARDWARE / "benchmark_global_summary.csv")
    print(HARDWARE / "example_variant_summary.csv")
    print(CODEGEN / "hot_symbol_summary.csv")


if __name__ == "__main__":
    main()
