#!/usr/bin/env python3
"""Parse TGX RGB565 kernel benchmark telemetry into CSV summaries."""

from __future__ import annotations

import argparse
import csv
import re
from pathlib import Path


LINE_RE = re.compile(
    r"^KERNEL=(?P<kernel>\S+)\s+"
    r"VARIANT=(?P<variant>\S+)\s+"
    r"CORRECT=(?P<correct>[01])\s+"
    r"MISMATCHES=(?P<mismatches>\d+)\s+"
    r"TIME_US=(?P<time_us>\d+)\s+"
    r"CYCLES=(?P<cycles>\d+)\s+"
    r"ITERATIONS=(?P<iterations>\d+)\s+"
    r"COUNT=(?P<count>\d+)\s+"
    r"CHECKSUM=(?P<checksum>0x[0-9A-Fa-f]+)"
    r"(?:\s+NOTES=(?P<notes>.*))?$"
)


def reference_variant(kernel: str) -> str:
    if kernel.startswith("span_copy_"):
        return "copy16_loop"
    if kernel.startswith("span_fill_"):
        return "fill16_loop"
    if kernel.startswith("span_"):
        return "current_tgx_loop"
    return "current_tgx"


def parse_file(path: Path) -> tuple[str, str, list[dict[str, object]]]:
    board = ""
    config = ""
    rows: list[dict[str, object]] = []
    started = False
    ended = False
    for raw in path.read_text(errors="replace").splitlines():
        line = raw.strip()
        if line == "TGX_RGB565_BENCH_BEGIN":
            started = True
            continue
        if line == "TGX_RGB565_BENCH_END":
            ended = True
            continue
        if line.startswith("BOARD="):
            board = line.split("=", 1)[1]
            continue
        if line.startswith("CONFIG="):
            config = line.split("=", 1)[1]
            continue
        match = LINE_RE.match(line)
        if not match:
            continue
        item = match.groupdict()
        time_us = int(item["time_us"])
        cycles = int(item["cycles"])
        iterations = int(item["iterations"])
        count = int(item["count"])
        ops = iterations * count
        rows.append(
            {
                "source": str(path),
                "board": board,
                "config": config,
                "kernel": item["kernel"],
                "variant": item["variant"],
                "correct": int(item["correct"]),
                "mismatches": int(item["mismatches"]),
                "time_us": time_us,
                "cycles": cycles,
                "iterations": iterations,
                "count": count,
                "ops": ops,
                "ns_per_op": (time_us * 1000.0 / ops) if ops else 0.0,
                "cycles_per_op": (cycles / ops) if ops and cycles else 0.0,
                "checksum": item["checksum"],
                "notes": item.get("notes") or "",
                "start_marker_seen": int(started),
                "end_marker_seen": int(ended),
            }
        )
    return board, config, rows


def add_speedups(rows: list[dict[str, object]]) -> None:
    refs: dict[tuple[str, str], dict[str, object]] = {}
    for row in rows:
        key = (str(row["board"]), str(row["kernel"]))
        if row["variant"] == reference_variant(str(row["kernel"])):
            refs[key] = row
    for row in rows:
        key = (str(row["board"]), str(row["kernel"]))
        ref = refs.get(key)
        if not ref:
            row["speedup_vs_ref"] = ""
            row["delta_pct_vs_ref"] = ""
            continue
        t = float(row["time_us"])
        rt = float(ref["time_us"])
        if t <= 0 or rt <= 0:
            row["speedup_vs_ref"] = ""
            row["delta_pct_vs_ref"] = ""
        else:
            speedup = rt / t
            row["speedup_vs_ref"] = speedup
            row["delta_pct_vs_ref"] = (speedup - 1.0) * 100.0


def write_csv(path: Path, rows: list[dict[str, object]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    if not rows:
        path.write_text("")
        return
    fieldnames = list(rows[0].keys())
    with path.open("w", newline="") as fh:
        writer = csv.DictWriter(fh, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)


def build_summary(rows: list[dict[str, object]]) -> list[dict[str, object]]:
    summary: list[dict[str, object]] = []
    for row in rows:
        if row["correct"] != 1:
            continue
        if row["variant"] == reference_variant(str(row["kernel"])):
            continue
        delta = row.get("delta_pct_vs_ref")
        if delta == "":
            continue
        summary.append(
            {
                "board": row["board"],
                "kernel": row["kernel"],
                "variant": row["variant"],
                "delta_pct_vs_ref": delta,
                "speedup_vs_ref": row["speedup_vs_ref"],
                "ns_per_op": row["ns_per_op"],
                "cycles_per_op": row["cycles_per_op"],
                "mismatches": row["mismatches"],
                "checksum": row["checksum"],
            }
        )
    summary.sort(key=lambda r: (str(r["board"]), -float(r["delta_pct_vs_ref"])))
    return summary


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("telemetry", nargs="+", type=Path)
    parser.add_argument("--out", required=True, type=Path)
    parser.add_argument("--summary", required=True, type=Path)
    args = parser.parse_args()

    rows: list[dict[str, object]] = []
    for path in args.telemetry:
        _, _, parsed = parse_file(path)
        rows.extend(parsed)
    add_speedups(rows)
    write_csv(args.out, rows)
    write_csv(args.summary, build_summary(rows))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
