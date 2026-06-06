import csv
import re
from pathlib import Path


OUT = Path("tmp/config_optimization/macro_matrix.csv")
LOGDIR = Path("tmp/config_optimization/macro_probe_logs")


def parse_log(path: Path):
    stem = path.name[:-len("_serial.txt")]
    board, label = stem.split("_", 1) if "_" in stem else (stem, "")
    row = {"board": board, "label": label, "serial_log": str(path)}
    for line in path.read_text(errors="replace").splitlines():
        if "=" not in line:
            continue
        key, value = line.split("=", 1)
        if re.match(r"^[A-Za-z0-9_]+$", key):
            row[key] = value
    return row


def main():
    rows = []
    keys = set()
    for path in sorted(LOGDIR.glob("*_serial.txt")):
        row = parse_log(path)
        rows.append(row)
        keys.update(row.keys())

    fields = ["board", "label"]
    preferred = [
        "TGX_INLINE",
        "TGX_NOINLINE",
        "TGX_INLINE_ZDIVIDE",
        "TGX_SHADER_USE_INCREMENTAL_PIXEL_POINTERS",
        "TGX_USE_FAST_INV_SQRT_TRICK",
        "TGX_USE_FAST_SQRT_TRICK",
        "TGX_USE_FAST_INV_TRICK",
        "TGX_USE_FMA_MATH",
        "ARDUINO_TEENSY41",
        "ARDUINO_ARCH_RP2040",
        "ARDUINO_ARCH_RP2350",
        "PICO_RP2040",
        "PICO_RP2350",
        "ESP32",
        "ESP32S3",
        "CONFIG_IDF_TARGET_ESP32",
        "CONFIG_IDF_TARGET_ESP32S3",
        "__ARM_ARCH_8M_MAIN__",
    ]
    fields.extend([k for k in preferred if k in keys])
    fields.extend(sorted(k for k in keys if k not in set(fields) and k != "serial_log"))
    fields.append("serial_log")

    OUT.parent.mkdir(parents=True, exist_ok=True)
    with OUT.open("w", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=fields)
        writer.writeheader()
        for row in rows:
            writer.writerow(row)
    print(f"wrote {OUT} ({len(rows)} rows)")


if __name__ == "__main__":
    main()
