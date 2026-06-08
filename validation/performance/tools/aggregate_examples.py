#!/usr/bin/env python3
import csv
import json
import statistics
import subprocess
from collections import defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
HV_PARSED = ROOT / "tmp" / "hardware_validation" / "parsed"
OUT = ROOT / "tmp" / "inline_granularity"

def read_csv(path):
    if not path.exists() or path.stat().st_size == 0:
        return []
    with path.open(newline="", encoding="utf-8-sig") as f:
        return list(csv.DictReader(f))

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

def as_float(value):
    try:
        return float(str(value).replace(",", "."))
    except (TypeError, ValueError):
        return None

def metadata(run_id):
    path = HV_PARSED / f"{run_id}.json"
    if not path.exists():
        return {}
    return json.loads(path.read_text(encoding="utf-8"))

def current_commit():
    return subprocess.check_output(["git", "rev-parse", "--short", "HEAD"], cwd=str(ROOT), text=True).strip()

def parse_label(label):
    if "_baseline_example_" in label and not label.startswith("baseline_example_"):
        label = "baseline_example_" + label.split("_baseline_example_", 1)[1]
    # baseline_example_<group>_<example...>_<suffix>
    parts = label.split("_")
    if len(parts) < 5 or parts[0] != "baseline" or parts[1] != "example":
        return "", label, ""
    group = parts[2]
    suffix = parts[-1]
    example = "_".join(parts[3:-1])
    return group, example, suffix

def main():
    commit = current_commit()
    rows = []
    summary_values = defaultdict(list)

    for path in sorted(HV_PARSED.glob("*_baseline_example_*_example.csv")):
        run_id = path.name[:-len("_example.csv")]
        meta = metadata(run_id)
        label = meta.get("label") or run_id
        group, example, suffix = parse_label(label)
        board = meta.get("board", "")
        status = meta.get("run_status", "")
        for row in read_csv(path):
            fps = as_float(row.get("fps"))
            out = dict(row)
            out.update({
                "run_id": run_id,
                "label": label,
                "group": group,
                "board": board,
                "example": example,
                "suffix": suffix,
                "run_status": status,
                "commit": commit,
            })
            rows.append(out)
            if status == "SUCCESS" and fps is not None:
                summary_values[(board, example, suffix, row.get("scene", ""))].append(fps)

    summary = []
    for (board, example, suffix, scene), values in sorted(summary_values.items()):
        summary.append({
            "board": board,
            "example": example,
            "suffix": suffix,
            "scene": scene,
            "rows": len(values),
            "fps_avg": f"{statistics.mean(values):.6g}",
            "fps_min": f"{min(values):.6g}",
            "fps_max": f"{max(values):.6g}",
        })

    baseline = {}
    for row in summary:
        if row["suffix"] == "run1":
            baseline[(row["board"], row["example"], row["scene"])] = as_float(row["fps_avg"])

    deltas = []
    for row in summary:
        if row["suffix"] != "final":
            continue
        base = baseline.get((row["board"], row["example"], row["scene"]))
        final = as_float(row["fps_avg"])
        delta = ""
        if base and final is not None:
            delta = f"{((final / base) - 1.0) * 100.0:.6g}"
        out = dict(row)
        out["baseline_fps_avg"] = f"{base:.6g}" if base else ""
        out["delta_pct_vs_baseline"] = delta
        deltas.append(out)

    write_csv(OUT / "example_telemetry.csv", rows)
    write_csv(OUT / "example_telemetry_summary.csv", summary)
    write_csv(OUT / "example_telemetry_delta.csv", deltas)
    print("wrote example telemetry CSVs")

if __name__ == "__main__":
    main()
