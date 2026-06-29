import csv
import re
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
INVESTIGATION = Path(__file__).resolve().parent
TELEMETRY = ROOT / "telemetry"
OUT_DIR = INVESTIGATION / "hardware"


LINE_RE = re.compile(
    r"\[TGX telemetry\]\s+cycle=(?P<cycle>\d+)\s+scene=(?P<scene>\S+)\s+"
    r"fps=(?P<fps>[0-9.]+)\s+frame_avg_us=(?P<avg>[0-9.]+)\s+"
    r"frame_min_us=(?P<min>\d+)\s+frame_max_us=(?P<max>\d+)\s+frames=(?P<frames>\d+)"
)


RUNS = {
    "opt_gouraud_cached": {
        "teensy41": "teensy41_cone_cylinder_opt_cached_teensy41.txt",
        "core2": "core2_cone_cylinder_opt_cached_core2.txt",
        "cores3": "cores3_cone_cylinder_opt_cached_cores3.txt",
        "pico2": "pico2_cone_cylinder_opt_cached_pico2.txt",
    },
    "opt_gouraud_cached_cone": {
        "teensy41": "teensy41_cone_cylinder_opt_cached_cone_teensy41.txt",
        "core2": "core2_cone_cylinder_opt_cached_cone_core2.txt",
        "cores3": "cores3_cone_cylinder_opt_cached_cone_cores3.txt",
        "pico2": "pico2_cone_cylinder_opt_cached_cone_pico2.txt",
    },
    "opt_gouraud_cached_shared": {
        "teensy41": "teensy41_cone_cylinder_opt_cached_shared_teensy41.txt",
        "core2": "core2_cone_cylinder_opt_cached_shared_core2.txt",
        "cores3": "cores3_cone_cylinder_opt_cached_shared_cores3.txt",
        "pico2": "pico2_cone_cylinder_opt_cached_shared_pico2.txt",
    },
    "opt_gouraud_cached_shared_slots": {
        "teensy41": "teensy41_cone_cylinder_opt_cached_shared_slots_teensy41.txt",
        "core2": "core2_cone_cylinder_opt_cached_shared_slots_core2.txt",
        "cores3": "cores3_cone_cylinder_opt_cached_shared_slots_cores3.txt",
        "pico2": "pico2_cone_cylinder_opt_cached_shared_slots_pico2.txt",
    },
}


def parse_file(path):
    rows = []
    for line in path.read_text(errors="replace").splitlines():
        m = LINE_RE.search(line)
        if not m:
            continue
        rows.append(
            {
                "cycle": int(m.group("cycle")),
                "scene": m.group("scene"),
                "fps": float(m.group("fps")),
                "frame_avg_us": float(m.group("avg")),
                "frame_min_us": int(m.group("min")),
                "frame_max_us": int(m.group("max")),
                "frames": int(m.group("frames")),
            }
        )
    return rows


def summarize(rows):
    by_scene = {}
    for row in rows:
        by_scene.setdefault(row["scene"], []).append(row)

    out = []
    for scene, vals in sorted(by_scene.items()):
        vals = sorted(vals, key=lambda r: r["cycle"])
        stable = vals[1:] if len(vals) > 1 else vals
        n = len(stable)
        fps_avg = sum(r["fps"] for r in stable) / n
        us_avg = sum(r["frame_avg_us"] for r in stable) / n
        out.append(
            {
                "scene": scene,
                "samples": n,
                "fps_avg": fps_avg,
                "frame_avg_us": us_avg,
                "fps_min": min(r["fps"] for r in stable),
                "fps_max": max(r["fps"] for r in stable),
            }
        )
    return out


def load_summary(path, run_name, summaries):
    if not path.exists():
        return
    with path.open(newline="") as f:
        reader = csv.DictReader(f)
        for row in reader:
            item = {
                "board": row["board"],
                "scene": row["scene"],
                "samples": int(row["samples"]),
                "fps_avg": float(row["fps_avg"]),
                "frame_avg_us": float(row["frame_avg_us"]),
                "fps_min": float(row["fps_min"]),
                "fps_max": float(row["fps_max"]),
            }
            summaries[(run_name, item["board"], item["scene"])] = item


def write_summary(path, rows):
    fieldnames = ["board", "scene", "samples", "fps_avg", "frame_avg_us", "fps_min", "fps_max"]
    with path.open("w", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        writer.writeheader()
        for row in rows:
            writer.writerow(
                {
                    "board": row["board"],
                    "scene": row["scene"],
                    "samples": row["samples"],
                    "fps_avg": f"{row['fps_avg']:.3f}",
                    "frame_avg_us": f"{row['frame_avg_us']:.2f}",
                    "fps_min": f"{row['fps_min']:.3f}",
                    "fps_max": f"{row['fps_max']:.3f}",
                }
            )


def write_comparison(path, summaries, candidate_run):
    with path.open("w", newline="") as f:
        writer = csv.DictWriter(
            f,
            fieldnames=[
                "board",
                "scene",
                "initial_fps",
                "candidate_fps",
                "delta_fps",
                "delta_pct",
                "initial_us",
                "candidate_us",
            ],
        )
        writer.writeheader()
        for key, initial in sorted(summaries.items()):
            run_name, board, scene = key
            if run_name != "initial":
                continue
            candidate = summaries.get((candidate_run, board, scene))
            if candidate is None:
                continue
            delta = candidate["fps_avg"] - initial["fps_avg"]
            delta_pct = 100.0 * delta / initial["fps_avg"]
            writer.writerow(
                {
                    "board": board,
                    "scene": scene,
                    "initial_fps": f"{initial['fps_avg']:.3f}",
                    "candidate_fps": f"{candidate['fps_avg']:.3f}",
                    "delta_fps": f"{delta:.3f}",
                    "delta_pct": f"{delta_pct:.2f}",
                    "initial_us": f"{initial['frame_avg_us']:.2f}",
                    "candidate_us": f"{candidate['frame_avg_us']:.2f}",
                }
            )


def main():
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    summaries = {}
    load_summary(OUT_DIR / "initial_hardware_summary.csv", "initial", summaries)

    for run_name, inputs in RUNS.items():
        all_rows = []
        for board, filename in inputs.items():
            rows = parse_file(TELEMETRY / filename)
            summary = summarize(rows)
            for row in summary:
                row["board"] = board
                all_rows.append(row)
                summaries[(run_name, board, row["scene"])] = row

        out_path = OUT_DIR / f"{run_name}_hardware_summary.csv"
        write_summary(out_path, all_rows)
        print(out_path)

    for candidate_run in RUNS:
        cmp_path = OUT_DIR / f"{candidate_run}_vs_initial.csv"
        write_comparison(cmp_path, summaries, candidate_run)
        print(cmp_path)


if __name__ == "__main__":
    main()
