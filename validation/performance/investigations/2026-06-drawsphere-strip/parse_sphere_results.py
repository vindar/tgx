import argparse
import csv
from pathlib import Path


def parse_result_line(line: str):
    line = line.strip()
    if not line.startswith("TGX_SPHERE_RESULT,"):
        return None
    row = {}
    for part in line.split(",")[1:]:
        if "=" not in part:
            continue
        key, value = part.split("=", 1)
        row[key] = value
    return row


def parse_file(path: Path, label: str):
    rows = []
    for line in path.read_text(errors="replace").splitlines():
        row = parse_result_line(line)
        if row is None:
            continue
        row["label"] = label
        row["source"] = str(path)
        rows.append(row)
    return rows


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--out", required=True)
    parser.add_argument("inputs", nargs="+", help="label=path telemetry inputs")
    args = parser.parse_args()

    rows = []
    for item in args.inputs:
        if "=" not in item:
            raise SystemExit(f"input must be label=path: {item}")
        label, raw_path = item.split("=", 1)
        rows.extend(parse_file(Path(raw_path), label))

    fields = [
        "label", "board", "case", "sectors", "stacks", "scale", "shading",
        "texture", "light", "specular", "frames", "avg_us", "fps", "hash",
        "source",
    ]
    out = Path(args.out)
    out.parent.mkdir(parents=True, exist_ok=True)
    with out.open("w", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=fields, extrasaction="ignore")
        writer.writeheader()
        writer.writerows(rows)
    print(f"wrote {out} ({len(rows)} rows)")


if __name__ == "__main__":
    main()
