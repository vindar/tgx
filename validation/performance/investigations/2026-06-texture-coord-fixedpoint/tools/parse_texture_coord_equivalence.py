#!/usr/bin/env python3
import argparse
import csv
import re
from pathlib import Path
from statistics import mean, median


ROW_RE = re.compile(r"([A-Z0-9_]+)=([^ ]+)")


def parse_file(path: Path):
    board = ""
    rows = []
    in_block = False
    for raw in path.read_text(errors="replace").splitlines():
        line = raw.strip()
        if line == "TGX_TEXCOORD_EQ_BEGIN":
            in_block = True
            continue
        if line == "TGX_TEXCOORD_EQ_END":
            in_block = False
            continue
        if line.startswith("BOARD="):
            board = line.split("=", 1)[1]
            continue
        if not in_block or not line.startswith("CASE="):
            continue
        fields = dict(ROW_RE.findall(line))
        fields["board"] = board
        fields["source_file"] = str(path)
        rows.append(fields)
    return rows


def f(row, key, default=0.0):
    try:
        return float(row.get(key, default))
    except ValueError:
        return default


def i(row, key, default=0):
    try:
        return int(float(row.get(key, default)))
    except ValueError:
        return default


def write_csv(path: Path, rows, fieldnames):
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", newline="") as fp:
        writer = csv.DictWriter(fp, fieldnames=fieldnames, extrasaction="ignore")
        writer.writeheader()
        writer.writerows(rows)


def summarize(rows):
    groups = {}
    for row in rows:
        key = (
            row.get("board", ""),
            row.get("MODE", ""),
            row.get("VARIANT", ""),
            row.get("PERSPECTIVE", ""),
            row.get("WRAP", ""),
            row.get("FRAC", ""),
        )
        groups.setdefault(key, []).append(row)

    out = []
    for key, items in sorted(groups.items()):
        deltas = [f(r, "DELTA") for r in items]
        mismatches = [i(r, "PIXEL_MISMATCHES") for r in items]
        mismatch_pct = [f(r, "MISMATCH_PCT") for r in items]
        max_err = [i(r, "MAX_ABS_CHANNEL_ERROR") for r in items]
        mean_err = [f(r, "MEAN_ABS_CHANNEL_ERROR") for r in items]
        rmse = [f(r, "RMSE") for r in items]
        exact = sum(1 for r in items if i(r, "CORRECT") == 1)
        out.append({
            "board": key[0],
            "mode": key[1],
            "variant": key[2],
            "perspective": key[3],
            "wrap": key[4],
            "frac": key[5],
            "rows": len(items),
            "exact_rows": exact,
            "exact_pct": 100.0 * exact / len(items),
            "mean_delta_pct": mean(deltas),
            "median_delta_pct": median(deltas),
            "best_delta_pct": max(deltas),
            "worst_delta_pct": min(deltas),
            "total_pixel_mismatches": sum(mismatches),
            "mean_mismatch_pct": mean(mismatch_pct),
            "max_mismatch_pct": max(mismatch_pct),
            "max_abs_channel_error": max(max_err),
            "mean_abs_channel_error": mean(mean_err),
            "mean_rmse": mean(rmse),
        })
    return out


def summarize_cases(rows):
    groups = {}
    for row in rows:
        case = row.get("CASE", "")
        tex, coord = case.split("/", 1) if "/" in case else (case, "")
        key = (
            row.get("board", ""),
            tex,
            coord,
            row.get("MODE", ""),
            row.get("VARIANT", ""),
            row.get("FRAC", ""),
        )
        groups.setdefault(key, []).append(row)
    out = []
    for key, items in sorted(groups.items()):
        deltas = [f(r, "DELTA") for r in items]
        mismatch_pct = [f(r, "MISMATCH_PCT") for r in items]
        max_err = [i(r, "MAX_ABS_CHANNEL_ERROR") for r in items]
        exact = sum(1 for r in items if i(r, "CORRECT") == 1)
        out.append({
            "board": key[0],
            "texture_case": key[1],
            "coord_case": key[2],
            "mode": key[3],
            "variant": key[4],
            "frac": key[5],
            "rows": len(items),
            "exact_rows": exact,
            "exact_pct": 100.0 * exact / len(items),
            "mean_delta_pct": mean(deltas),
            "mean_mismatch_pct": mean(mismatch_pct),
            "max_mismatch_pct": max(mismatch_pct),
            "max_abs_channel_error": max(max_err),
        })
    return out


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("telemetry", nargs="+", type=Path)
    parser.add_argument("--out-dir", type=Path, required=True)
    args = parser.parse_args()

    rows = []
    for path in args.telemetry:
        rows.extend(parse_file(path))

    fieldnames = [
        "board", "source_file", "CASE", "MODE", "VARIANT", "SAFE",
        "PERSPECTIVE", "WRAP", "TEX", "FRAC", "LEN", "CORRECT",
        "PIXEL_MISMATCHES", "MISMATCH_PCT",
        "MAX_ABS_CHANNEL_ERROR", "MEAN_ABS_CHANNEL_ERROR", "RMSE",
        "PIXELS_GT1", "FB_HASH_REF", "FB_HASH_CAND", "TIME_REF_US",
        "TIME_CAND_US", "CYCLES_REF", "CYCLES_CAND", "REPEATS", "DELTA",
    ]
    write_csv(args.out_dir / "texture_coord_equivalence.csv", rows, fieldnames)
    write_csv(args.out_dir / "texture_coord_summary.csv", summarize(rows), [
        "board", "mode", "variant", "perspective", "wrap", "frac", "rows",
        "exact_rows", "exact_pct", "mean_delta_pct", "median_delta_pct",
        "best_delta_pct", "worst_delta_pct", "total_pixel_mismatches",
        "mean_mismatch_pct", "max_mismatch_pct", "max_abs_channel_error",
        "mean_abs_channel_error", "mean_rmse",
    ])
    write_csv(args.out_dir / "texture_coord_case_summary.csv", summarize_cases(rows), [
        "board", "texture_case", "coord_case", "mode", "variant", "frac",
        "rows", "exact_rows", "exact_pct", "mean_delta_pct",
        "mean_mismatch_pct", "max_mismatch_pct", "max_abs_channel_error",
    ])
    print(f"rows={len(rows)}")


if __name__ == "__main__":
    main()
