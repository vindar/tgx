#!/usr/bin/env python3
import csv
import json
import re
import statistics
import subprocess
from pathlib import Path


ROOT = Path(__file__).resolve().parents[4]
INV = ROOT / "validation" / "performance" / "investigations" / "2026-06-rasterize-triangle-inline"
PARSED = ROOT / "validation" / "performance" / "parsed"
LOGS = ROOT / "validation" / "performance" / "logs"
BUILDS = ROOT / "validation" / "performance" / "builds"
HARDWARE = INV / "hardware"
CODEGEN = INV / "codegen"

BOARDS = ("teensy41", "core2", "cores3", "pico2")
VARIANTS = ("default", "raster_empty", "raster_noinline", "raster_noinline_noclone")

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


def metadata_status(board, round_id, variant, kind, subject):
    if kind == "benchmark":
        path = PARSED / f"{board}_rasterinline_r{round_id}_{variant}_benchmark_{board}.json"
    else:
        path = PARSED / f"{board}_rasterinline_r{round_id}_{variant}_example_{subject}_{board}.json"
    if not path.exists():
        return ""
    try:
        return json.loads(path.read_text(encoding="utf-8")).get("run_status", "")
    except Exception:
        return ""


def collect_benchmark_globals():
    rows = []
    for board in BOARDS:
        for variant in VARIANTS:
            for path in sorted(PARSED.glob(f"{board}_rasterinline_r*_{variant}_benchmark_{board}_benchmark_global.csv")):
                m = re.search(r"_rasterinline_r(\d+)_", path.name)
                round_id = int(m.group(1)) if m else 0
                scores = [fnum(row.get("score")) for row in read_csv(path)]
                scores = [v for v in scores if v is not None]
                rows.append({
                    "board": board,
                    "round": round_id,
                    "variant": variant,
                    "status": metadata_status(board, round_id, variant, "benchmark", "benchmark"),
                    "score_count": len(scores),
                    "score_mean": statistics.mean(scores) if scores else "",
                    "score_1": scores[0] if len(scores) > 0 else "",
                    "score_2": scores[1] if len(scores) > 1 else "",
                    "score_3": scores[2] if len(scores) > 2 else "",
                })
    return rows


def summarize_benchmark_globals(rows):
    grouped = {}
    for row in rows:
        if row["score_mean"] == "":
            continue
        grouped.setdefault((row["board"], row["variant"]), []).append(float(row["score_mean"]))
    out = []
    base = {}
    for (board, variant), values in grouped.items():
        if variant == "default":
            base[board] = statistics.mean(values)
    for (board, variant), values in sorted(grouped.items()):
        mean = statistics.mean(values)
        row = {
            "board": board,
            "variant": variant,
            "runs": len(values),
            "mean_score": mean,
            "median_score": statistics.median(values),
            "min_score": min(values),
            "max_score": max(values),
        }
        if board in base:
            row["delta_pct_vs_default_mean"] = (mean / base[board] - 1.0) * 100.0
        else:
            row["delta_pct_vs_default_mean"] = ""
        out.append(row)
    return out


def collect_benchmark_subtests():
    raw = []
    for board in BOARDS:
        for variant in VARIANTS:
            for path in sorted(PARSED.glob(f"{board}_rasterinline_r*_{variant}_benchmark_{board}_benchmark.csv")):
                m = re.search(r"_rasterinline_r(\d+)_", path.name)
                round_id = int(m.group(1)) if m else 0
                for row in read_csv(path):
                    fps = fnum(row.get("fps"))
                    if fps is None:
                        continue
                    raw.append({
                        "board": board,
                        "round": round_id,
                        "variant": variant,
                        "renderer_size": row.get("renderer_size", ""),
                        "subtest": row.get("subtest", ""),
                        "fps": fps,
                        "avg_us": fnum(row.get("avg_us")),
                    })

    grouped = {}
    for row in raw:
        key = (row["board"], row["variant"], row["renderer_size"], row["subtest"])
        grouped.setdefault(key, []).append(row["fps"])

    averaged = []
    for (board, variant, renderer_size, subtest), vals in sorted(grouped.items()):
        averaged.append({
            "board": board,
            "variant": variant,
            "renderer_size": renderer_size,
            "subtest": subtest,
            "runs": len(vals),
            "fps_mean": statistics.mean(vals),
            "fps_median": statistics.median(vals),
            "fps_min": min(vals),
            "fps_max": max(vals),
        })

    base = {
        (row["board"], row["renderer_size"], row["subtest"]): row["fps_mean"]
        for row in averaged
        if row["variant"] == "default"
    }
    for row in averaged:
        b = base.get((row["board"], row["renderer_size"], row["subtest"]))
        if b:
            row["delta_pct_vs_default_mean"] = (row["fps_mean"] / b - 1.0) * 100.0
        else:
            row["delta_pct_vs_default_mean"] = ""
    return raw, averaged


def summarize_subtests(rows):
    grouped = {}
    for row in rows:
        delta = row.get("delta_pct_vs_default_mean")
        if delta == "":
            continue
        grouped.setdefault((row["board"], row["variant"]), []).append(float(delta))
    out = []
    for (board, variant), vals in sorted(grouped.items()):
        out.append({
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
    return out


def collect_examples():
    raw = []
    for board in BOARDS:
        for variant in VARIANTS:
            for path in sorted(PARSED.glob(f"{board}_rasterinline_r*_{variant}_example_*_{board}_example.csv")):
                m = re.match(rf"{board}_rasterinline_r(\d+)_{variant}_example_(.+)_{board}_example\.csv", path.name)
                if not m:
                    continue
                round_id = int(m.group(1))
                example = m.group(2)
                vals = []
                scenes = []
                for row in read_csv(path):
                    fps = fnum(row.get("fps"))
                    if fps is not None:
                        vals.append(fps)
                        scenes.append(row.get("scene", ""))
                if not vals:
                    continue
                raw.append({
                    "board": board,
                    "round": round_id,
                    "variant": variant,
                    "example": example,
                    "status": metadata_status(board, round_id, variant, "example", example),
                    "rows": len(vals),
                    "avg_fps": statistics.mean(vals),
                    "median_fps": statistics.median(vals),
                    "min_fps": min(vals),
                    "max_fps": max(vals),
                    "scenes": ";".join(sorted(set(scenes))),
                })

    grouped = {}
    for row in raw:
        grouped.setdefault((row["board"], row["variant"], row["example"]), []).append(row["avg_fps"])
    averaged = []
    for (board, variant, example), vals in sorted(grouped.items()):
        averaged.append({
            "board": board,
            "variant": variant,
            "example": example,
            "runs": len(vals),
            "avg_fps_mean": statistics.mean(vals),
            "avg_fps_median": statistics.median(vals),
            "avg_fps_min": min(vals),
            "avg_fps_max": max(vals),
        })
    base = {
        (row["board"], row["example"]): row["avg_fps_mean"]
        for row in averaged
        if row["variant"] == "default"
    }
    for row in averaged:
        b = base.get((row["board"], row["example"]))
        if b:
            row["delta_pct_vs_default_avg"] = (row["avg_fps_mean"] / b - 1.0) * 100.0
        else:
            row["delta_pct_vs_default_avg"] = ""
    return raw, averaged


def summarize_examples(rows):
    grouped = {}
    for row in rows:
        delta = row.get("delta_pct_vs_default_avg")
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
            "regress_lt_minus_1pct": sum(1 for v in vals if v <= -1.0),
            "improve_gt_1pct": sum(1 for v in vals if v >= 1.0),
        })
    return out


def collect_sizes():
    rows = []
    for log in sorted(LOGS.glob("*rasterinline_r*.log")):
        name = log.name[:-4]
        m = re.match(r"(.+?)_rasterinline_r(\d+)_(.+?)_benchmark_(.+)$", name)
        if m:
            board, round_id, variant, suffix_board = m.groups()
            kind = "benchmark"
            subject = "benchmark"
        else:
            m = re.match(r"(.+?)_rasterinline_r(\d+)_(.+?)_example_(.+)_(.+)$", name)
            if not m:
                continue
            board, round_id, variant, subject, suffix_board = m.groups()
            kind = "example"
        if board != suffix_board or board not in BOARDS or variant not in VARIANTS:
            continue

        text = log.read_text(errors="replace")
        row = {"board": board, "round": int(round_id), "variant": variant, "kind": kind, "subject": subject}
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

    summary = []
    numeric_keys = ("flash_code", "ram1_code", "ram1_free", "sketch_bytes", "global_bytes")
    grouped = {}
    for row in rows:
        grouped.setdefault((row["board"], row["variant"], row["kind"], row["subject"]), []).append(row)
    for (board, variant, kind, subject), group in sorted(grouped.items()):
        out = {"board": board, "variant": variant, "kind": kind, "subject": subject, "runs": len(group)}
        for key in numeric_keys:
            vals = [g[key] for g in group if key in g]
            if vals:
                out[f"{key}_mean"] = statistics.mean(vals)
                out[f"{key}_min"] = min(vals)
                out[f"{key}_max"] = max(vals)
        summary.append(out)
    return rows, summary


def collect_codegen():
    rows = []
    subjects = (
        ("benchmark", "benchmark"),
        ("example", "pointlight_textured_meshes"),
        ("example", "spotlight_checkerboard"),
    )
    for board in BOARDS:
        nm = NM[board]
        if not nm.exists():
            continue
        for variant in VARIANTS:
            for kind, subject in subjects:
                if kind == "benchmark":
                    build = BUILDS / f"{board}_rasterinline_r1_{variant}_benchmark_{board}"
                else:
                    build = BUILDS / f"{board}_rasterinline_r1_{variant}_example_{subject}_{board}"
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
                    if "rasterizeTriangle" not in sym:
                        continue
                    try:
                        size_i = int(size, 16)
                    except ValueError:
                        continue
                    rows.append({
                        "board": board,
                        "variant": variant,
                        "kind": kind,
                        "subject": subject,
                        "type": typ,
                        "size": size_i,
                        "symbol": sym,
                    })
    summary = []
    grouped = {}
    for row in rows:
        grouped.setdefault((row["board"], row["variant"], row["kind"], row["subject"]), []).append(row["size"])
    for (board, variant, kind, subject), vals in sorted(grouped.items()):
        summary.append({
            "board": board,
            "variant": variant,
            "kind": kind,
            "subject": subject,
            "count": len(vals),
            "total_size": sum(vals),
            "max_size": max(vals),
        })
    return rows, summary


def main():
    HARDWARE.mkdir(parents=True, exist_ok=True)
    CODEGEN.mkdir(parents=True, exist_ok=True)

    benchmark_global_runs = collect_benchmark_globals()
    benchmark_global_summary = summarize_benchmark_globals(benchmark_global_runs)
    benchmark_subtest_raw, benchmark_subtest_avg = collect_benchmark_subtests()
    benchmark_subtest_summary = summarize_subtests(benchmark_subtest_avg)
    example_raw, example_avg = collect_examples()
    example_summary = summarize_examples(example_avg)
    size_rows, size_summary = collect_sizes()
    codegen_rows, codegen_summary = collect_codegen()

    write_csv(HARDWARE / "benchmark_global_runs.csv", benchmark_global_runs)
    write_csv(HARDWARE / "benchmark_global_summary.csv", benchmark_global_summary)
    write_csv(HARDWARE / "benchmark_subtest_runs.csv", benchmark_subtest_raw)
    write_csv(HARDWARE / "benchmark_subtest_avg.csv", benchmark_subtest_avg)
    write_csv(HARDWARE / "benchmark_subtest_summary.csv", benchmark_subtest_summary)
    write_csv(HARDWARE / "example_runs.csv", example_raw)
    write_csv(HARDWARE / "example_avg.csv", example_avg)
    write_csv(HARDWARE / "example_variant_summary.csv", example_summary)
    write_csv(HARDWARE / "compile_size_runs.csv", size_rows)
    write_csv(HARDWARE / "compile_size_summary.csv", size_summary)
    write_csv(CODEGEN / "rasterize_symbol_rows.csv", codegen_rows)
    write_csv(CODEGEN / "rasterize_symbol_summary.csv", codegen_summary)

    print(HARDWARE / "benchmark_global_summary.csv")
    print(HARDWARE / "benchmark_subtest_summary.csv")
    print(HARDWARE / "example_variant_summary.csv")
    print(HARDWARE / "compile_size_summary.csv")
    print(CODEGEN / "rasterize_symbol_summary.csv")


if __name__ == "__main__":
    main()
