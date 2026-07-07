#!/usr/bin/env python3
"""Aggregate one versioned TGX validation baseline into compact CSV files."""

from __future__ import annotations

import argparse
import csv
import json
import re
import statistics
from pathlib import Path
from typing import Iterable


def read_csv(path: Path) -> list[dict]:
    if not path.exists() or path.stat().st_size == 0:
        return []
    with path.open(newline="", encoding="utf-8-sig") as f:
        return list(csv.DictReader(f))


def write_csv(path: Path, rows: Iterable[dict]) -> None:
    rows = list(rows)
    path.parent.mkdir(parents=True, exist_ok=True)
    fieldnames: list[str] = []
    for row in rows:
        for key in row:
            if key not in fieldnames:
                fieldnames.append(key)
    with path.open("w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)


def read_json(path: Path) -> dict:
    if not path.exists():
        return {}
    try:
        return json.loads(path.read_text(encoding="utf-8-sig"))
    except Exception as exc:  # pragma: no cover - diagnostic path
        return {"json_error": str(exc)}


def as_float(value: object) -> float | None:
    try:
        return float(str(value).strip().replace(",", "."))
    except (TypeError, ValueError):
        return None


def aggregate_benchmark(root: Path, version: str) -> tuple[list[dict], list[dict]]:
    benchmark_root = root / "benchmark3d"
    rows: list[dict] = []
    runs: list[dict] = []
    for manifest_path in sorted(benchmark_root.glob("*/batch_manifest.json")):
        manifest = read_json(manifest_path)
        batch_dir = manifest_path.parent
        for run in manifest.get("runs", []):
            out = {
                "version": version,
                "batch": batch_dir.name,
                "board": run.get("board", ""),
                "module": run.get("module", ""),
                "status": run.get("status", ""),
                "run_dir": run.get("run_dir", ""),
                "results_csv": run.get("results_csv", ""),
                "error": run.get("error", ""),
            }
            runs.append(out)
        for row in read_csv(batch_dir / "batch_results.csv"):
            row = dict(row)
            row["version"] = version
            row["batch"] = batch_dir.name
            rows.append(row)
    return rows, runs


def parse_simple_fps_line(line: str) -> dict:
    out: dict[str, str] = {}
    for key in ("fps", "render_us", "avg_us", "min_us", "max_us", "frames"):
        match = re.search(rf"{key}=([0-9]+(?:\.[0-9]+)?)", line)
        if match:
            out[key] = match.group(1)
    return out


def aggregate_examples(root: Path, version: str) -> tuple[list[dict], list[dict]]:
    examples_root = root / "examples"
    manifest_rows = read_csv(examples_root / "example_manifest.csv")
    telemetry_rows: list[dict] = []
    summary: list[dict] = []

    def resolved_metadata_path(value: str) -> Path:
        path = Path(value)
        if path.is_absolute():
            return path
        repo_root = root.parents[4] if len(root.parents) > 4 else Path.cwd()
        candidate = (repo_root / path).resolve()
        if candidate.exists():
            return candidate
        return (root / path).resolve()

    if manifest_rows:
        metadata_paths: list[tuple[dict, Path]] = []
        for row in manifest_rows:
            meta_value = str(row.get("metadata", ""))
            metadata_paths.append((row, resolved_metadata_path(meta_value) if meta_value else Path()))
    else:
        metadata_paths = [
            ({}, meta_path)
            for meta_path in sorted((examples_root / "parsed").glob("*.json"))
        ]

    for manifest_row, meta_path in metadata_paths:
        meta = read_json(meta_path)
        board = str(manifest_row.get("board") or meta.get("board", ""))
        label = str(meta.get("label", meta_path.stem if meta_path else ""))
        example = str(manifest_row.get("example") or meta.get("example", ""))
        parts = label.split("_")
        if example:
            pass
        elif board and f"_{board}_" in label:
            example = label.split(f"_{board}_", 1)[1]
        elif len(parts) >= 3:
            example = parts[-1]
        else:
            example = label

        csv_path = meta_path.with_name(meta_path.stem + "_example.csv") if meta_path else Path()
        fps_values: list[float] = []
        for row in read_csv(csv_path):
            out = dict(row)
            out["version"] = version
            out["board"] = board
            out["example"] = example
            out["run_status"] = meta.get("run_status", "")
            out["metadata"] = str(meta_path)
            fps = as_float(out.get("fps"))
            if fps is None:
                parsed = parse_simple_fps_line(str(out.get("line", "")))
                out.update({k: v for k, v in parsed.items() if k not in out or not out[k]})
                fps = as_float(out.get("fps"))
            if fps is not None:
                fps_values.append(fps)
            telemetry_rows.append(out)

        summary.append({
            "version": version,
            "board": board,
            "example": example,
            "status": manifest_row.get("status") or meta.get("run_status", ""),
            "parsed_telemetry_rows": manifest_row.get("parsed_telemetry_rows") or meta.get("parsed_telemetry_rows", ""),
            "captured_lines": meta.get("captured_lines", ""),
            "fps_rows": len(fps_values),
            "fps_avg": f"{statistics.mean(fps_values):.6g}" if fps_values else "",
            "fps_min": f"{min(fps_values):.6g}" if fps_values else "",
            "fps_max": f"{max(fps_values):.6g}" if fps_values else "",
            "metadata": str(meta_path) if meta_path else manifest_row.get("metadata", ""),
            "note": manifest_row.get("note", ""),
        })
    return telemetry_rows, summary


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--baseline-root", required=True, type=Path)
    parser.add_argument("--version-label", required=True)
    args = parser.parse_args()

    root = args.baseline_root.resolve()
    summary_root = root / "summary"
    bench_rows, bench_runs = aggregate_benchmark(root, args.version_label)
    example_rows, example_summary = aggregate_examples(root, args.version_label)

    write_csv(summary_root / "benchmark_results.csv", bench_rows)
    write_csv(summary_root / "benchmark_runs.csv", bench_runs)
    write_csv(summary_root / "example_telemetry.csv", example_rows)
    write_csv(summary_root / "example_summary.csv", example_summary)

    info = {
        "version": args.version_label,
        "baseline_root": str(root),
        "benchmark_result_rows": len(bench_rows),
        "benchmark_runs": len(bench_runs),
        "example_telemetry_rows": len(example_rows),
        "example_runs": len(example_summary),
    }
    (summary_root / "summary.json").write_text(json.dumps(info, indent=2), encoding="utf-8")
    print(json.dumps(info, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
