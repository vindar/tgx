#!/usr/bin/env python3
import csv
import json
import statistics
import subprocess
from collections import defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
HV_PARSED = ROOT / "tmp" / "hardware_validation" / "parsed"
HV_BUILDS = ROOT / "tmp" / "hardware_validation" / "builds"
OUT = ROOT / "tmp" / "inline_granularity"

BASELINE_GLOBAL = OUT / "baseline_global_scores.csv"
BASELINE_SUBTESTS = OUT / "baseline_subtests.csv"

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

def current_commit():
    return subprocess.check_output(["git", "rev-parse", "--short", "HEAD"], cwd=str(ROOT), text=True).strip()

def metadata(run_id):
    path = HV_PARSED / f"{run_id}.json"
    if not path.exists():
        return {}
    return json.loads(path.read_text(encoding="utf-8"))

def baseline_global_map():
    values = defaultdict(list)
    for row in read_csv(BASELINE_GLOBAL):
        if row.get("run_status") != "SUCCESS":
            continue
        score = as_float(row.get("score"))
        if score is not None:
            values[(row.get("board"), row.get("score_index"))].append(score)
    return {k: statistics.median(v) for k, v in values.items() if v}

def baseline_subtest_map():
    values = defaultdict(list)
    for row in read_csv(BASELINE_SUBTESTS):
        if row.get("run_status") != "SUCCESS":
            continue
        fps = as_float(row.get("fps"))
        if fps is not None:
            key = (row.get("board"), row.get("renderer_size"), row.get("subtest"))
            values[key].append(fps)
    return {k: statistics.median(v) for k, v in values.items() if v}

def candidate_run_ids():
    ids = []
    for path in sorted(HV_PARSED.glob("*_benchmark_global.csv")):
        run_id = path.name[:-len("_benchmark_global.csv")]
        if "baseline_bench" in run_id or "parse_sanity_bench" in run_id:
            continue
        meta = metadata(run_id)
        if meta.get("parse_mode") == "benchmark":
            ids.append(run_id)
    return ids

def binary_sizes(run_id):
    build = HV_BUILDS / run_id
    out = {}
    if not build.exists():
        return out
    for ext in ("elf", "bin", "uf2", "hex", "map"):
        files = list(build.glob(f"*.{ext}"))
        if files:
            out[f"{ext}_bytes"] = sum(p.stat().st_size for p in files)
            out[f"{ext}_files"] = ";".join(p.name for p in files)
    return out

def aggregate():
    bg = baseline_global_map()
    bs = baseline_subtest_map()
    commit = current_commit()
    global_rows = []
    subtest_rows = []
    summary_rows = []
    size_rows = []

    for run_id in candidate_run_ids():
        meta = metadata(run_id)
        board = meta.get("board", "")
        label = meta.get("label", run_id)
        status = meta.get("run_status", "")
        candidate = label
        suffix = f"_bench_{board}"
        if candidate.endswith(suffix):
            candidate = candidate[:-len(suffix)]

        for row in read_csv(HV_PARSED / f"{run_id}_benchmark_global.csv"):
            score = as_float(row.get("score"))
            base = bg.get((board, row.get("score_index")))
            delta = ""
            if score is not None and base:
                delta = f"{((score / base) - 1.0) * 100.0:.6g}"
            out = dict(row)
            out.update({
                "candidate": candidate,
                "run_id": run_id,
                "board": board,
                "run_status": status,
                "baseline_score": f"{base:.6g}" if base else "",
                "delta_pct_vs_baseline": delta,
                "commit": commit,
            })
            global_rows.append(out)

        deltas = []
        for row in read_csv(HV_PARSED / f"{run_id}_benchmark.csv"):
            fps = as_float(row.get("fps"))
            base = bs.get((board, row.get("renderer_size"), row.get("subtest")))
            delta_value = None
            if fps is not None and base:
                delta_value = ((fps / base) - 1.0) * 100.0
            out = dict(row)
            out.update({
                "candidate": candidate,
                "run_id": run_id,
                "board": board,
                "run_status": status,
                "baseline_fps": f"{base:.6g}" if base else "",
                "delta_pct_vs_baseline": f"{delta_value:.6g}" if delta_value is not None else "",
                "commit": commit,
            })
            subtest_rows.append(out)
            if delta_value is not None:
                deltas.append((delta_value, row.get("renderer_size"), row.get("subtest")))

        if deltas:
            values = [d[0] for d in deltas]
            worst = min(deltas, key=lambda x: x[0])
            best = max(deltas, key=lambda x: x[0])
            summary_rows.append({
                "candidate": candidate,
                "run_id": run_id,
                "board": board,
                "run_status": status,
                "subtests": len(values),
                "mean_delta_pct": f"{statistics.mean(values):.6g}",
                "median_delta_pct": f"{statistics.median(values):.6g}",
                "improved_gt_1pct": sum(1 for x in values if x > 1.0),
                "neutral_abs_le_0_5pct": sum(1 for x in values if abs(x) <= 0.5),
                "weak_regress_0_5_to_1pct": sum(1 for x in values if -1.0 < x < -0.5),
                "regress_lt_minus_1pct": sum(1 for x in values if x <= -1.0),
                "regress_lt_minus_3pct": sum(1 for x in values if x <= -3.0),
                "worst_delta_pct": f"{worst[0]:.6g}",
                "worst_renderer_size": worst[1],
                "worst_test": worst[2],
                "best_delta_pct": f"{best[0]:.6g}",
                "best_renderer_size": best[1],
                "best_test": best[2],
            })

        size_row = {
            "candidate": candidate,
            "run_id": run_id,
            "board": board,
            "run_status": status,
            "commit": commit,
        }
        size_row.update(binary_sizes(run_id))
        size_rows.append(size_row)

    write_csv(OUT / "global_scores.csv", global_rows)
    write_csv(OUT / "subtest_matrix.csv", subtest_rows)
    write_csv(OUT / "candidate_subtest_summary.csv", summary_rows)
    write_csv(OUT / "candidate_binary_size.csv", size_rows)

    report = OUT / "reports" / "candidate_subtest_summaries.md"
    report.parent.mkdir(parents=True, exist_ok=True)
    with report.open("w", encoding="utf-8") as report_file:
        report_file.write("# Candidate Subtest Summaries\n\n")
        for row in summary_rows:
            report_file.write(
                f"- {row['candidate']} / {row['board']} / {row['run_status']}: "
                f"mean {row['mean_delta_pct']}%, median {row['median_delta_pct']}%, "
                f"worst {row['worst_delta_pct']}% ({row['worst_renderer_size']} {row['worst_test']}), "
                f"best {row['best_delta_pct']}% ({row['best_renderer_size']} {row['best_test']}), "
                f"regressions <= -3%: {row['regress_lt_minus_3pct']}\n"
            )
    print("wrote candidate aggregate CSVs")

if __name__ == "__main__":
    aggregate()
