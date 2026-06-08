#!/usr/bin/env python3
import argparse
import csv
import re
from collections import defaultdict
from pathlib import Path


KV_RE = re.compile(r"([A-Z_]+)=([^ ]+)")


def parse_file(path: Path):
    board = ""
    rows = []
    for raw in path.read_text(errors="replace").splitlines():
        line = raw.strip()
        if line.startswith("BOARD="):
            board = line.split("=", 1)[1]
            continue
        if not line.startswith("KERNEL=jpeg_rgb565_copy "):
            continue
        data = {k.lower(): v for k, v in KV_RE.findall(line)}
        data["board"] = board
        data["source_file"] = str(path)
        for key in ("w", "h", "stride", "in_bounds", "correct", "mismatches", "repeats", "time_us"):
            if key in data:
                data[key] = int(data[key])
        if "opacity" in data:
            data["opacity"] = float(data["opacity"])
        rows.append(data)
    return rows


def build_summary(rows):
    by_case = defaultdict(dict)
    for row in rows:
        key = (
            row.get("board", ""),
            row.get("case", ""),
            row.get("w", 0),
            row.get("h", 0),
            row.get("stride", 0),
            row.get("opacity", 0.0),
            row.get("in_bounds", 0),
        )
        by_case[key][row.get("variant", "")] = row

    summary = []
    for key, variants in sorted(by_case.items()):
        old = variants.get("old_loop")
        mem = variants.get("row_memcpy")
        prod = variants.get("production")
        if not old or not mem:
            continue
        old_us = old["time_us"]
        mem_us = mem["time_us"]
        speedup = ((old_us / mem_us) - 1.0) * 100.0 if mem_us > 0 else 0.0
        prod_us = prod["time_us"] if prod else ""
        prod_speedup = ((old_us / prod_us) - 1.0) * 100.0 if prod and prod_us > 0 else ""
        summary.append({
            "board": key[0],
            "case": key[1],
            "w": key[2],
            "h": key[3],
            "stride": key[4],
            "opacity": key[5],
            "in_bounds": key[6],
            "correct": mem.get("correct", 0),
            "mismatches": mem.get("mismatches", 0),
            "repeats": mem.get("repeats", 0),
            "old_time_us": old_us,
            "memcpy_time_us": mem_us,
            "speedup_pct": f"{speedup:.3f}",
            "production_time_us": prod_us,
            "production_speedup_pct": f"{prod_speedup:.3f}" if prod else "",
            "production_correct": prod.get("correct", "") if prod else "",
            "production_mismatches": prod.get("mismatches", "") if prod else "",
            "old_checksum": old.get("checksum", ""),
            "memcpy_checksum": mem.get("checksum", ""),
            "production_checksum": prod.get("checksum", "") if prod else "",
        })
    return summary


def write_csv(path: Path, rows, fieldnames):
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames, extrasaction="ignore")
        writer.writeheader()
        writer.writerows(rows)


def main():
    parser = argparse.ArgumentParser(description="Parse TGX JPEG RGB565 copy microbench telemetry.")
    parser.add_argument("inputs", nargs="+", type=Path)
    parser.add_argument("--rows-out", type=Path, required=True)
    parser.add_argument("--summary-out", type=Path, required=True)
    args = parser.parse_args()

    rows = []
    for path in args.inputs:
        rows.extend(parse_file(path))

    row_fields = [
        "source_file", "board", "kernel", "variant", "case", "w", "h", "stride",
        "opacity", "in_bounds", "correct", "mismatches", "repeats", "time_us",
        "checksum",
    ]
    summary_fields = [
        "board", "case", "w", "h", "stride", "opacity", "in_bounds", "correct",
        "mismatches", "repeats", "old_time_us", "memcpy_time_us", "speedup_pct",
        "production_time_us", "production_speedup_pct", "production_correct",
        "production_mismatches", "old_checksum", "memcpy_checksum",
        "production_checksum",
    ]
    summary = build_summary(rows)
    write_csv(args.rows_out, rows, row_fields)
    write_csv(args.summary_out, summary, summary_fields)

    bad = [r for r in summary if int(r["correct"]) != 1 or int(r["mismatches"]) != 0]
    print(f"parsed_rows={len(rows)} summary_rows={len(summary)} mismatching_cases={len(bad)}")


if __name__ == "__main__":
    main()
