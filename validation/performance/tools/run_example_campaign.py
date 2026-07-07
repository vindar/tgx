#!/usr/bin/env python3
"""Run TGX real-example telemetry captures into a versioned baseline folder."""

from __future__ import annotations

import argparse
import csv
import json
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable


ROOT = Path(__file__).resolve().parents[3]
UPLOAD_SCRIPT = ROOT / "validation" / "performance" / "tools" / "upload_and_capture.ps1"


@dataclass(frozen=True)
class ExampleRun:
    board: str
    name: str
    sketch: str
    baud: int
    min_rows: int = 3
    timeout_s: int = 45
    line_regex: str = r"(\[TGX telemetry\]|fps=| FPS=| fps)"
    note: str = ""


EXAMPLES: list[ExampleRun] = [
    ExampleRun("teensy41", "borg_cube", r"examples\Teensy4\3D\borg_cube", 9600, 3, 45),
    ExampleRun("teensy41", "buddha", r"examples\Teensy4\3D\buddha", 9600, 3, 45),
    ExampleRun("teensy41", "characters", r"examples\Teensy4\3D\characters", 9600, 3, 45),
    ExampleRun("teensy41", "mars", r"examples\Teensy4\3D\mars", 9600, 5, 60),
    ExampleRun("teensy41", "multi_lighting", r"examples\Teensy4\3D\multi-lighting", 9600, 3, 45),
    ExampleRun("teensy41", "pointlight_room", r"examples\Teensy4\3D\pointlight_room", 9600, 3, 45),
    ExampleRun("teensy41", "pointlight_textured_meshes", r"examples\Teensy4\3D\pointlight_textured_meshes", 9600, 3, 45),
    ExampleRun("teensy41", "scream", r"examples\Teensy4\3D\scream", 9600, 5, 60),
    ExampleRun("teensy41", "spotlight_checkerboard", r"examples\Teensy4\3D\spotlight_checkerboard", 9600, 3, 45),
    ExampleRun("teensy41", "test_shading", r"examples\Teensy4\3D\test-shading", 9600, 5, 60),
    ExampleRun("teensy41", "test_texture", r"examples\Teensy4\3D\test-texture", 9600, 5, 60),

    ExampleRun("core2", "asteroid_demo", r"examples\M5Stack\asteroid_demo", 115200, 3, 45),
    ExampleRun("core2", "borg_cube", r"examples\M5Stack\borg_cube", 115200, 3, 45),
    ExampleRun("core2", "donkeykong", r"examples\M5Stack\donkeykong", 115200, 5, 60),
    ExampleRun("core2", "pointlight_room", r"examples\M5Stack\pointlight_room", 115200, 3, 45),
    ExampleRun("core2", "pointlight_textured_meshes", r"examples\M5Stack\pointlight_textured_meshes", 115200, 3, 45),
    ExampleRun("core2", "scream", r"examples\M5Stack\scream", 115200, 3, 45),
    ExampleRun("core2", "spotlight_checkerboard", r"examples\M5Stack\spotlight_checkerboard", 115200, 3, 45),

    ExampleRun("cores3", "asteroid_demo", r"examples\M5Stack\asteroid_demo", 115200, 3, 45),
    ExampleRun("cores3", "borg_cube", r"examples\M5Stack\borg_cube", 115200, 3, 45),
    ExampleRun("cores3", "donkeykong", r"examples\M5Stack\donkeykong", 115200, 5, 60),
    ExampleRun("cores3", "pointlight_room", r"examples\M5Stack\pointlight_room", 115200, 3, 45),
    ExampleRun("cores3", "pointlight_textured_meshes", r"examples\M5Stack\pointlight_textured_meshes", 115200, 3, 45),
    ExampleRun("cores3", "scream", r"examples\M5Stack\scream", 115200, 3, 45),
    ExampleRun("cores3", "spotlight_checkerboard", r"examples\M5Stack\spotlight_checkerboard", 115200, 3, 45),

    ExampleRun("picow", "asteroid_demo", r"examples\Pico_RP2040_RP2350\asteroid_demo", 115200, 3, 45),
    ExampleRun("picow", "borg_cube", r"examples\Pico_RP2040_RP2350\borg_cube", 115200, 3, 45),
    ExampleRun("picow", "bunny_fig", r"examples\Pico_RP2040_RP2350\bunny_fig", 115200, 5, 60),
    ExampleRun("picow", "pointlight_room", r"examples\Pico_RP2040_RP2350\pointlight_room", 115200, 3, 45),
    ExampleRun("picow", "pointlight_textured_meshes", r"examples\Pico_RP2040_RP2350\pointlight_textured_meshes", 115200, 3, 45),
    ExampleRun("picow", "scream", r"examples\Pico_RP2040_RP2350\scream", 115200, 3, 45),
    ExampleRun("picow", "spotlight_checkerboard", r"examples\Pico_RP2040_RP2350\spotlight_checkerboard", 115200, 3, 45),

    ExampleRun("pico2", "asteroid_demo", r"examples\Pico_RP2040_RP2350\asteroid_demo", 115200, 3, 45),
    ExampleRun("pico2", "borg_cube", r"examples\Pico_RP2040_RP2350\borg_cube", 115200, 3, 45),
    ExampleRun("pico2", "bunny_fig", r"examples\Pico_RP2040_RP2350\bunny_fig", 115200, 5, 60),
    ExampleRun("pico2", "pointlight_room", r"examples\Pico_RP2040_RP2350\pointlight_room", 115200, 3, 45),
    ExampleRun("pico2", "pointlight_textured_meshes", r"examples\Pico_RP2040_RP2350\pointlight_textured_meshes", 115200, 3, 45),
    ExampleRun("pico2", "scream", r"examples\Pico_RP2040_RP2350\scream", 115200, 3, 45),
    ExampleRun("pico2", "spotlight_checkerboard", r"examples\Pico_RP2040_RP2350\spotlight_checkerboard", 115200, 3, 45),
]


def selected_examples(boards: Iterable[str], names: Iterable[str]) -> list[ExampleRun]:
    board_set = {b.strip() for b in boards if b.strip()}
    name_set = {n.strip() for n in names if n.strip()}
    out = []
    for ex in EXAMPLES:
        if board_set and ex.board not in board_set:
            continue
        if name_set and ex.name not in name_set:
            continue
        out.append(ex)
    return out


def read_metadata(path: Path) -> dict:
    if not path.exists():
        return {}
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except Exception as exc:  # pragma: no cover - diagnostic path
        return {"run_status": "METADATA_PARSE_FAILED", "metadata_error": str(exc)}


def read_csv(path: Path) -> list[dict]:
    if not path.exists() or path.stat().st_size == 0:
        return []
    with path.open(newline="", encoding="utf-8-sig") as f:
        return list(csv.DictReader(f))


def write_csv(path: Path, rows: list[dict]) -> None:
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


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--baseline-root", required=True, type=Path)
    parser.add_argument("--library-root", required=True, type=Path)
    parser.add_argument("--version-label", required=True)
    parser.add_argument("--boards", default="teensy41,core2,cores3,picow,pico2")
    parser.add_argument("--examples", default="")
    parser.add_argument("--timeout-scale", default=1.0, type=float)
    parser.add_argument("--retry-count", default=1, type=int)
    parser.add_argument("--skip-existing-success", action="store_true")
    parser.add_argument("--dry-run", action="store_true")
    args = parser.parse_args()

    baseline_root = args.baseline_root.resolve()
    library_root = args.library_root.resolve()
    examples_root = baseline_root / "examples"
    manifest_path = examples_root / "example_manifest.csv"
    summary_path = examples_root / "example_summary.csv"
    examples_root.mkdir(parents=True, exist_ok=True)

    runs = selected_examples(args.boards.split(","), args.examples.split(","))
    rows_by_key: dict[tuple[str, str], dict] = {}
    for row in read_csv(manifest_path):
        rows_by_key[(row.get("board", ""), row.get("example", ""))] = row

    for ex in runs:
        sketch = (ROOT / ex.sketch).resolve()
        label = f"{args.version_label}_{ex.board}_{ex.name}"
        run_id = "".join(ch if ch.isalnum() or ch in "_.-" else "_" for ch in f"{ex.board}_{label}")
        metadata_path = examples_root / "parsed" / f"{run_id}.json"

        if args.skip_existing_success:
            meta = read_metadata(metadata_path)
            if meta.get("run_status") == "SUCCESS" and int(meta.get("parsed_telemetry_rows", 0) or 0) >= ex.min_rows:
                print(f"[skip] {ex.board}/{ex.name}: existing SUCCESS")
                rows_by_key[(ex.board, ex.name)] = {
                    "version": args.version_label,
                    "board": ex.board,
                    "example": ex.name,
                    "status": "SUCCESS",
                    "parsed_telemetry_rows": meta.get("parsed_telemetry_rows", ""),
                    "metadata": str(metadata_path),
                    "note": "existing_success",
                }
                continue

        if not sketch.exists():
            print(f"[skip] {ex.board}/{ex.name}: missing sketch {sketch}")
            rows_by_key[(ex.board, ex.name)] = {
                "version": args.version_label,
                "board": ex.board,
                "example": ex.name,
                "status": "SKIPPED",
                "reason": "missing_sketch",
                "sketch": str(sketch),
            }
            continue

        command = [
            "powershell",
            "-ExecutionPolicy",
            "Bypass",
            "-File",
            str(UPLOAD_SCRIPT),
            "-Board",
            ex.board,
            "-Sketch",
            str(sketch),
            "-Label",
            label,
            "-ParseMode",
            "example",
            "-Baud",
            str(ex.baud),
            "-LineRegex",
            ex.line_regex,
            "-MinTelemetryLines",
            str(ex.min_rows),
            "-TimeoutSeconds",
            str(max(1, int(ex.timeout_s * args.timeout_scale))),
            "-RetryCount",
            str(args.retry_count),
            "-LibraryRoot",
            str(library_root),
            "-OutputRoot",
            str(examples_root),
        ]

        print(f"[run] {ex.board}/{ex.name}")
        print("+ " + " ".join(command))
        exit_code = 0
        if not args.dry_run:
            proc = subprocess.run(command, cwd=str(ROOT), text=True, encoding="utf-8", errors="replace")
            exit_code = proc.returncode

        meta = read_metadata(metadata_path)
        row = {
            "version": args.version_label,
            "board": ex.board,
            "example": ex.name,
            "sketch": str(sketch),
            "library_root": str(library_root),
            "status": meta.get("run_status", "DRY_RUN" if args.dry_run else "NO_METADATA"),
            "exit_code": exit_code,
            "upload_status": meta.get("upload_status", ""),
            "serial_status": meta.get("serial_status", ""),
            "parsed_telemetry_rows": meta.get("parsed_telemetry_rows", ""),
            "captured_lines": meta.get("captured_lines", ""),
            "metadata": str(metadata_path),
            "telemetry": meta.get("telemetry_file", ""),
            "log": meta.get("log_file", ""),
            "note": ex.note,
        }
        rows_by_key[(ex.board, ex.name)] = row
        write_csv(manifest_path, list(rows_by_key.values()))
        if row["status"] != "SUCCESS":
            print(f"[warn] {ex.board}/{ex.name}: {row['status']} rows={row['parsed_telemetry_rows']}")
        else:
            print(f"[ok] {ex.board}/{ex.name}: rows={row['parsed_telemetry_rows']}")

    rows = list(rows_by_key.values())
    write_csv(manifest_path, rows)
    write_csv(summary_path, rows)
    if args.dry_run:
        failed: list[dict] = []
    else:
        failed = [r for r in rows if r.get("status") not in ("SUCCESS", "SKIPPED")]
    print(f"[done] examples={len(rows)} failed_or_partial={len(failed)} manifest={manifest_path}")
    return 0 if not failed else 2


if __name__ == "__main__":
    raise SystemExit(main())
