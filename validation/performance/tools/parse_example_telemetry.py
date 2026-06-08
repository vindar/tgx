import csv
import re
from pathlib import Path


LOGDIR = Path("tmp/config_optimization/example_logs")
OUT = Path("tmp/config_optimization/example_telemetry.csv")


def parse_name(path: Path):
    stem = path.name[:-len("_serial.txt")]
    parts = stem.split("_")
    board = parts[0]
    label_start = None
    for i, p in enumerate(parts[1:], start=1):
        if p in ("baseline", "candidate", "variant", "final"):
            label_start = i
            break
    if label_start is None:
        return board, "_".join(parts[1:]), ""
    return board, "_".join(parts[1:label_start]), "_".join(parts[label_start:])


def summarize(path: Path):
    board, example, label = parse_name(path)
    fps = []
    scenes = []
    displays = set()
    for line in path.read_text(errors="replace").splitlines():
        m = re.search(r"\bscene=([A-Za-z0-9_.-]+)", line)
        if m:
            scenes.append(m.group(1))
        m = re.search(r"\bdisplay=([A-Za-z0-9_.-]+)", line)
        if m:
            displays.add(m.group(1))
        for pattern in (r"\bfps=([0-9]+(?:\.[0-9]+)?)", r"\bfps:\s*([0-9]+(?:\.[0-9]+)?)"):
            m = re.search(pattern, line)
            if m:
                fps.append(float(m.group(1)))
                break
    return {
        "board": board,
        "example": example,
        "label": label,
        "samples": len(fps),
        "avg_fps": f"{sum(fps) / len(fps):.2f}" if fps else "",
        "min_fps": f"{min(fps):.2f}" if fps else "",
        "max_fps": f"{max(fps):.2f}" if fps else "",
        "scenes": "|".join(dict.fromkeys(scenes)),
        "displays": "|".join(sorted(displays)),
        "serial_log": str(path),
        "notes": "" if fps else "no fps captured",
    }


def main():
    rows = [summarize(path) for path in sorted(LOGDIR.glob("*_serial.txt"))]
    OUT.parent.mkdir(parents=True, exist_ok=True)
    with OUT.open("w", newline="") as f:
        fields = [
            "board", "example", "label", "samples", "avg_fps", "min_fps",
            "max_fps", "scenes", "displays", "serial_log", "notes",
        ]
        writer = csv.DictWriter(f, fieldnames=fields)
        writer.writeheader()
        writer.writerows(rows)
    print(f"wrote {OUT} ({len(rows)} rows)")


if __name__ == "__main__":
    main()
