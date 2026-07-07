#!/usr/bin/env python3
"""Run the Teensy 4.x Renderer3D flash-placement macro benchmark matrix.

This is a small orchestrator around validation/benchmark3d/tools/run_bench3d.py.
It runs the same Teensy4 benchmark modules several times with different
TGX_BENCH_FLASH_* defines, then compares each candidate with the "none"
candidate.
"""

from __future__ import annotations

import argparse
import csv
import datetime as _dt
import json
import os
from pathlib import Path
import subprocess
import sys
from typing import Iterable


ROOT = Path(__file__).resolve().parents[3]
RUN_BENCH3D = ROOT / "validation" / "benchmark3d" / "tools" / "run_bench3d.py"
DEFAULT_OUT_ROOT = (
    ROOT / "validation" / "local_results" / "performance" / "investigations"
    / (_dt.datetime.now().strftime("%Y-%m-%d") + "-teensy4-flash-macro-matrix")
)

MATRIX: dict[str, list[str]] = {
    "none": [],
    "sphere": ["TGX_BENCH_FLASH_SPHERE"],
    "truncated_cone": ["TGX_BENCH_FLASH_TRUNCATED_CONE"],
    "mesh3d": ["TGX_BENCH_FLASH_MESH3D"],
    "mesh3dv2": ["TGX_BENCH_FLASH_MESH3DV2"],
    "sphere_cone": ["TGX_BENCH_FLASH_SPHERE", "TGX_BENCH_FLASH_TRUNCATED_CONE"],
    "mesh_all": ["TGX_BENCH_FLASH_MESH3D", "TGX_BENCH_FLASH_MESH3DV2"],
    "all": [
        "TGX_BENCH_FLASH_SPHERE",
        "TGX_BENCH_FLASH_TRUNCATED_CONE",
        "TGX_BENCH_FLASH_MESH3D",
        "TGX_BENCH_FLASH_MESH3DV2",
    ],
}


def git(args: list[str]) -> str:
    proc = subprocess.run(
        ["git", *args],
        cwd=str(ROOT),
        text=True,
        encoding="utf-8",
        errors="replace",
        stdout=subprocess.PIPE,
        stderr=subprocess.DEVNULL,
    )
    return proc.stdout.strip() if proc.returncode == 0 else ""


def git_info() -> dict[str, object]:
    status = git(["status", "--short"])
    return {
        "commit": git(["rev-parse", "--short", "HEAD"]),
        "dirty": bool(status),
        "status_short": status,
    }


def split_csv(value: str) -> list[str]:
    return [item.strip() for item in value.split(",") if item.strip()]


def select_candidates(value: str) -> list[str]:
    if value == "all":
        return list(MATRIX)
    names = split_csv(value)
    unknown = [name for name in names if name not in MATRIX]
    if unknown:
        raise SystemExit("Unknown candidate(s): " + ", ".join(unknown))
    if "none" not in names:
        names.insert(0, "none")
    return names


def run_command(command: list[str], env: dict[str, str], log_path: Path, dry_run: bool) -> int:
    log_path.parent.mkdir(parents=True, exist_ok=True)
    log_path.write_text("+ " + " ".join(command) + "\n", encoding="utf-8")
    print("+ " + " ".join(command))
    if dry_run:
        return 0

    proc = subprocess.run(
        command,
        cwd=str(ROOT),
        env=env,
        text=True,
        encoding="utf-8",
        errors="replace",
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    log_path.write_text("+ " + " ".join(command) + "\n" + proc.stdout, encoding="utf-8")
    print(proc.stdout)
    return proc.returncode


def latest_output_dir(candidate_dir: Path) -> Path | None:
    batch_dirs = sorted(
        [path for path in candidate_dir.glob("*_batch") if (path / "batch_results.csv").exists()]
    )
    if batch_dirs:
        return batch_dirs[-1]

    run_dirs = sorted(
        [path for path in candidate_dir.iterdir() if path.is_dir() and (path / "results.csv").exists()]
    ) if candidate_dir.exists() else []
    return run_dirs[-1] if run_dirs else None


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
            writer.writerow({field: row.get(field, "") for field in fields})


def load_results(output_dir: Path) -> list[dict[str, str]]:
    batch_csv = output_dir / "batch_results.csv"
    if batch_csv.exists():
        return read_csv(batch_csv)
    return read_csv(output_dir / "results.csv")


def result_key(row: dict[str, str]) -> tuple[str, str, str, str]:
    return (
        row.get("module", ""),
        row.get("test", ""),
        row.get("width", ""),
        row.get("height", ""),
    )


def float_or_none(value: str | None) -> float | None:
    if value in (None, ""):
        return None
    try:
        return float(value)
    except ValueError:
        return None


def delta_pct(now: float | None, old: float | None) -> float | None:
    if now is None or old is None or old == 0.0:
        return None
    return (now / old - 1.0) * 100.0


def fmt_float(value: float | None) -> str:
    return "" if value is None else f"{value:.3f}"


def build_size_from_manifest(manifest: Path) -> tuple[str, dict[str, int]]:
    data = json.loads(manifest.read_text(encoding="utf-8"))
    module = str(data.get("module", ""))
    size = data.get("build_size", {})
    flash = size.get("teensy_flash", {}) if isinstance(size, dict) else {}
    ram1 = size.get("teensy_ram1", {}) if isinstance(size, dict) else {}
    ram2 = size.get("teensy_ram2", {}) if isinstance(size, dict) else {}

    def get(mapping: object, key: str) -> int:
        if isinstance(mapping, dict):
            value = mapping.get(key, 0)
            return int(value) if isinstance(value, int) else int(str(value or 0))
        return 0

    return module, {
        "flash_code": get(flash, "code"),
        "flash_data": get(flash, "data"),
        "ram1_variables": get(ram1, "variables"),
        "ram1_code": get(ram1, "code"),
        "ram1_padding": get(ram1, "padding"),
        "ram1_free": get(ram1, "free"),
        "ram2_variables": get(ram2, "variables"),
        "ram2_free": get(ram2, "free"),
    }


def load_sizes(output_dir: Path) -> dict[str, dict[str, int]]:
    manifests = sorted(output_dir.glob("runs/*/manifest.json"))
    if not manifests and (output_dir / "manifest.json").exists():
        manifests = [output_dir / "manifest.json"]

    sizes: dict[str, dict[str, int]] = {}
    for manifest in manifests:
        module, values = build_size_from_manifest(manifest)
        if module:
            sizes[module] = values
    return sizes


def write_summary(
    out_root: Path,
    candidate_outputs: dict[str, Path],
    candidate_status: list[dict[str, object]],
) -> None:
    summary_dir = out_root / "summary"
    base_output = candidate_outputs.get("none")
    if base_output is None:
        return

    base_results = {
        result_key(row): row for row in load_results(base_output)
        if row.get("status") == "OK"
    }
    base_sizes = load_sizes(base_output)

    perf_rows: list[dict[str, object]] = []
    size_rows: list[dict[str, object]] = []

    for candidate, output_dir in candidate_outputs.items():
        if candidate == "none":
            continue

        for row in load_results(output_dir):
            if row.get("status") != "OK":
                continue
            base = base_results.get(result_key(row))
            if base is None:
                continue

            mean_us = float_or_none(row.get("mean_us"))
            base_mean_us = float_or_none(base.get("mean_us"))
            fps = float_or_none(row.get("fps"))
            base_fps = float_or_none(base.get("fps"))
            perf_rows.append({
                "candidate": candidate,
                "module": row.get("module", ""),
                "test": row.get("test", ""),
                "width": row.get("width", ""),
                "height": row.get("height", ""),
                "base_mean_us": fmt_float(base_mean_us),
                "mean_us": fmt_float(mean_us),
                "time_delta_pct": fmt_float(delta_pct(mean_us, base_mean_us)),
                "base_fps": fmt_float(base_fps),
                "fps": fmt_float(fps),
                "fps_delta_pct": fmt_float(delta_pct(fps, base_fps)),
            })

        sizes = load_sizes(output_dir)
        for module, values in sorted(sizes.items()):
            base = base_sizes.get(module)
            if base is None:
                continue
            row: dict[str, object] = {"candidate": candidate, "module": module}
            for key in [
                "flash_code",
                "flash_data",
                "ram1_variables",
                "ram1_code",
                "ram1_padding",
                "ram1_free",
                "ram2_variables",
                "ram2_free",
            ]:
                row[key] = values.get(key, 0)
                row[key + "_delta"] = values.get(key, 0) - base.get(key, 0)
            size_rows.append(row)

    write_csv(
        summary_dir / "candidate_status.csv",
        candidate_status,
        ["candidate", "flags", "status", "returncode", "output_dir", "log"],
    )
    write_csv(
        summary_dir / "perf_delta_vs_none.csv",
        perf_rows,
        [
            "candidate",
            "module",
            "test",
            "width",
            "height",
            "base_mean_us",
            "mean_us",
            "time_delta_pct",
            "base_fps",
            "fps",
            "fps_delta_pct",
        ],
    )
    write_csv(
        summary_dir / "size_delta_vs_none.csv",
        size_rows,
        [
            "candidate",
            "module",
            "flash_code",
            "flash_code_delta",
            "flash_data",
            "flash_data_delta",
            "ram1_variables",
            "ram1_variables_delta",
            "ram1_code",
            "ram1_code_delta",
            "ram1_padding",
            "ram1_padding_delta",
            "ram1_free",
            "ram1_free_delta",
            "ram2_variables",
            "ram2_variables_delta",
            "ram2_free",
            "ram2_free_delta",
        ],
    )

    worst = sorted(
        perf_rows,
        key=lambda row: float(row["time_delta_pct"]) if row.get("time_delta_pct") else -999.0,
        reverse=True,
    )[:10]
    best_size = sorted(
        size_rows,
        key=lambda row: int(row.get("ram1_code_delta", 0)),
    )[:10]

    lines = [
        "# Teensy4 Renderer3D Flash Macro Matrix Summary",
        "",
        "Reference candidate: `none`.",
        "",
        "Generated files:",
        "",
        "- `candidate_status.csv`",
        "- `perf_delta_vs_none.csv`",
        "- `size_delta_vs_none.csv`",
        "",
        "## Worst Time Deltas",
        "",
        "| Candidate | Module | Test | Time delta |",
        "| --- | --- | --- | ---: |",
    ]
    for row in worst:
        lines.append(
            f"| {row['candidate']} | {row['module']} | {row['test']} | {row['time_delta_pct']}% |"
        )

    lines += [
        "",
        "## Largest RAM1 Code Savings",
        "",
        "| Candidate | Module | RAM1 code delta | RAM1 free delta |",
        "| --- | --- | ---: | ---: |",
    ]
    for row in best_size:
        lines.append(
            f"| {row['candidate']} | {row['module']} | {row['ram1_code_delta']} | {row['ram1_free_delta']} |"
        )
    lines.append("")
    (summary_dir / "README.md").write_text("\n".join(lines), encoding="utf-8")


def main(argv: Iterable[str]) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--out", default=str(DEFAULT_OUT_ROOT))
    parser.add_argument("--modules", default="all", help="Module list passed to run_bench3d.py.")
    parser.add_argument("--candidates", default="all", help="Candidate list or 'all'.")
    parser.add_argument("--python", default=sys.executable, help="Python executable used to run run_bench3d.py.")
    parser.add_argument("--dry-run", action="store_true")
    parser.add_argument("--no-compile", action="store_true")
    parser.add_argument("--stop-on-error", action="store_true")
    args = parser.parse_args(list(argv))

    out_root = Path(args.out)
    out_root.mkdir(parents=True, exist_ok=True)
    candidates = select_candidates(args.candidates)

    manifest = {
        "created": _dt.datetime.now().isoformat(timespec="seconds"),
        "root": str(ROOT),
        "git": git_info(),
        "board": "teensy41",
        "modules": args.modules,
        "candidates": [
            {"name": name, "flags": MATRIX[name]} for name in candidates
        ],
    }
    (out_root / "matrix_manifest.json").write_text(json.dumps(manifest, indent=2), encoding="utf-8")

    candidate_outputs: dict[str, Path] = {}
    candidate_status: list[dict[str, object]] = []

    for candidate in candidates:
        flags = MATRIX[candidate]
        candidate_dir = out_root / candidate
        log_path = candidate_dir / "run.log"
        env = os.environ.copy()
        if flags:
            env["TGX_BENCH_EXTRA_CPP_FLAGS"] = " ".join("-D" + flag for flag in flags)
        else:
            env.pop("TGX_BENCH_EXTRA_CPP_FLAGS", None)

        command = [
            args.python,
            str(RUN_BENCH3D),
            "--board",
            "teensy41",
            "--module",
            args.modules,
            "--out",
            str(candidate_dir),
        ]
        if args.no_compile:
            command.append("--no-compile")
        if args.stop_on_error:
            command.append("--stop-on-error")

        returncode = run_command(command, env, log_path, args.dry_run)
        output_dir = latest_output_dir(candidate_dir) if not args.dry_run else None
        status = "DRY_RUN" if args.dry_run else ("OK" if returncode == 0 and output_dir else "ERROR")

        if output_dir:
            candidate_outputs[candidate] = output_dir
        candidate_status.append({
            "candidate": candidate,
            "flags": " ".join(flags),
            "status": status,
            "returncode": returncode,
            "output_dir": str(output_dir or ""),
            "log": str(log_path),
        })

        if status == "ERROR" and args.stop_on_error:
            break

    if not args.dry_run:
        write_summary(out_root, candidate_outputs, candidate_status)
    else:
        write_csv(
            out_root / "summary" / "candidate_status.csv",
            candidate_status,
            ["candidate", "flags", "status", "returncode", "output_dir", "log"],
        )

    ok = all(row["status"] in ("OK", "DRY_RUN") for row in candidate_status)
    print(f"TGX_TEENSY4_FLASH_MATRIX_DONE,status={'OK' if ok else 'ERROR'},out={out_root}")
    return 0 if ok else 1


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
