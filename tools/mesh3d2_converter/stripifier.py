from __future__ import annotations

import subprocess
import tempfile
from dataclasses import dataclass
from pathlib import Path

from .mesh import FaceVertex, Meshlet, ObjMesh
from .meshlets import build_triangle_adjacency


DEFAULT_LKH_EXE = Path(r"D:\Programmation\myProjects\tgxmeshlets\LKH-2.exe")


@dataclass
class StripStats:
    meshlets: int
    triangles: int
    simple_chains: int
    lkh_chains: int
    lkh_runs: int

    @property
    def lkh_extra_vertex_refs(self) -> int:
        return 2 * self.lkh_chains

    @property
    def simple_vertices_loaded(self) -> int:
        return self.triangles + 2 * self.simple_chains

    @property
    def lkh_vertices_loaded(self) -> int:
        return self.triangles + 2 * self.lkh_chains


def _triangle_vertices(mesh: ObjMesh, tri_index: int) -> tuple[FaceVertex, FaceVertex, FaceVertex]:
    tri = mesh.triangles[tri_index]
    return tri.corners


def _edge(triangle: tuple[FaceVertex, FaceVertex, FaceVertex], i: int) -> tuple[FaceVertex, FaceVertex]:
    if i == 0:
        return triangle[0], triangle[1]
    if i == 1:
        return triangle[1], triangle[2]
    return triangle[2], triangle[0]


def _is_adjacent(a: tuple[FaceVertex, FaceVertex, FaceVertex], b: tuple[FaceVertex, FaceVertex, FaceVertex]) -> bool:
    a_edges = {_edge(a, 0), _edge(a, 1), _edge(a, 2)}
    b_edges_reversed = {(_edge(b, 0)[1], _edge(b, 0)[0]), (_edge(b, 1)[1], _edge(b, 1)[0]), (_edge(b, 2)[1], _edge(b, 2)[0])}
    return bool(a_edges.intersection(b_edges_reversed))


def _count_chains_in_order(triangles: list[tuple[FaceVertex, FaceVertex, FaceVertex]]) -> int:
    if not triangles:
        return 0
    chains = 1
    for i in range(1, len(triangles)):
        if not _is_adjacent(triangles[i - 1], triangles[i]):
            chains += 1
    return chains


def simple_chain_count(mesh: ObjMesh, meshlet: Meshlet) -> int:
    """Greedy chain count restricted to triangle adjacency inside one meshlet."""
    if not meshlet.triangles:
        return 0

    adjacency_all = build_triangle_adjacency(mesh)
    remaining = set(meshlet.triangles)
    chains = 0
    while remaining:
        chains += 1
        current = remaining.pop()
        while True:
            candidates = sorted(n for n in adjacency_all[current] if n in remaining)
            if not candidates:
                break
            current = candidates[0]
            remaining.remove(current)
    return chains


def _write_lkh_problem(workdir: Path, mesh: ObjMesh, tri_indices: list[int]) -> None:
    local_triangles = [_triangle_vertices(mesh, ti) for ti in tri_indices]
    local_count = len(local_triangles)

    with (workdir / "meshlet.par").open("w", newline="\n") as f:
        f.write("PROBLEM_FILE = meshlet.tsp\n")
        f.write("MOVE_TYPE = 5\n")
        f.write("PATCHING_C = 3\n")
        f.write("PATCHING_A = 2\n")
        f.write("RUNS = 4\n")
        f.write("OUTPUT_TOUR_FILE = meshlet_res.txt\n")

    with (workdir / "meshlet.tsp").open("w", newline="\n") as f:
        f.write("NAME: meshlet\n")
        f.write("TYPE: TSP\n")
        f.write(f"DIMENSION: {local_count}\n")
        f.write("EDGE_WEIGHT_TYPE: EXPLICIT\n")
        f.write("EDGE_WEIGHT_FORMAT: LOWER_DIAG_ROW\n")
        f.write("EDGE_WEIGHT_SECTION\n")
        for i in range(local_count):
            for j in range(i + 1):
                cost = 0 if i == j or _is_adjacent(local_triangles[i], local_triangles[j]) else 1
                f.write(f"{cost} ")
            f.write("\n")
        f.write("EOF\n")


def _read_lkh_tour(workdir: Path, tri_indices: list[int]) -> list[int]:
    lines = (workdir / "meshlet_res.txt").read_text().splitlines()
    try:
        start = next(i for i, line in enumerate(lines) if line.strip() == "TOUR_SECTION") + 1
    except StopIteration as exc:
        raise RuntimeError("LKH output tour does not contain TOUR_SECTION") from exc

    order: list[int] = []
    for line in lines[start:]:
        token = line.split()[0] if line.split() else ""
        if token == "-1" or token == "EOF":
            break
        order.append(tri_indices[int(token) - 1])
    return order


def lkh_chain_count(mesh: ObjMesh, meshlet: Meshlet, lkh_exe: str | Path = DEFAULT_LKH_EXE) -> tuple[int, bool]:
    """Return LKH chain count for a meshlet and whether LKH was actually run."""
    tri_indices = meshlet.triangles
    if len(tri_indices) <= 1:
        return len(tri_indices), False
    if len(tri_indices) == 2:
        a = _triangle_vertices(mesh, tri_indices[0])
        b = _triangle_vertices(mesh, tri_indices[1])
        return (1 if _is_adjacent(a, b) else 2), False

    lkh_exe = Path(lkh_exe)
    if not lkh_exe.exists():
        raise FileNotFoundError(f"LKH executable not found: {lkh_exe}")

    with tempfile.TemporaryDirectory(prefix="tgx_lkh_") as tmp:
        workdir = Path(tmp)
        _write_lkh_problem(workdir, mesh, tri_indices)
        completed = subprocess.run(
            [str(lkh_exe), "meshlet.par"],
            cwd=workdir,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.PIPE,
            text=True,
            check=False,
        )
        if completed.returncode != 0 or not (workdir / "meshlet_res.txt").exists():
            raise RuntimeError(f"LKH failed for meshlet with {len(tri_indices)} triangles: {completed.stderr}")
        tour = _read_lkh_tour(workdir, tri_indices)

    triangles = [_triangle_vertices(mesh, ti) for ti in tour]
    if len(triangles) > 1:
        # Cut the cyclic tour at a rupture when possible so a good circular tour is not penalized
        # by an arbitrary start point.
        for i in range(len(triangles)):
            if not _is_adjacent(triangles[i - 1], triangles[i]):
                triangles = triangles[i:] + triangles[:i]
                break
    return _count_chains_in_order(triangles), True


def strip_stats(mesh: ObjMesh, meshlets: list[Meshlet], lkh_exe: str | Path = DEFAULT_LKH_EXE) -> StripStats:
    simple = 0
    lkh = 0
    runs = 0
    triangles = 0
    for meshlet in meshlets:
        triangles += len(meshlet.triangles)
        simple += simple_chain_count(mesh, meshlet)
        count, did_run = lkh_chain_count(mesh, meshlet, lkh_exe)
        lkh += count
        if did_run:
            runs += 1
    return StripStats(len(meshlets), triangles, simple, lkh, runs)
