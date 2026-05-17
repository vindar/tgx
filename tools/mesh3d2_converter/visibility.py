from __future__ import annotations

import os
import shutil
import struct
import subprocess
import tempfile
from pathlib import Path

import numpy as np

from .mesh import Meshlet, ObjMesh


INPUT_MAGIC = b"TGVIS1\0\0"
OUTPUT_MAGIC = b"TGVOU1\0\0"
CPP_DIR = Path(__file__).resolve().parent / "cpp"
DEFAULT_BUILD_DIR = CPP_DIR / "build"


def build_tgx_visibility_helper(build_dir: str | Path = DEFAULT_BUILD_DIR) -> Path:
    build_dir = Path(build_dir)
    build_dir.mkdir(parents=True, exist_ok=True)
    exe = build_dir / "Release" / "tgx_mesh3d2_visibility.exe"
    if not exe.exists():
        exe = build_dir / "tgx_mesh3d2_visibility.exe"
    sources = [CPP_DIR / "CMakeLists.txt", CPP_DIR / "tgx_mesh3d2_visibility.cpp"]
    if exe.exists() and all(exe.stat().st_mtime >= src.stat().st_mtime for src in sources if src.exists()):
        return exe
    subprocess.check_call(["cmake", "-S", str(CPP_DIR), "-B", str(build_dir), "-DCMAKE_BUILD_TYPE=Release"])
    subprocess.check_call(["cmake", "--build", str(build_dir), "--config", "Release"])
    exe = build_dir / "Release" / "tgx_mesh3d2_visibility.exe"
    if not exe.exists():
        exe = build_dir / "tgx_mesh3d2_visibility.exe"
    if not exe.exists():
        raise FileNotFoundError(f"TGX visibility helper was not built in {build_dir}")
    return exe


def _write_visibility_input(path: Path, mesh: ObjMesh, meshlets: list[Meshlet], views: np.ndarray, width: int, height: int) -> None:
    tri_to_meshlet = np.full((len(mesh.triangles),), -1, dtype=np.int64)
    for mi, meshlet in enumerate(meshlets):
        for ti in meshlet.triangles:
            tri_to_meshlet[ti] = mi
    if np.any(tri_to_meshlet < 0):
        missing = int(np.sum(tri_to_meshlet < 0))
        raise ValueError(f"{missing} triangles are not assigned to any meshlet")

    views32 = np.asarray(views, dtype="<f4")
    vertices32 = np.asarray(mesh.vertices, dtype="<f4")
    with path.open("wb") as f:
        f.write(INPUT_MAGIC)
        f.write(struct.pack("<6I", width, height, len(views32), len(vertices32), len(mesh.triangles), len(meshlets)))
        f.write(views32.reshape(-1, 3).tobytes(order="C"))
        f.write(vertices32.reshape(-1, 3).tobytes(order="C"))
        for ti, tri in enumerate(mesh.triangles):
            v0, v1, v2 = mesh.triangle_vertex_indices(ti)
            f.write(struct.pack("<4I", v0, v1, v2, int(tri_to_meshlet[ti])))


def _read_visibility_output(path: Path, nb_views: int, nb_meshlets: int) -> np.ndarray:
    data = path.read_bytes()
    header_size = 8 + 8
    if len(data) < header_size or data[:8] != OUTPUT_MAGIC:
        raise ValueError("invalid TGX visibility output")
    out_views, out_meshlets = struct.unpack("<2I", data[8:16])
    if out_views != nb_views or out_meshlets != nb_meshlets:
        raise ValueError(f"visibility output shape mismatch: got {out_views}x{out_meshlets}, expected {nb_views}x{nb_meshlets}")
    payload = data[16:]
    expected = nb_views * nb_meshlets
    if len(payload) != expected:
        raise ValueError(f"visibility output size mismatch: got {len(payload)} bytes, expected {expected}")
    return np.frombuffer(payload, dtype=np.uint8).reshape((nb_views, nb_meshlets)).copy()


def compute_tgx_visibility(
    mesh: ObjMesh,
    meshlets: list[Meshlet],
    views: np.ndarray,
    *,
    width: int = 512,
    height: int = 512,
    helper: str | Path | None = None,
    keep_files: bool = False,
) -> np.ndarray:
    if helper is None:
        helper_path = build_tgx_visibility_helper()
    else:
        helper_path = Path(helper)
    if not helper_path.exists():
        raise FileNotFoundError(helper_path)

    tmp_parent = Path("tmp") if Path("tmp").exists() else Path(tempfile.gettempdir())
    tmp_parent.mkdir(parents=True, exist_ok=True)
    tmp_dir = Path(tempfile.mkdtemp(prefix="tgx_vis_", dir=tmp_parent))
    in_path = tmp_dir / "visibility_input.bin"
    out_path = tmp_dir / "visibility_output.bin"
    try:
        _write_visibility_input(in_path, mesh, meshlets, views, width, height)
        subprocess.check_call([str(helper_path), str(in_path), str(out_path)])
        return _read_visibility_output(out_path, len(views), len(meshlets))
    finally:
        if not keep_files:
            shutil.rmtree(tmp_dir, ignore_errors=True)
