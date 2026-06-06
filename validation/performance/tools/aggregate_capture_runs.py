#!/usr/bin/env python3
import csv
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
HV_PARSED = ROOT / "tmp" / "hardware_validation" / "parsed"
HV_BUILDS = ROOT / "tmp" / "hardware_validation" / "builds"
OUT = ROOT / "tmp" / "inline_granularity"

BASELINE_BENCHMARKS = [
    "core2_baseline_bench_core2_run1",
    "cores3_parse_sanity_bench_cores3",
    "cores3_baseline_bench_cores3_run2",
    "pico2_parse_sanity_bench_pico2",
    "pico2_baseline_bench_pico2_run2",
    "teensy41_baseline_bench_teensy41_run1",
    "teensy41_baseline_bench_teensy41_run2",
]

BASELINE_EXAMPLES = [
    "teensy41_parse_sanity_example_teensy_scream",
    "pico2_parse_sanity_example_pico2_bunny_fig",
]

def read_csv(path):
    if not path.exists() or path.stat().st_size == 0:
        return []
    with path.open(newline="", encoding="utf-8") as f:
        return [normalize_row(row) for row in csv.DictReader(f)]

def normalize_row(row):
    numeric = {
        "score", "min_us", "max_us", "avg_us", "stddev_us", "fps",
        "frame_avg_us"
    }
    out = dict(row)
    for key in numeric:
        if key in out and isinstance(out[key], str):
            out[key] = out[key].replace(",", ".")
    return out

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

def load_metadata(run_id):
    path = HV_PARSED / (run_id + ".json")
    if not path.exists():
        return {}
    return json.loads(path.read_text(encoding="utf-8"))

def benchmark_rows():
    global_rows = []
    subtest_rows = []
    for run_id in BASELINE_BENCHMARKS:
        meta = load_metadata(run_id)
        for row in read_csv(HV_PARSED / (run_id + "_benchmark_global.csv")):
            row.update({
                "run_id": run_id,
                "phase": "baseline",
                "run_status": meta.get("run_status", ""),
                "commit": current_commit(),
            })
            global_rows.append(row)
        for row in read_csv(HV_PARSED / (run_id + "_benchmark.csv")):
            row.update({
                "run_id": run_id,
                "phase": "baseline",
                "run_status": meta.get("run_status", ""),
                "commit": current_commit(),
            })
            subtest_rows.append(row)
    return global_rows, subtest_rows

_commit = None
def current_commit():
    global _commit
    if _commit is None:
        import subprocess
        _commit = subprocess.check_output(["git", "rev-parse", "--short", "HEAD"], cwd=str(ROOT), text=True).strip()
    return _commit

def binary_size_rows(run_ids):
    rows = []
    for run_id in run_ids:
        meta = load_metadata(run_id)
        build = HV_BUILDS / run_id
        row = {
            "run_id": run_id,
            "board": meta.get("board", ""),
            "label": meta.get("label", run_id),
            "phase": "baseline",
            "run_status": meta.get("run_status", ""),
            "commit": current_commit(),
        }
        if build.exists():
            for ext in (".elf", ".bin", ".uf2", ".hex", ".map"):
                files = list(build.glob("*" + ext))
                if files:
                    row[ext[1:] + "_bytes"] = sum(p.stat().st_size for p in files)
                    row[ext[1:] + "_files"] = ";".join(p.name for p in files)
        rows.append(row)
    return rows

def example_rows():
    rows = []
    seen = set(BASELINE_EXAMPLES)
    run_ids = list(BASELINE_EXAMPLES)
    for path in sorted(HV_PARSED.glob("*_baseline_example_*_example.csv")):
        run_id = path.name[:-len("_example.csv")]
        if run_id not in seen:
            seen.add(run_id)
            run_ids.append(run_id)
    for run_id in run_ids:
        meta = load_metadata(run_id)
        for row in read_csv(HV_PARSED / (run_id + "_example.csv")):
            row.update({
                "run_id": run_id,
                "phase": "baseline",
                "run_status": meta.get("run_status", ""),
                "commit": current_commit(),
            })
            rows.append(row)
    return rows

def example_summary_rows(rows):
    groups = {}
    for row in rows:
        key = (
            row.get("board", ""),
            row.get("run_id", ""),
            row.get("scene", ""),
        )
        try:
            fps = float(str(row.get("fps", "")).replace(",", "."))
        except ValueError:
            continue
        groups.setdefault(key, []).append(fps)

    out = []
    for (board, run_id, scene), values in sorted(groups.items()):
        if not values:
            continue
        out.append({
            "board": board,
            "run_id": run_id,
            "scene": scene,
            "rows": len(values),
            "fps_avg": f"{sum(values) / len(values):.6g}",
            "fps_min": f"{min(values):.6g}",
            "fps_max": f"{max(values):.6g}",
            "phase": "baseline",
            "commit": current_commit(),
        })
    return out

def main():
    global_rows, subtest_rows = benchmark_rows()
    write_csv(OUT / "baseline_global_scores.csv", global_rows)
    write_csv(OUT / "baseline_subtests.csv", subtest_rows)
    write_csv(OUT / "baseline_binary_size.csv", binary_size_rows(BASELINE_BENCHMARKS))
    examples = example_rows()
    write_csv(OUT / "baseline_example_telemetry.csv", examples)
    write_csv(OUT / "baseline_example_telemetry_summary.csv", example_summary_rows(examples))
    print("wrote baseline aggregate CSVs")

if __name__ == "__main__":
    main()
