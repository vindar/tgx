import argparse
import csv
import statistics
from pathlib import Path


def read_rows(path: Path, board: str, label: str):
    rows = {}
    with path.open(newline="") as f:
        for row in csv.DictReader(f):
            if row["board"] != board or row["label"] != label:
                continue
            key = (row["score_index"], row["renderer_size"], row["subtest"])
            rows[key] = float(row["fps"])
    return rows


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--matrix", default="tmp/config_optimization/subtest_matrix.csv")
    ap.add_argument("--board", required=True)
    ap.add_argument("--baseline", required=True)
    ap.add_argument("--variant", required=True)
    ap.add_argument("--out", required=True)
    args = ap.parse_args()

    base = read_rows(Path(args.matrix), args.board, args.baseline)
    var = read_rows(Path(args.matrix), args.board, args.variant)
    rows = []
    for key, b in base.items():
        if key not in var or b == 0:
            continue
        v = var[key]
        rows.append({
            "board": args.board,
            "baseline": args.baseline,
            "variant": args.variant,
            "score_index": key[0],
            "renderer_size": key[1],
            "subtest": key[2],
            "baseline_fps": f"{b:.2f}",
            "variant_fps": f"{v:.2f}",
            "pct_delta": f"{100.0 * (v - b) / b:.2f}",
        })
    rows.sort(key=lambda r: float(r["pct_delta"]))

    out = Path(args.out)
    out.parent.mkdir(parents=True, exist_ok=True)
    with out.open("w", newline="") as f:
        fields = [
            "board", "baseline", "variant", "score_index", "renderer_size",
            "subtest", "baseline_fps", "variant_fps", "pct_delta",
        ]
        writer = csv.DictWriter(f, fieldnames=fields)
        writer.writeheader()
        writer.writerows(rows)

    deltas = [float(r["pct_delta"]) for r in rows]
    if not deltas:
        print(f"wrote {out} (0 matched rows)")
        return
    print(f"wrote {out} ({len(rows)} rows)")
    print(f"mean={statistics.mean(deltas):.2f}% median={statistics.median(deltas):.2f}%")
    print(f"improved_gt_1={sum(d > 1.0 for d in deltas)} neutral_0.5={sum(-0.5 <= d <= 0.5 for d in deltas)} weak_regress={sum(-1.0 < d < -0.5 for d in deltas)} regress_le_1={sum(d <= -1.0 for d in deltas)}")
    print("worst:")
    for r in rows[:10]:
        print(f"{r['pct_delta']:>8}% {r['score_index']} {r['subtest']} {r['baseline_fps']} -> {r['variant_fps']}")
    print("best:")
    for r in rows[-10:]:
        print(f"{r['pct_delta']:>8}% {r['score_index']} {r['subtest']} {r['baseline_fps']} -> {r['variant_fps']}")


if __name__ == "__main__":
    main()
