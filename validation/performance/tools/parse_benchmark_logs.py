import argparse
import csv
import re
from pathlib import Path


SCORE_RE = re.compile(r"\*\*\* BENCHMARK FINAL SCORE:\s*([0-9.]+)")
SUBTEST_RE = re.compile(
    r"^(.*?)\s+\[.*?\].*?\bframes:\s*([0-9]+)\s+"
    r"min:\s*([0-9.]+)\s+us\s+max:\s*([0-9.]+)\s+us\s+"
    r"avg:\s*([0-9.]+)\s+us\s+stddev:\s*([0-9.]+)\s+us\s+fps:\s*([0-9.]+)"
)
RENDERER_RE = re.compile(r"RENDERER SIZE:\s*(.*)")


def parse_serial(path: Path):
    board_label = path.name[:-len("_serial.txt")]
    if "_" in board_label:
        board, label = board_label.split("_", 1)
    else:
        board, label = board_label, ""

    renderer = ""
    score_index = 0
    scores = []
    subtests = []
    for line in path.read_text(errors="replace").splitlines():
        m = RENDERER_RE.search(line)
        if m:
            renderer = " ".join(m.group(1).split())
            continue

        m = SUBTEST_RE.search(line)
        if m:
            name = " ".join(m.group(1).split())
            if name != "total:":
                subtests.append({
                    "board": board,
                    "label": label,
                    "score_index": score_index + 1,
                    "renderer_size": renderer,
                    "subtest": name,
                    "frames": m.group(2),
                    "min_us": m.group(3),
                    "max_us": m.group(4),
                    "avg_us": m.group(5),
                    "stddev_us": m.group(6),
                    "fps": m.group(7),
                    "serial_log": str(path),
                })
            continue

        m = SCORE_RE.search(line)
        if m:
            score_index += 1
            scores.append({
                "board": board,
                "label": label,
                "score_index": score_index,
                "score": m.group(1),
                "serial_log": str(path),
            })

    return scores, subtests


def parse_compile(path: Path):
    board_label = path.name[:-len("_compile.txt")]
    if "_" in board_label:
        board, label = board_label.split("_", 1)
    else:
        board, label = board_label, ""

    rows = []
    for line in path.read_text(errors="replace").splitlines():
        stripped = line.strip()
        if any(token in stripped for token in (
            "Le croquis utilise",
            "Les variables globales utilisent",
            "Sketch uses",
            "Global variables use",
            "Memory Usage",
            "FLASH:",
            "RAM1:",
            "RAM2:",
        )):
            rows.append({
                "board": board,
                "label": label,
                "line": stripped,
                "compile_log": str(path),
            })
    return rows


def write_csv(path: Path, rows, fields):
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=fields)
        writer.writeheader()
        writer.writerows(rows)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--logdir", default="tmp/config_optimization/logs")
    ap.add_argument("--out-global", default="tmp/config_optimization/global_scores.csv")
    ap.add_argument("--out-subtests", default="tmp/config_optimization/subtest_matrix.csv")
    ap.add_argument("--out-size", default="tmp/config_optimization/binary_size_lines.csv")
    args = ap.parse_args()

    logdir = Path(args.logdir)
    scores = []
    subtests = []
    sizes = []

    for serial in sorted(logdir.glob("*_serial.txt")):
        s, t = parse_serial(serial)
        scores.extend(s)
        subtests.extend(t)

    for compile_log in sorted(logdir.glob("*_compile.txt")):
        sizes.extend(parse_compile(compile_log))

    write_csv(Path(args.out_global), scores, ["board", "label", "score_index", "score", "serial_log"])
    write_csv(Path(args.out_subtests), subtests, [
        "board", "label", "score_index", "renderer_size", "subtest",
        "frames", "min_us", "max_us", "avg_us", "stddev_us", "fps", "serial_log",
    ])
    write_csv(Path(args.out_size), sizes, ["board", "label", "line", "compile_log"])

    print(f"wrote {args.out_global} ({len(scores)} rows)")
    print(f"wrote {args.out_subtests} ({len(subtests)} rows)")
    print(f"wrote {args.out_size} ({len(sizes)} rows)")


if __name__ == "__main__":
    main()
