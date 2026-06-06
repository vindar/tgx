#!/usr/bin/env python3
import argparse
import csv
import re
from pathlib import Path


LINE_RE = re.compile(
    r"^KERNEL=(?P<kernel>\S+)\s+"
    r"VARIANT=(?P<variant>\S+)\s+"
    r"LEN=(?P<len>\d+)\s+"
    r"ALIGN=(?P<align>\d+)\s+"
    r"PATTERN=(?P<pattern>\S+)\s+"
    r"CORRECT=(?P<correct>[01])\s+"
    r"MISMATCHES=(?P<mismatches>\d+)\s+"
    r"TIME_US=(?P<time_us>\d+)\s+"
    r"CYCLES=(?P<cycles>\d+)\s+"
    r"REPEATS=(?P<repeats>\d+)\s+"
    r"OPS=(?P<ops>\d+)\s+"
    r"FB_CHECKSUM=0x(?P<fb_checksum>[0-9A-Fa-f]+)\s+"
    r"Z_CHECKSUM=0x(?P<z_checksum>[0-9A-Fa-f]+)"
)


def parse_file(path: Path):
    board = ""
    config = ""
    start_seen = False
    end_seen = False
    rows = []
    for raw in path.read_text(errors="replace").splitlines():
        line = raw.strip()
        if line == "TGX_RASTER_SPAN_BENCH_BEGIN":
            start_seen = True
            continue
        if line == "TGX_RASTER_SPAN_BENCH_END":
            end_seen = True
            continue
        if line.startswith("BOARD="):
            board = line.split("=", 1)[1]
            continue
        if line.startswith("CONFIG="):
            config = line.split("=", 1)[1]
            continue
        m = LINE_RE.match(line)
        if not m:
            continue
        d = m.groupdict()
        d["source"] = str(path)
        d["board"] = board
        d["config"] = config
        d["start_marker_seen"] = int(start_seen)
        d["end_marker_seen"] = 0
        d["len"] = int(d["len"])
        d["align"] = int(d["align"])
        d["correct"] = int(d["correct"])
        d["mismatches"] = int(d["mismatches"])
        d["time_us"] = int(d["time_us"])
        d["cycles"] = int(d["cycles"])
        d["repeats"] = int(d["repeats"])
        d["ops"] = int(d["ops"])
        d["ns_per_op"] = (d["time_us"] * 1000.0) / d["ops"] if d["ops"] else 0.0
        d["cycles_per_op"] = d["cycles"] / d["ops"] if d["ops"] else 0.0
        rows.append(d)
    for row in rows:
        row["end_marker_seen"] = int(end_seen)
    return rows


def add_deltas(rows):
    refs = {}
    for row in rows:
        key = (row["board"], row["kernel"], row["len"], row["align"], row["pattern"])
        refs.setdefault(key, row)
    for row in rows:
        key = (row["board"], row["kernel"], row["len"], row["align"], row["pattern"])
        ref = refs[key]
        row["ref_variant"] = ref["variant"]
        row["speedup_vs_ref"] = ref["ns_per_op"] / row["ns_per_op"] if row["ns_per_op"] else 0.0
        row["delta_pct_vs_ref"] = (row["speedup_vs_ref"] - 1.0) * 100.0


def write_csv(path: Path, rows):
    if not rows:
        path.write_text("")
        return
    fields = [
        "source", "board", "config", "kernel", "variant", "ref_variant",
        "len", "align", "pattern", "correct", "mismatches",
        "time_us", "cycles", "repeats", "ops", "ns_per_op", "cycles_per_op",
        "speedup_vs_ref", "delta_pct_vs_ref", "fb_checksum", "z_checksum",
        "start_marker_seen", "end_marker_seen",
    ]
    with path.open("w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=fields, extrasaction="ignore")
        w.writeheader()
        w.writerows(rows)


def write_summary(path: Path, rows):
    grouped = {}
    for row in rows:
        key = (row["board"], row["kernel"], row["variant"])
        grouped.setdefault(key, []).append(row)
    out = []
    for (board, kernel, variant), items in grouped.items():
        exact = [r for r in items if r["correct"] and r["mismatches"] == 0]
        deltas = [r["delta_pct_vs_ref"] for r in exact if r["variant"] != r["ref_variant"]]
        all_deltas = [r["delta_pct_vs_ref"] for r in items if r["variant"] != r["ref_variant"]]
        out.append({
            "board": board,
            "kernel": kernel,
            "variant": variant,
            "rows": len(items),
            "correct_rows": len(exact),
            "invalid_rows": len(items) - len(exact),
            "mean_delta_pct_exact": sum(deltas) / len(deltas) if deltas else 0.0,
            "best_delta_pct": max(all_deltas) if all_deltas else 0.0,
            "worst_delta_pct": min(all_deltas) if all_deltas else 0.0,
        })
    out.sort(key=lambda r: (r["board"], r["kernel"], r["variant"]))
    with path.open("w", newline="") as f:
        fields = ["board", "kernel", "variant", "rows", "correct_rows", "invalid_rows", "mean_delta_pct_exact", "best_delta_pct", "worst_delta_pct"]
        w = csv.DictWriter(f, fieldnames=fields)
        w.writeheader()
        w.writerows(out)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("telemetry", nargs="+", type=Path)
    ap.add_argument("--out", type=Path, required=True)
    ap.add_argument("--summary", type=Path, required=True)
    args = ap.parse_args()

    rows = []
    for path in args.telemetry:
        rows.extend(parse_file(path))
    add_deltas(rows)
    write_csv(args.out, rows)
    write_summary(args.summary, rows)


if __name__ == "__main__":
    main()
