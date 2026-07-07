#!/usr/bin/env python3
"""Compare two aggregated TGX performance baselines."""

from __future__ import annotations

import argparse
import csv
from pathlib import Path


def read_csv(path: Path) -> list[dict[str, str]]:
    if not path.exists():
        return []
    with path.open("r", newline="", encoding="utf-8") as handle:
        return list(csv.DictReader(handle))


def write_csv(path: Path, rows: list[dict[str, object]], fields: list[str]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, fieldnames=fields)
        writer.writeheader()
        for row in rows:
            writer.writerow(row)


def f(row: dict[str, str], key: str) -> float | None:
    value = row.get(key, "")
    if value == "":
        return None
    try:
        return float(value)
    except ValueError:
        return None


def pct(now: float | None, old: float | None) -> float | None:
    if now is None or old is None or old == 0.0:
        return None
    return (now / old - 1.0) * 100.0


def fmt(value: float | None) -> str:
    if value is None:
        return ""
    return f"{value:.3f}"


def compare_benchmark(current_root: Path, baseline_root: Path) -> list[dict[str, object]]:
    cur_rows = [
        row for row in read_csv(current_root / "summary" / "benchmark_results.csv")
        if row.get("status") == "OK"
    ]
    base_rows = [
        row for row in read_csv(baseline_root / "summary" / "benchmark_results.csv")
        if row.get("status") == "OK"
    ]
    base = {
        (row.get("board_id"), row.get("module"), row.get("test"), row.get("width"), row.get("height")): row
        for row in base_rows
    }
    out: list[dict[str, object]] = []
    for cur in cur_rows:
        key = (cur.get("board_id"), cur.get("module"), cur.get("test"), cur.get("width"), cur.get("height"))
        old = base.get(key)
        if old is None:
            continue
        cur_fps = f(cur, "fps")
        old_fps = f(old, "fps")
        cur_us = f(cur, "mean_us")
        old_us = f(old, "mean_us")
        out.append({
            "board": cur.get("board_id", ""),
            "module": cur.get("module", ""),
            "test": cur.get("test", ""),
            "width": cur.get("width", ""),
            "height": cur.get("height", ""),
            "current_fps": fmt(cur_fps),
            "baseline_fps": fmt(old_fps),
            "fps_delta_pct": fmt(pct(cur_fps, old_fps)),
            "current_mean_us": fmt(cur_us),
            "baseline_mean_us": fmt(old_us),
            "mean_us_delta_pct": fmt(pct(cur_us, old_us)),
            "checksum_same": "1" if cur.get("checksum") == old.get("checksum") else "0",
        })
    return out


def compare_examples(current_root: Path, baseline_root: Path) -> list[dict[str, object]]:
    cur_rows = [
        row for row in read_csv(current_root / "summary" / "example_summary.csv")
        if row.get("status") == "SUCCESS"
    ]
    base_rows = [
        row for row in read_csv(baseline_root / "summary" / "example_summary.csv")
        if row.get("status") == "SUCCESS"
    ]
    base = {(row.get("board"), row.get("example")): row for row in base_rows}
    out: list[dict[str, object]] = []
    for cur in cur_rows:
        key = (cur.get("board"), cur.get("example"))
        old = base.get(key)
        if old is None:
            continue
        cur_fps = f(cur, "fps_avg")
        old_fps = f(old, "fps_avg")
        out.append({
            "board": cur.get("board", ""),
            "example": cur.get("example", ""),
            "current_fps_avg": fmt(cur_fps),
            "baseline_fps_avg": fmt(old_fps),
            "fps_delta_pct": fmt(pct(cur_fps, old_fps)),
            "current_rows": cur.get("fps_rows", ""),
            "baseline_rows": old.get("fps_rows", ""),
        })
    return out


def numeric(row: dict[str, object], key: str) -> float:
    value = row.get(key, "")
    try:
        return float(value)  # type: ignore[arg-type]
    except (TypeError, ValueError):
        return 0.0


def make_report(path: Path, current_label: str, baseline_label: str,
                bench: list[dict[str, object]], examples: list[dict[str, object]]) -> None:
    regress_bench = sorted([r for r in bench if numeric(r, "fps_delta_pct") < -0.7], key=lambda r: numeric(r, "fps_delta_pct"))[:20]
    improve_bench = sorted([r for r in bench if numeric(r, "fps_delta_pct") > 0.7], key=lambda r: numeric(r, "fps_delta_pct"), reverse=True)[:20]
    regress_ex = sorted([r for r in examples if numeric(r, "fps_delta_pct") < -0.7], key=lambda r: numeric(r, "fps_delta_pct"))[:20]
    improve_ex = sorted([r for r in examples if numeric(r, "fps_delta_pct") > 0.7], key=lambda r: numeric(r, "fps_delta_pct"), reverse=True)[:20]

    def table(rows: list[dict[str, object]], fields: list[str]) -> list[str]:
        if not rows:
            return ["None."]
        lines = ["| " + " | ".join(fields) + " |", "| " + " | ".join(["---"] * len(fields)) + " |"]
        for row in rows:
            lines.append("| " + " | ".join(str(row.get(field, "")) for field in fields) + " |")
        return lines

    lines: list[str] = [
        f"# {current_label} vs {baseline_label}",
        "",
        f"- Comparable benchmark rows: {len(bench)}",
        f"- Comparable example rows: {len(examples)}",
        "- Deltas are FPS deltas: positive is faster in the current baseline.",
        "- Rows absent from one side are not compared here; see each baseline manifest for unsupported/failed modules.",
        "",
        "## Benchmark Regressions",
        *table(regress_bench, ["board", "module", "test", "fps_delta_pct", "current_fps", "baseline_fps", "checksum_same"]),
        "",
        "## Benchmark Improvements",
        *table(improve_bench, ["board", "module", "test", "fps_delta_pct", "current_fps", "baseline_fps", "checksum_same"]),
        "",
        "## Example Regressions",
        *table(regress_ex, ["board", "example", "fps_delta_pct", "current_fps_avg", "baseline_fps_avg"]),
        "",
        "## Example Improvements",
        *table(improve_ex, ["board", "example", "fps_delta_pct", "current_fps_avg", "baseline_fps_avg"]),
        "",
    ]
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("\n".join(lines), encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--current-root", required=True, type=Path)
    parser.add_argument("--baseline-root", required=True, type=Path)
    parser.add_argument("--current-label", required=True)
    parser.add_argument("--baseline-label", required=True)
    parser.add_argument("--out-dir", required=True, type=Path)
    args = parser.parse_args()

    bench = compare_benchmark(args.current_root, args.baseline_root)
    examples = compare_examples(args.current_root, args.baseline_root)
    stem = f"{args.current_label}_vs_{args.baseline_label}".replace("/", "_").replace("\\", "_")
    write_csv(args.out_dir / f"{stem}_benchmark.csv", bench, [
        "board", "module", "test", "width", "height",
        "current_fps", "baseline_fps", "fps_delta_pct",
        "current_mean_us", "baseline_mean_us", "mean_us_delta_pct", "checksum_same",
    ])
    write_csv(args.out_dir / f"{stem}_examples.csv", examples, [
        "board", "example", "current_fps_avg", "baseline_fps_avg",
        "fps_delta_pct", "current_rows", "baseline_rows",
    ])
    make_report(args.out_dir / f"{stem}_report.md", args.current_label, args.baseline_label, bench, examples)
    print(f"benchmark_rows={len(bench)} example_rows={len(examples)} out={args.out_dir}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
