import csv
import json
import re
from pathlib import Path
from statistics import mean, median


ROOT = Path("validation/performance")
INV = ROOT / "investigations" / "2026-06-shader-incremental-flag-matrix"
PARSED = ROOT / "parsed"
LOGS = ROOT / "logs"
RESULTS = INV / "results"


def split_label(label):
    if "__" in label:
        variant, case = label.split("__", 1)
    else:
        variant, case = label, ""
    return variant, case


def write_csv(path, rows, fields):
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=fields)
        w.writeheader()
        w.writerows(rows)


def read_csv(path):
    if not path.exists() or path.stat().st_size == 0:
        return []
    with path.open(newline="") as f:
        return list(csv.DictReader(f))


def aggregate_benchmarks():
    global_rows = []
    subtest_rows = []

    for path in sorted(PARSED.glob("*_benchmark_global.csv")):
        for row in read_csv(path):
            variant, case = split_label(row.get("label", ""))
            if case != "benchmark":
                continue
            row["variant"] = variant
            row["case"] = case
            row["source_csv"] = str(path)
            global_rows.append(row)

    for path in sorted(PARSED.glob("*_benchmark.csv")):
        if path.name.endswith("_benchmark_global.csv"):
            continue
        for row in read_csv(path):
            variant, case = split_label(row.get("label", ""))
            if case != "benchmark":
                continue
            row["variant"] = variant
            row["case"] = case
            row["source_csv"] = str(path)
            subtest_rows.append(row)

    write_csv(
        RESULTS / "benchmark_global_scores.csv",
        global_rows,
        [
            "board", "label", "variant", "case", "score_index", "score",
            "line_number", "source_csv",
        ],
    )
    write_csv(
        RESULTS / "benchmark_subtests.csv",
        subtest_rows,
        [
            "board", "label", "variant", "case", "renderer_size",
            "subtest", "frames", "min_us", "max_us", "avg_us", "stddev_us",
            "fps", "line_number", "source_csv",
        ],
    )


def robust_average(vals):
    vals = [float(v) for v in vals if v not in ("", None)]
    if not vals:
        return "", "", 0, 0
    med = median(vals)
    robust = [v for v in vals if v < med * 1.8]
    return f"{mean(robust):.3f}", f"{med:.3f}", len(vals), len(robust)


def aggregate_examples():
    rows = []
    summary = []
    grouped = {}

    for path in sorted(PARSED.glob("*_example.csv")):
        for row in read_csv(path):
            variant, case = split_label(row.get("label", ""))
            row["variant"] = variant
            row["example"] = case
            row["source_csv"] = str(path)
            rows.append(row)
            key = (row.get("board", ""), variant, case, row.get("scene", ""))
            grouped.setdefault(key, []).append(row)

    for (board, variant, example, scene), group in sorted(grouped.items()):
        frame_avg, frame_median, samples, robust_samples = robust_average(
            [r.get("frame_avg_us", "") for r in group]
        )
        fps_avg, fps_median, fps_samples, fps_robust_samples = robust_average([r.get("fps", "") for r in group])
        summary.append(
            {
                "board": board,
                "variant": variant,
                "example": example,
                "scene": scene,
                "rows": len(group),
                "frame_samples": samples,
                "frame_robust_samples": robust_samples,
                "fps_samples": fps_samples,
                "fps_robust_samples": fps_robust_samples,
                "frame_avg_us": frame_avg,
                "frame_median_us": frame_median,
                "fps_avg": fps_avg,
                "fps_median": fps_median,
            }
        )

    write_csv(
        RESULTS / "example_telemetry.csv",
        rows,
        [
            "board", "label", "variant", "example", "scene", "fps",
            "frame_avg_us", "frames", "line_number", "raw", "source_csv",
        ],
    )
    write_csv(
        RESULTS / "example_summary.csv",
        summary,
        [
            "board", "variant", "example", "scene", "rows", "frame_samples",
            "frame_robust_samples", "fps_samples", "fps_robust_samples",
            "frame_avg_us", "frame_median_us", "fps_avg", "fps_median",
        ],
    )


SIZE_PATTERNS = [
    re.compile(r"Le croquis utilise\s+([0-9]+)\s+octets"),
    re.compile(r"Sketch uses\s+([0-9]+)\s+bytes"),
]
RAM_PATTERNS = [
    re.compile(r"Les variables globales utilisent\s+([0-9]+)\s+octets"),
    re.compile(r"Global variables use\s+([0-9]+)\s+bytes"),
]


def aggregate_manifest_and_sizes():
    manifest = []
    sizes = []

    for path in sorted(PARSED.glob("*.json")):
        data = json.loads(path.read_text(errors="replace"))
        label = data.get("label", "") or Path(data.get("metadata_file", path)).stem.split("_", 1)[1]
        variant, case = split_label(label)
        manifest.append(
            {
                "board": data.get("board", ""),
                "variant": variant,
                "case": case,
                "label": label,
                "status": data.get("run_status", ""),
                "parse_mode": data.get("parse_mode", ""),
                "telemetry_rows": data.get("parsed_telemetry_rows", ""),
                "global_scores": data.get("parsed_global_scores", ""),
                "subtests": data.get("parsed_subtests", ""),
                "metadata": str(path),
            }
        )

    for path in sorted(LOGS.glob("*.log")):
        name = path.stem
        if "_" not in name:
            continue
        board, label = name.split("_", 1)
        variant, case = split_label(label)
        sketch_bytes = ""
        globals_bytes = ""
        for line in path.read_text(errors="replace").splitlines():
            for pat in SIZE_PATTERNS:
                m = pat.search(line)
                if m:
                    sketch_bytes = m.group(1)
            for pat in RAM_PATTERNS:
                m = pat.search(line)
                if m:
                    globals_bytes = m.group(1)
        if sketch_bytes or globals_bytes:
            sizes.append(
                {
                    "board": board,
                    "variant": variant,
                    "case": case,
                    "label": label,
                    "sketch_bytes": sketch_bytes,
                    "globals_bytes": globals_bytes,
                    "log": str(path),
                }
            )

    write_csv(
        RESULTS / "run_manifest.csv",
        manifest,
        [
            "board", "variant", "case", "label", "status", "parse_mode",
            "telemetry_rows", "global_scores", "subtests", "metadata",
        ],
    )
    write_csv(
        RESULTS / "binary_size.csv",
        sizes,
        [
            "board", "variant", "case", "label", "sketch_bytes",
            "globals_bytes", "log",
        ],
    )


def main():
    aggregate_benchmarks()
    aggregate_examples()
    aggregate_manifest_and_sizes()
    print(f"wrote results under {RESULTS}")


if __name__ == "__main__":
    main()
