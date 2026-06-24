import csv
import json
from collections import defaultdict
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
PARSED = ROOT / "parsed"
OUTDIR = Path(__file__).resolve().parent

PREFIX = "pico2rp2350cfg"
EXAMPLES = ("bunny_fig", "borg_cube", "scream")


def read_json(path):
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except Exception:
        return {}


def read_rows(path):
    if not path.exists():
        return []
    with path.open(newline="", encoding="utf-8-sig") as f:
        return list(csv.DictReader(f))


def safe_float(value):
    if value in (None, ""):
        return None
    try:
        return float(value)
    except ValueError:
        return None


def safe_int(value):
    if value in (None, ""):
        return None
    try:
        return int(float(value))
    except ValueError:
        return None


def parse_size_from_log(log_path):
    if not log_path:
        return None, None
    path = Path(log_path)
    if not path.exists():
        return None, None
    sketch = None
    dynamic = None
    for line in path.read_text(errors="replace").splitlines():
        if "Le croquis utilise" in line and "octets" in line:
            parts = line.split()
            for i, part in enumerate(parts):
                if part == "utilise" and i + 1 < len(parts):
                    sketch = safe_int(parts[i + 1])
                    break
        if "Les variables globales utilisent" in line and "octets" in line:
            parts = line.split()
            for i, part in enumerate(parts):
                if part == "utilisent" and i + 1 < len(parts):
                    dynamic = safe_int(parts[i + 1])
                    break
    return sketch, dynamic


def label_parts(label):
    if not label.startswith(PREFIX + "_"):
        return None
    rest = label[len(PREFIX) + 1 :]
    for example in EXAMPLES:
        suffix = "_" + example
        if rest.endswith(suffix):
            return rest[: -len(suffix)], example
    if rest == "macro":
        return rest, "macro"
    return None


def summarize():
    summary = []
    metadata_paths = sorted(PARSED.glob(f"pico2_{PREFIX}_*.json"))
    for meta_path in metadata_paths:
        meta = read_json(meta_path)
        label = meta.get("board", "") + "_" + meta_path.stem[len("pico2_") :]
        raw_label = meta_path.stem[len("pico2_") :]
        parts = label_parts(raw_label)
        if parts is None:
            continue
        candidate, example = parts
        if example == "macro":
            continue

        csv_path = meta_path.with_name(meta_path.stem + "_example.csv")
        rows = read_rows(csv_path)
        grouped = defaultdict(list)
        for row in rows:
            grouped[row.get("scene") or "unknown"].append(row)

        sketch_size, dyn_size = parse_size_from_log(meta.get("log_file", ""))
        if not grouped:
            summary.append({
                "candidate": candidate,
                "example": example,
                "scene": "",
                "status": meta.get("run_status", ""),
                "samples": 0,
                "frame_avg_us_weighted": "",
                "fps_avg": "",
                "sketch_bytes": sketch_size if sketch_size is not None else "",
                "dynamic_bytes": dyn_size if dyn_size is not None else "",
                "fqbn": meta.get("fqbn", ""),
                "metadata_file": str(meta_path),
            })
            continue

        for scene, scene_rows in sorted(grouped.items()):
            fps_values = [safe_float(row.get("fps")) for row in scene_rows]
            fps_values = [v for v in fps_values if v is not None]
            weighted_num = 0.0
            weighted_den = 0
            unweighted_frames = []
            for row in scene_rows:
                frame = safe_float(row.get("frame_avg_us"))
                frames = safe_int(row.get("frames"))
                if frame is not None and frames:
                    weighted_num += frame * frames
                    weighted_den += frames
                elif frame is not None:
                    unweighted_frames.append(frame)
            if weighted_den:
                frame_avg = weighted_num / weighted_den
            elif unweighted_frames:
                frame_avg = sum(unweighted_frames) / len(unweighted_frames)
            else:
                frame_avg = None

            summary.append({
                "candidate": candidate,
                "example": example,
                "scene": scene,
                "status": meta.get("run_status", ""),
                "samples": len(scene_rows),
                "frame_avg_us_weighted": f"{frame_avg:.6f}" if frame_avg is not None else "",
                "fps_avg": f"{(sum(fps_values) / len(fps_values)):.6f}" if fps_values else "",
                "sketch_bytes": sketch_size if sketch_size is not None else "",
                "dynamic_bytes": dyn_size if dyn_size is not None else "",
                "fqbn": meta.get("fqbn", ""),
                "metadata_file": str(meta_path),
            })
    return summary


def compute_deltas(summary):
    by_key = {(r["candidate"], r["example"], r["scene"]): r for r in summary}
    base = {
        (r["example"], r["scene"]): r
        for r in summary
        if r["candidate"] == "base_fast"
    }
    deltas = []
    for row in summary:
        candidate = row["candidate"]
        if candidate == "base_fast":
            continue
        b = base.get((row["example"], row["scene"]))
        if not b:
            continue
        frame_base = safe_float(b["frame_avg_us_weighted"])
        frame_candidate = safe_float(row["frame_avg_us_weighted"])
        fps_base = safe_float(b["fps_avg"])
        fps_candidate = safe_float(row["fps_avg"])
        frame_delta = ""
        frame_delta_pct = ""
        fps_delta_pct = ""
        metric = ""
        if frame_base and frame_candidate:
            frame_delta = frame_candidate - frame_base
            frame_delta_pct = 100.0 * frame_delta / frame_base
            metric = "frame_us_negative_is_faster"
        elif fps_base and fps_candidate:
            fps_delta_pct = 100.0 * (fps_candidate - fps_base) / fps_base
            metric = "fps_positive_is_faster"

        sketch_delta = ""
        if row["sketch_bytes"] != "" and b["sketch_bytes"] != "":
            sketch_delta = int(row["sketch_bytes"]) - int(b["sketch_bytes"])

        deltas.append({
            "candidate": candidate,
            "example": row["example"],
            "scene": row["scene"],
            "metric": metric,
            "base_frame_avg_us": b["frame_avg_us_weighted"],
            "candidate_frame_avg_us": row["frame_avg_us_weighted"],
            "frame_delta_us": f"{frame_delta:.6f}" if frame_delta != "" else "",
            "frame_delta_pct_negative_is_faster": f"{frame_delta_pct:.6f}" if frame_delta_pct != "" else "",
            "base_fps": b["fps_avg"],
            "candidate_fps": row["fps_avg"],
            "fps_delta_pct_positive_is_faster": f"{fps_delta_pct:.6f}" if fps_delta_pct != "" else "",
            "base_sketch_bytes": b["sketch_bytes"],
            "candidate_sketch_bytes": row["sketch_bytes"],
            "sketch_delta_bytes": sketch_delta,
            "status": row["status"],
            "fqbn": row["fqbn"],
        })
    return deltas


def write_csv(path, rows):
    fields = list(rows[0].keys()) if rows else []
    with path.open("w", newline="", encoding="utf-8") as f:
        if fields:
            writer = csv.DictWriter(f, fieldnames=fields)
            writer.writeheader()
            writer.writerows(rows)


def main():
    summary = summarize()
    deltas = compute_deltas(summary)
    OUTDIR.mkdir(parents=True, exist_ok=True)
    write_csv(OUTDIR / "summary_pico2_config.csv", summary)
    write_csv(OUTDIR / "delta_pico2_config_vs_base_fast.csv", deltas)
    print(f"summary rows={len(summary)}")
    print(f"delta rows={len(deltas)}")


if __name__ == "__main__":
    main()
