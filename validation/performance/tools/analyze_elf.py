import argparse
import csv
import re
import subprocess
from pathlib import Path


HOT_PATTERNS = [
    "_drawMesh",
    "_drawTriangle",
    "_drawTriangleClipped",
    "_drawTriangleClippedSub",
    "_drawQuad",
    "rasterizeTriangle",
    "shader_select",
    "uber_shader",
    "zdivide",
    "normalize_fast",
    "RGBf",
    "Vec2",
    "Vec3",
    "Vec4",
]


TOOLCHAINS = {
    "pico2": {
        "nm": r"C:\Users\Vindar\AppData\Local\Arduino15\packages\rp2040\tools\pqt-gcc\4.1.0-1aec55e\bin\arm-none-eabi-nm.exe",
        "objdump": r"C:\Users\Vindar\AppData\Local\Arduino15\packages\rp2040\tools\pqt-gcc\4.1.0-1aec55e\bin\arm-none-eabi-objdump.exe",
        "size": r"C:\Users\Vindar\AppData\Local\Arduino15\packages\rp2040\tools\pqt-gcc\4.1.0-1aec55e\bin\arm-none-eabi-size.exe",
    },
    "picow": {
        "nm": r"C:\Users\Vindar\AppData\Local\Arduino15\packages\rp2040\tools\pqt-gcc\4.1.0-1aec55e\bin\arm-none-eabi-nm.exe",
        "objdump": r"C:\Users\Vindar\AppData\Local\Arduino15\packages\rp2040\tools\pqt-gcc\4.1.0-1aec55e\bin\arm-none-eabi-objdump.exe",
        "size": r"C:\Users\Vindar\AppData\Local\Arduino15\packages\rp2040\tools\pqt-gcc\4.1.0-1aec55e\bin\arm-none-eabi-size.exe",
    },
    "teensy41": {
        "nm": r"C:\Users\Vindar\AppData\Local\Arduino15\packages\teensy\tools\teensy-compile\11.3.1\arm\bin\arm-none-eabi-nm.exe",
        "objdump": r"C:\Users\Vindar\AppData\Local\Arduino15\packages\teensy\tools\teensy-compile\11.3.1\arm\bin\arm-none-eabi-objdump.exe",
        "size": r"C:\Users\Vindar\AppData\Local\Arduino15\packages\teensy\tools\teensy-compile\11.3.1\arm\bin\arm-none-eabi-size.exe",
    },
    "core2": {
        "nm": r"C:\Users\Vindar\AppData\Local\Arduino15\packages\esp32\tools\esp-x32\2601\bin\xtensa-esp32-elf-nm.exe",
        "objdump": r"C:\Users\Vindar\AppData\Local\Arduino15\packages\esp32\tools\esp-x32\2601\bin\xtensa-esp32-elf-objdump.exe",
        "size": r"C:\Users\Vindar\AppData\Local\Arduino15\packages\esp32\tools\esp-x32\2601\bin\xtensa-esp32-elf-size.exe",
    },
    "cores3": {
        "nm": r"C:\Users\Vindar\AppData\Local\Arduino15\packages\esp32\tools\esp-x32\2601\bin\xtensa-esp32-elf-nm.exe",
        "objdump": r"C:\Users\Vindar\AppData\Local\Arduino15\packages\esp32\tools\esp-x32\2601\bin\xtensa-esp32-elf-objdump.exe",
        "size": r"C:\Users\Vindar\AppData\Local\Arduino15\packages\esp32\tools\esp-x32\2601\bin\xtensa-esp32-elf-size.exe",
    },
}


def run(args):
    return subprocess.check_output(args, text=True, errors="replace")


def hot_key(name):
    for p in HOT_PATTERNS:
        if p in name:
            return p
    return None


def parse_nm(nm, elf):
    out = run([nm, "-S", "--size-sort", "-C", str(elf)])
    rows = []
    for line in out.splitlines():
        parts = line.split(maxsplit=3)
        if len(parts) < 4:
            continue
        addr, size, typ, name = parts
        if typ.lower() not in ("t", "w"):
            continue
        hk = hot_key(name)
        if not hk:
            continue
        rows.append({
            "hot_key": hk,
            "name": name,
            "addr": int(addr, 16),
            "size": int(size, 16),
            "align16": int(addr, 16) % 16,
            "align32": int(addr, 16) % 32,
        })
    return rows


def split_disasm(objdump, elf):
    text = run([objdump, "-d", "-C", str(elf)])
    funcs = {}
    current = None
    current_lines = []
    header_re = re.compile(r"^([0-9a-fA-F]+) <(.+)>:$")
    for line in text.splitlines():
        m = header_re.match(line)
        if m:
            if current is not None:
                funcs[current] = current_lines
            current = m.group(2)
            current_lines = [line]
        elif current is not None:
            current_lines.append(line)
    if current is not None:
        funcs[current] = current_lines
    return funcs


def instruction_stats(lines):
    stats = {
        "insn": 0,
        "calls": 0,
        "branches": 0,
        "loads": 0,
        "stores": 0,
        "fp": 0,
        "div": 0,
        "conversions": 0,
    }
    for line in lines:
        m = re.search(r":\s+([0-9a-fA-F]{2,}(?:\s+[0-9a-fA-F]{2,})*)\s+([a-zA-Z0-9_.]+)", line)
        if not m:
            continue
        op = m.group(2).lower()
        stats["insn"] += 1
        if op.startswith(("bl", "call")) or op in ("jal", "jarl"):
            stats["calls"] += 1
        if op.startswith(("b", "j")) and not op.startswith("bic"):
            stats["branches"] += 1
        if op.startswith(("ldr", "ldm", "vldr", "l32", "l16", "l8", "entry")):
            stats["loads"] += 1
        if op.startswith(("str", "stm", "vstr", "s32", "s16", "s8")):
            stats["stores"] += 1
        if op.startswith(("v", "f")) or op.endswith(".s"):
            stats["fp"] += 1
        if "div" in op:
            stats["div"] += 1
        if op.startswith(("vcvt", "cvt")) or "cvt" in op:
            stats["conversions"] += 1
    return stats


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--board", required=True, choices=sorted(TOOLCHAINS))
    ap.add_argument("--elf", required=True)
    ap.add_argument("--label", required=True)
    ap.add_argument("--outdir", default="tmp/config_optimization/codegen")
    args = ap.parse_args()

    tools = TOOLCHAINS[args.board]
    elf = Path(args.elf)
    outdir = Path(args.outdir)
    outdir.mkdir(parents=True, exist_ok=True)
    disasm_dir = Path("tmp/config_optimization/codegen/disasm") / args.label
    disasm_dir.mkdir(parents=True, exist_ok=True)

    symbols = parse_nm(tools["nm"], elf)
    funcs = split_disasm(tools["objdump"], elf)
    size_text = run([tools["size"], str(elf)])
    (outdir / f"{args.label}_size.txt").write_text(size_text)

    sym_path = outdir / f"{args.label}_hot_symbols.csv"
    with sym_path.open("w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=["hot_key", "name", "addr", "size", "align16", "align32"])
        w.writeheader()
        w.writerows(symbols)

    stats_rows = []
    for s in symbols:
        lines = funcs.get(s["name"])
        if not lines:
            continue
        stats = instruction_stats(lines)
        row = dict(s)
        row.update(stats)
        stats_rows.append(row)
        safe = re.sub(r"[^A-Za-z0-9_.-]+", "_", s["name"])[:160]
        (disasm_dir / f"{s['hot_key']}_{safe}.txt").write_text("\n".join(lines))

    stats_path = outdir / f"{args.label}_hot_stats.csv"
    with stats_path.open("w", newline="") as f:
        fields = ["hot_key", "name", "addr", "size", "align16", "align32",
                  "insn", "calls", "branches", "loads", "stores", "fp", "div", "conversions"]
        w = csv.DictWriter(f, fieldnames=fields)
        w.writeheader()
        w.writerows(stats_rows)

    print(f"symbols: {sym_path}")
    print(f"stats:   {stats_path}")
    print(f"size:    {outdir / (args.label + '_size.txt')}")
    print(f"disasm:  {disasm_dir}")


if __name__ == "__main__":
    main()
