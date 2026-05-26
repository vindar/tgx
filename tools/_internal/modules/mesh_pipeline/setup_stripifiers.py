from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile


TOOLS_ROOT = Path(__file__).resolve().parents[3]
INTERNAL_ROOT = TOOLS_ROOT / "_internal"
EXTERNAL = TOOLS_ROOT / "external_lib"
GA_EAX_SRC = EXTERNAL / "GA_EAX"
LKH_SRC = EXTERNAL / "LKH"
BIN_DIR = INTERNAL_ROOT / "bin"
BUILD_DIR = INTERNAL_ROOT / "build" / "stripifiers"


GA_EAX_SOURCES = [
    "main.cpp",
    "env.cpp",
    "cross.cpp",
    "evaluator.cpp",
    "indi.cpp",
    "rand.cpp",
    "kopt.cpp",
    "sort.cpp",
]


LKH_TGX_DISTANCE_SPECIAL = r'''#include "LKH.h"
#include <stdio.h>
#include <stdlib.h>

static unsigned char *TGXAdjBits = 0;
static int TGXAdjDimension = 0;

static size_t TGXBitIndex(int A, int B)
{
    return (size_t) A * (size_t) TGXAdjDimension + (size_t) B;
}

static void TGXSetAdjacency(int A, int B)
{
    size_t Bit = TGXBitIndex(A, B);
    TGXAdjBits[Bit >> 3] |= (unsigned char) (1u << (Bit & 7));
}

static int TGXGetAdjacency(int A, int B)
{
    size_t Bit = TGXBitIndex(A, B);
    return (TGXAdjBits[Bit >> 3] >> (Bit & 7)) & 1u;
}

static void LoadTGXAdjacency(void)
{
    const char *Name = getenv("TGX_LKH_GRAPH");
    FILE *F;
    int N, i, Degree, j, To;

    if (TGXAdjBits)
        return;
    if (!Name)
        eprintf("TGX_LKH_GRAPH environment variable is not set");
    if (!(F = fopen(Name, "r")))
        eprintf("Cannot open TGX_LKH_GRAPH: \"%s\"", Name);
    if (fscanf(F, "%d", &N) != 1 || N <= 0)
        eprintf("Invalid TGX graph header");
    TGXAdjDimension = N;
    TGXAdjBits = (unsigned char *) calloc(((size_t) N * (size_t) N + 7u) >> 3, 1);
    if (!TGXAdjBits)
        eprintf("Cannot allocate TGX adjacency bitset");
    for (i = 0; i < N; ++i) {
        if (fscanf(F, "%d", &Degree) != 1 || Degree < 0)
            eprintf("Invalid TGX adjacency row");
        for (j = 0; j < Degree; ++j) {
            if (fscanf(F, "%d", &To) != 1 || To < 0 || To >= N)
                eprintf("Invalid TGX adjacency index");
            TGXSetAdjacency(i, To);
        }
    }
    fclose(F);
}

int Distance_SPECIAL(Node * Na, Node * Nb)
{
    int A = Na->Id - 1;
    int B = Nb->Id - 1;
    LoadTGXAdjacency();
    if (A < 0 || B < 0 || A >= TGXAdjDimension || B >= TGXAdjDimension)
        return 1;
    if (A == B)
        return 0;
    return TGXGetAdjacency(A, B) ? 0 : 1;
}
'''


LKH_TGX_GETTIME = r'''#include <time.h>

double GetTime()
{
    return (double) clock() / CLOCKS_PER_SEC;
}
'''


def run(cmd: list[str], *, cwd: Path | None = None) -> None:
    print("+", " ".join(str(c) for c in cmd), flush=True)
    subprocess.run(cmd, cwd=cwd, check=True)


def run_msvc(command: str) -> None:
    print("+", command, flush=True)
    with tempfile.NamedTemporaryFile("w", suffix=".bat", delete=False, encoding="utf-8", newline="\r\n") as f:
        bat = Path(f.name)
        f.write("@echo off\n")
        f.write(command)
        f.write("\n")
    try:
        subprocess.run(["cmd", "/c", str(bat)], check=True)
    finally:
        try:
            bat.unlink()
        except OSError:
            pass


def find_msvc_command() -> str | None:
    candidates = [
        Path(r"C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat"),
        Path(r"C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat"),
        Path(r"C:\Program Files\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat"),
    ]
    for p in candidates:
        if p.exists():
            return str(p)
    return None


def compile_ga_eax() -> Path:
    BIN_DIR.mkdir(parents=True, exist_ok=True)
    ga_build = BUILD_DIR / "ga_eax"
    ga_build.mkdir(parents=True, exist_ok=True)
    exe = BIN_DIR / ("tgx_ga_eax_stripifier.exe" if os.name == "nt" else "tgx_ga_eax_stripifier")
    missing = [name for name in GA_EAX_SOURCES if not (GA_EAX_SRC / name).exists()]
    if missing:
        raise RuntimeError(f"GA-EAX source files missing: {', '.join(missing)}")
    if os.name == "nt":
        vc = find_msvc_command()
        if not vc:
            raise RuntimeError("MSVC not found. Install Visual Studio Build Tools or use a compiler in PATH.")
        srcs = " ".join(f'"{GA_EAX_SRC / source}"' for source in GA_EAX_SOURCES)
        cmd = f'call "{vc}" -arch=x64 >nul && cd /d "{ga_build}" && cl /nologo /O2 /EHsc {srcs} /Fe:"{exe}"'
        run_msvc(cmd)
    else:
        run(["c++", "-O3", "-std=c++11", "-o", str(exe), *[str(GA_EAX_SRC / s) for s in GA_EAX_SOURCES]])
    return exe


def find_lkh_source(root: Path) -> Path | None:
    candidates = []
    if (root / "SRC").exists():
        candidates.append(root)
    candidates.extend(p for p in root.iterdir() if p.is_dir() and (p / "SRC").exists())
    for candidate in candidates:
        has_header = (candidate / "SRC" / "LKH.h").exists() or (candidate / "SRC" / "INCLUDE" / "LKH.h").exists()
        if has_header and not (candidate / "SRC" / "Penalty_CVRP.c").exists():
            return candidate
    return None


def patch_lkh_copy(lkh_src: Path) -> Path:
    build = BUILD_DIR / "lkh2_tgx"
    if build.exists():
        shutil.rmtree(build)
    shutil.copytree(lkh_src, build, ignore=shutil.ignore_patterns("*.exe", "*.obj", "*.o"))
    (build / "SRC" / "Distance_SPECIAL.c").write_text(LKH_TGX_DISTANCE_SPECIAL.replace("\r\n", "\n"), encoding="utf-8")
    if os.name == "nt":
        (build / "SRC" / "GetTime.c").write_text(LKH_TGX_GETTIME.replace("\r\n", "\n"), encoding="utf-8")
    return build


def compile_lkh() -> Path | None:
    lkh_src = find_lkh_source(LKH_SRC)
    if not lkh_src:
        print("No LKH 2.x source tree found in tools/external_lib/LKH. LKH support is optional.")
        return None
    patched = patch_lkh_copy(lkh_src)
    BIN_DIR.mkdir(parents=True, exist_ok=True)
    exe = BIN_DIR / ("tgx_lkh_stripifier.exe" if os.name == "nt" else "tgx_lkh_stripifier")
    src_dir = patched / "SRC"
    if os.name == "nt":
        vc = find_msvc_command()
        if not vc:
            raise RuntimeError("MSVC not found. Install Visual Studio Build Tools or use a compiler in PATH.")
        cmd = f'call "{vc}" -arch=x64 >nul && cd /d "{src_dir}" && cl /nologo /O2 /D TWO_LEVEL_TREE /I INCLUDE *.c /Fe:"{exe}"'
        run_msvc(cmd)
    else:
        run(["make", "clean"], cwd=src_dir)
        run(["make"], cwd=src_dir)
        shutil.copy2(patched / "LKH", exe)
    return exe


def main() -> int:
    parser = argparse.ArgumentParser(description="Build TGX mesh stripifier helper executables.")
    parser.add_argument("--skip-ga-eax", action="store_true")
    parser.add_argument("--skip-lkh", action="store_true")
    args = parser.parse_args()

    config: dict[str, str] = {}
    if not args.skip_ga_eax:
        config["ga_eax"] = str(compile_ga_eax())
    if not args.skip_lkh:
        exe = compile_lkh()
        if exe:
            config["lkh"] = str(exe)

    BIN_DIR.mkdir(parents=True, exist_ok=True)
    config_path = BIN_DIR / "stripifiers.json"
    config_path.write_text(json.dumps(config, indent=2), encoding="utf-8")
    print(f"Wrote {config_path}")
    print(json.dumps(config, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
