from __future__ import annotations

from collections import defaultdict
from dataclasses import dataclass
from pathlib import Path
import subprocess
import tempfile

import numpy as np

from .mesh import Meshlet, ObjMesh, compute_triangle_centers, compute_triangle_normals, unique_valid


@dataclass
class MeshletBuildOptions:
    builder: str = "greedy"
    target_vertices: int = 52
    max_vertices: int = 127
    max_texcoords: int = 255
    max_normals: int = 255
    max_triangles: int = 70
    max_normal_angle_deg: float = 42.0
    radius_weight: float = 7.0
    normal_weight: float = 18.0
    vertex_weight: float = 2.0
    stop_score: float = 4.8
    shared_vertex_weight: float = 4.0
    test_pos_weight: float = 3.0
    compatible_edge_weight: float = 0.0
    incompatible_edge_penalty: float = 0.0
    compatible_merge_weight: float = 0.0
    lkh_exe: str = r"D:\Programmation\myProjects\tgxmeshlets\LKH-2.exe"
    nocull_lkh_component_limit: int = 500
    nocull_lkh_time_limit: int = 30
    fill_holes: bool = True
    merge_passes: int = 1
    smooth_passes: int = 0
    merge_max_normal_angle_deg: float = 48.0


def _edge_key(a: int, b: int) -> tuple[int, int]:
    return (a, b) if a < b else (b, a)


def build_triangle_adjacency(mesh: ObjMesh) -> list[set[int]]:
    edge_to_triangles: dict[tuple[int, int], list[int]] = defaultdict(list)
    for ti, tri in enumerate(mesh.triangles):
        v = [c.v for c in tri.corners]
        edge_to_triangles[_edge_key(v[0], v[1])].append(ti)
        edge_to_triangles[_edge_key(v[1], v[2])].append(ti)
        edge_to_triangles[_edge_key(v[2], v[0])].append(ti)

    adjacency = [set() for _ in mesh.triangles]
    for tris in edge_to_triangles.values():
        if len(tris) < 2:
            continue
        for a in tris:
            adjacency[a].update(b for b in tris if b != a)
    return adjacency


def build_edge_to_triangles(mesh: ObjMesh) -> dict[tuple[int, int], list[int]]:
    edge_to_triangles: dict[tuple[int, int], list[int]] = defaultdict(list)
    for ti, tri in enumerate(mesh.triangles):
        v = [c.v for c in tri.corners]
        edge_to_triangles[_edge_key(v[0], v[1])].append(ti)
        edge_to_triangles[_edge_key(v[1], v[2])].append(ti)
        edge_to_triangles[_edge_key(v[2], v[0])].append(ti)
    return edge_to_triangles


def _corner_edge_key(a, b) -> tuple:
    ka = (a.v, a.vt, a.vn)
    kb = (b.v, b.vt, b.vn)
    return (ka, kb) if ka < kb else (kb, ka)


def build_compatible_edge_to_triangles(mesh: ObjMesh) -> dict[tuple, list[int]]:
    edge_to_triangles: dict[tuple, list[int]] = defaultdict(list)
    for ti, tri in enumerate(mesh.triangles):
        c = tri.corners
        edge_to_triangles[_corner_edge_key(c[0], c[1])].append(ti)
        edge_to_triangles[_corner_edge_key(c[1], c[2])].append(ti)
        edge_to_triangles[_corner_edge_key(c[2], c[0])].append(ti)
    return edge_to_triangles


def _triangle_sets(mesh: ObjMesh, tri_index: int) -> tuple[set[int], set[int], set[int]]:
    tri = mesh.triangles[tri_index]
    return (
        {c.v for c in tri.corners},
        unique_valid(c.vt for c in tri.corners),
        unique_valid(c.vn for c in tri.corners),
    )


def _sphere_from_vertices(points: np.ndarray) -> tuple[np.ndarray, float]:
    bb_min = np.min(points, axis=0)
    bb_max = np.max(points, axis=0)
    center = (bb_min + bb_max) * 0.5
    radius = float(np.max(np.linalg.norm(points - center, axis=1))) if len(points) else 0.0
    return center, radius


def _normal_cone(normals: np.ndarray) -> tuple[np.ndarray, float]:
    if len(normals) == 0:
        return np.array([0.0, 0.0, 1.0], dtype=np.float64), 1.0
    if float(np.max(np.linalg.norm(normals, axis=1))) <= 1e-12:
        return np.array([0.0, 0.0, 1.0], dtype=np.float64), 0.0
    if len(normals) == 1:
        length = float(np.linalg.norm(normals[0]))
        if length <= 1e-12:
            return np.array([0.0, 0.0, 1.0], dtype=np.float64), 0.0
        return normals[0].copy() / length, 1.0

    seeds = [
        np.array([1.0, 0.0, 0.0]),
        np.array([-1.0, 0.0, 0.0]),
        np.array([0.0, 1.0, 0.0]),
        np.array([0.0, -1.0, 0.0]),
        np.array([0.0, 0.0, 1.0]),
        np.array([0.0, 0.0, -1.0]),
    ]
    mean_axis = np.sum(normals, axis=0)
    mean_len = float(np.linalg.norm(mean_axis))
    if mean_len > 1e-12:
        seeds.append(mean_axis / mean_len)
    seeds.extend(normals)

    best_axis = seeds[0]
    best_dot = -2.0
    for axis in seeds:
        axis_len = float(np.linalg.norm(axis))
        if axis_len <= 1e-12:
            continue
        axis = axis / axis_len
        min_dot = float(np.min(normals @ axis))
        if min_dot > best_dot:
            best_dot = min_dot
            best_axis = axis

    # Local search on the unit sphere. This is the same spirit as the old prototype:
    # maximize the minimum dot product between the axis and all triangle normals.
    directions = np.asarray(
        [
            [1.0, 0.0, 0.0],
            [-1.0, 0.0, 0.0],
            [0.0, 1.0, 0.0],
            [0.0, -1.0, 0.0],
            [0.0, 0.0, 1.0],
            [0.0, 0.0, -1.0],
            [0.0, 0.52573111, 0.85065081],
            [0.0, -0.52573111, 0.85065081],
            [0.52573111, 0.85065081, 0.0],
            [-0.52573111, 0.85065081, 0.0],
            [0.85065081, 0.0, 0.52573111],
            [-0.85065081, 0.0, 0.52573111],
        ],
        dtype=np.float64,
    )
    eps = 0.1
    while eps > 0.0005:
        improved = False
        base_axis = best_axis.copy()
        for direction in directions:
            axis = base_axis + eps * direction
            axis_len = float(np.linalg.norm(axis))
            if axis_len <= 1e-12:
                continue
            axis /= axis_len
            min_dot = float(np.min(normals @ axis))
            if min_dot > best_dot:
                best_dot = min_dot
                best_axis = axis
                improved = True
        if not improved:
            eps *= 0.5
    return best_axis, best_dot


def _normal_cone_fast(normals: np.ndarray) -> tuple[np.ndarray, float]:
    if len(normals) == 0:
        return np.array([0.0, 0.0, 1.0], dtype=np.float64), 1.0
    axis = np.sum(normals, axis=0)
    length = float(np.linalg.norm(axis))
    if length <= 1e-12:
        axis = normals[0].copy()
        length = float(np.linalg.norm(axis))
    axis = axis / length if length > 0.0 else np.array([0.0, 0.0, 1.0], dtype=np.float64)
    return axis, float(np.min(normals @ axis))


def _backface_cull_cone(normal_axis: np.ndarray, normal_min_dot: float) -> tuple[np.ndarray, float]:
    """Return the view-direction cone where all normals are back-facing.

    The normal cone half-angle is alpha=acos(normal_min_dot). All faces are back-facing
    for camera-to-object directions inside the normal cone axis with half-angle
    max(0, 90deg-alpha).
    cull_cos > 1 marks an empty culling cone.
    """
    if normal_min_dot <= 0.0:
        return normal_axis, 2.0
    cull_cos = float(np.sqrt(max(0.0, 1.0 - normal_min_dot * normal_min_dot)))
    return normal_axis, cull_cos


def _make_meshlet(mesh: ObjMesh, tri_indices: list[int], triangle_normals: np.ndarray) -> Meshlet:
    vertices: set[int] = set()
    texcoords: set[int] = set()
    normals: set[int] = set()
    for ti in tri_indices:
        v, vt, vn = _triangle_sets(mesh, ti)
        vertices.update(v)
        texcoords.update(vt)
        normals.update(vn)

    center, radius = _sphere_from_vertices(mesh.vertices[list(vertices)])
    normal_axis, normal_min_dot = _normal_cone(triangle_normals[tri_indices])
    cull_axis, cull_cos = _backface_cull_cone(normal_axis, normal_min_dot)
    material = mesh.triangles[tri_indices[0]].material
    return Meshlet(tri_indices, material, vertices, texcoords, normals, center, radius, normal_axis, normal_min_dot, cull_axis, cull_cos)


def _make_nocull_meshlet(mesh: ObjMesh, tri_indices: list[int]) -> Meshlet:
    vertices: set[int] = set()
    texcoords: set[int] = set()
    normals: set[int] = set()
    for ti in tri_indices:
        v, vt, vn = _triangle_sets(mesh, ti)
        vertices.update(v)
        texcoords.update(vt)
        normals.update(vn)

    center, radius = _sphere_from_vertices(mesh.vertices[list(vertices)])
    axis = np.array([0.0, 0.0, 1.0], dtype=np.float64)
    material = mesh.triangles[tri_indices[0]].material
    return Meshlet(
        tri_indices.copy(),
        material,
        vertices,
        texcoords,
        normals,
        center,
        radius,
        axis,
        1.0,
        axis,
        2.0,
        visibility_axis=axis,
        visibility_cos=-1.0,
        visibility_cull_axis=axis,
        visibility_cull_cos=2.0,
        visibility_samples=0,
    )


def build_meshlets(mesh: ObjMesh, options: MeshletBuildOptions | None = None) -> list[Meshlet]:
    """Build connected, local, normal-coherent meshlets under Mesh3D2 index limits."""
    opt = options or MeshletBuildOptions()
    if opt.builder == "nocull":
        return build_meshlets_nocull(mesh, opt)
    if opt.builder == "hybrid":
        meshlets = build_meshlets_hybrid(mesh, opt)
        return refine_meshlets(mesh, meshlets, opt)
    if opt.builder != "greedy":
        raise ValueError(f"unknown meshlet builder: {opt.builder}")

    centers = compute_triangle_centers(mesh)
    normals = compute_triangle_normals(mesh)
    adjacency = build_triangle_adjacency(mesh)
    scale = mesh.scale_hint()

    remaining: set[int] = set(range(len(mesh.triangles)))
    meshlets: list[Meshlet] = []

    max_normal_cost = 1.0 - float(np.cos(np.radians(opt.max_normal_angle_deg)))

    def candidate_score(
        current: list[int],
        vertices: set[int],
        texcoords: set[int],
        normal_indices: set[int],
        current_radius: float,
        current_center_mean: np.ndarray,
        tri_index: int,
    ) -> float | None:
        tri = mesh.triangles[tri_index]
        if tri.material != mesh.triangles[current[0]].material:
            return None

        v, vt, vn = _triangle_sets(mesh, tri_index)
        new_vertices = vertices | v
        if len(new_vertices) > opt.max_vertices:
            return None
        if len(texcoords | vt) > opt.max_texcoords:
            return None
        if len(normal_indices | vn) > opt.max_normals:
            return None
        if len(current) >= opt.max_triangles:
            return None

        new_center, new_radius = _sphere_from_vertices(mesh.vertices[list(new_vertices)])
        _ = new_center
        radius_cost = max(0.0, new_radius - current_radius) / scale

        normal_axis, normal_min_dot = _normal_cone_fast(normals[current + [tri_index]])
        _ = normal_axis
        normal_cost = 1.0 - normal_min_dot
        if normal_cost > max_normal_cost:
            return None

        new_vertex_cost = len(new_vertices) - len(vertices)
        spatial_hint = float(np.linalg.norm(centers[tri_index] - current_center_mean)) / scale

        return (
            opt.vertex_weight * new_vertex_cost
            + opt.radius_weight * radius_cost
            + opt.normal_weight * normal_cost
            + 0.5 * spatial_hint
        )

    while remaining:
        seed = min(remaining)
        current = [seed]
        remaining.remove(seed)
        vertices, texcoords, normals_set = _triangle_sets(mesh, seed)
        frontier = {n for n in adjacency[seed] if n in remaining}

        while frontier:
            _, current_radius = _sphere_from_vertices(mesh.vertices[list(vertices)])
            current_center_mean = np.mean(centers[current], axis=0)
            scored = []
            for ti in sorted(frontier):
                score = candidate_score(current, vertices, texcoords, normals_set, current_radius, current_center_mean, ti)
                if score is not None:
                    scored.append((score, ti))
            if not scored:
                break
            score, chosen = min(scored, key=lambda x: (x[0], x[1]))
            chosen_vertices, chosen_texcoords, chosen_normals = _triangle_sets(mesh, chosen)
            if len(vertices) >= opt.target_vertices and score > opt.stop_score:
                break

            current.append(chosen)
            remaining.remove(chosen)
            vertices.update(chosen_vertices)
            texcoords.update(chosen_texcoords)
            normals_set.update(chosen_normals)
            frontier.discard(chosen)
            for n in adjacency[chosen]:
                if n in remaining:
                    frontier.add(n)

        meshlets.append(_make_meshlet(mesh, current, normals))

    return refine_meshlets(mesh, meshlets, opt)


def sort_meshlets_by_material(meshlets: list[Meshlet]) -> list[Meshlet]:
    """Return meshlets grouped by material while preserving local order inside each group."""
    return [meshlet for _, meshlet in sorted(enumerate(meshlets), key=lambda item: (item[1].material, item[0]))]


def build_meshlets_nocull(mesh: ObjMesh, options: MeshletBuildOptions | None = None) -> list[Meshlet]:
    """Build large, compact Mesh3D2 meshlets with meshlet culling disabled.

    This builder is intended as a compact Mesh3D-like mode. It first splits the mesh into
    strong compatible connected components: two triangles are connected only when they share an
    edge with identical vertex/texcoord/normal corners and the same material. Components that
    already fit in one meshlet are kept whole. Other components are ordered with LKH under a
    configurable time limit, then packed into the largest local-index meshlets possible.
    """
    opt = options or MeshletBuildOptions(builder="nocull")
    components = _compatible_components_by_material(mesh)
    meshlets: list[Meshlet] = []

    for component in components:
        if _fits_nocull(mesh, component, opt):
            meshlets.append(_make_nocull_meshlet(mesh, component))
        else:
            ordered = _lkh_order_for_component(mesh, component, opt.lkh_exe, opt.nocull_lkh_time_limit)
            meshlets.extend(_pack_nocull_order(mesh, ordered, opt))

    if opt.merge_passes <= 0:
        return meshlets
    return _merge_nocull_meshlets(mesh, meshlets, opt)


def _compatible_components_by_material(mesh: ObjMesh) -> list[list[int]]:
    edge_to_tris = build_compatible_edge_to_triangles(mesh)
    adjacency = [set() for _ in mesh.triangles]
    for tris in edge_to_tris.values():
        if len(tris) < 2:
            continue
        for a in tris:
            for b in tris:
                if a != b and mesh.triangles[a].material == mesh.triangles[b].material:
                    adjacency[a].add(b)

    seen: set[int] = set()
    components: list[list[int]] = []
    for ti in range(len(mesh.triangles)):
        if ti in seen:
            continue
        stack = [ti]
        seen.add(ti)
        component: list[int] = []
        while stack:
            current = stack.pop()
            component.append(current)
            for other in adjacency[current]:
                if other not in seen:
                    seen.add(other)
                    stack.append(other)
        components.append(component)
    components.sort(key=len, reverse=True)
    return components


def _lkh_order_for_component(mesh: ObjMesh, tri_indices: list[int], lkh_exe: str | Path, time_limit: int) -> list[int]:
    if len(tri_indices) <= 2:
        return tri_indices.copy()
    try:
        from .exporter import _is_chain_adjacent
        from .stripifier import _read_lkh_tour, _triangle_vertices
    except Exception:
        return tri_indices.copy()

    lkh_exe = Path(lkh_exe)
    if not lkh_exe.exists():
        return tri_indices.copy()

    local_triangles = [_triangle_vertices(mesh, ti) for ti in tri_indices]
    try:
        with tempfile.TemporaryDirectory(prefix="tgx_lkh_nocull_") as tmp:
            workdir = Path(tmp)
            with (workdir / "meshlet.par").open("w", newline="\n") as f:
                f.write("PROBLEM_FILE = meshlet.tsp\n")
                f.write("MOVE_TYPE = 5\n")
                f.write("PATCHING_C = 3\n")
                f.write("PATCHING_A = 2\n")
                f.write("RUNS = 4\n")
                if time_limit > 0:
                    f.write(f"TIME_LIMIT = {int(time_limit)}\n")
                f.write("TRACE_LEVEL = 0\n")
                f.write("OUTPUT_TOUR_FILE = meshlet_res.txt\n")

            with (workdir / "meshlet.tsp").open("w", newline="\n") as f:
                f.write("NAME: meshlet\n")
                f.write("TYPE: TSP\n")
                f.write(f"DIMENSION: {len(local_triangles)}\n")
                f.write("EDGE_WEIGHT_TYPE: EXPLICIT\n")
                f.write("EDGE_WEIGHT_FORMAT: LOWER_DIAG_ROW\n")
                f.write("EDGE_WEIGHT_SECTION\n")
                for i in range(len(local_triangles)):
                    for j in range(i + 1):
                        cost = 0 if i == j or _is_chain_adjacent(local_triangles[i], local_triangles[j]) else 1
                        f.write(f"{cost} ")
                    f.write("\n")
                f.write("EOF\n")

            completed = subprocess.run(
                [str(lkh_exe), "meshlet.par"],
                cwd=workdir,
                stdout=subprocess.DEVNULL,
                stderr=subprocess.PIPE,
                text=True,
                check=False,
            )
            if completed.returncode != 0 or not (workdir / "meshlet_res.txt").exists():
                return tri_indices.copy()
            return _read_lkh_tour(workdir, tri_indices)
    except Exception:
        return tri_indices.copy()


def _fits_nocull(mesh: ObjMesh, tri_indices: list[int], opt: MeshletBuildOptions) -> bool:
    if not tri_indices or len(tri_indices) > opt.max_triangles:
        return False
    material = mesh.triangles[tri_indices[0]].material
    vertices: set[int] = set()
    texcoords: set[int] = set()
    normals: set[int] = set()
    for ti in tri_indices:
        if mesh.triangles[ti].material != material:
            return False
        v, vt, vn = _triangle_sets(mesh, ti)
        vertices.update(v)
        texcoords.update(vt)
        normals.update(vn)
        if len(vertices) > opt.max_vertices or len(texcoords) > opt.max_texcoords or len(normals) > opt.max_normals:
            return False
    return True


def _pack_nocull_order(mesh: ObjMesh, ordered: list[int], opt: MeshletBuildOptions) -> list[Meshlet]:
    return _pack_nocull_units(mesh, _nocull_chain_units(mesh, ordered, opt), opt)


def _nocull_chain_units(mesh: ObjMesh, ordered: list[int], opt: MeshletBuildOptions) -> list[list[int]]:
    if not ordered:
        return []
    try:
        from .exporter import _is_chain_adjacent
    except Exception:
        return [[ti] for ti in ordered]

    triangles = [mesh.triangles[i].corners for i in ordered]
    if len(triangles) > 1:
        for i in range(len(triangles)):
            if not _is_chain_adjacent(triangles[i - 1], triangles[i]):
                ordered = ordered[i:] + ordered[:i]
                triangles = triangles[i:] + triangles[:i]
                break

    units: list[list[int]] = []
    current: list[int] = []
    for ti, tri in zip(ordered, triangles):
        can_continue = bool(current) and len(current) < 255 and _is_chain_adjacent(mesh.triangles[current[-1]].corners, tri)
        if not can_continue:
            if current:
                units.extend(_split_nocull_unit_to_fit(mesh, current, opt))
            current = [ti]
        else:
            current.append(ti)
    if current:
        units.extend(_split_nocull_unit_to_fit(mesh, current, opt))
    return units


def _split_nocull_unit_to_fit(mesh: ObjMesh, tri_indices: list[int], opt: MeshletBuildOptions) -> list[list[int]]:
    units: list[list[int]] = []
    current: list[int] = []
    vertices: set[int] = set()
    texcoords: set[int] = set()
    normals: set[int] = set()
    for ti in tri_indices:
        v, vt, vn = _triangle_sets(mesh, ti)
        fits = (
            current
            and len(current) < 255
            and len(current) < opt.max_triangles
            and len(vertices | v) <= opt.max_vertices
            and len(texcoords | vt) <= opt.max_texcoords
            and len(normals | vn) <= opt.max_normals
        )
        if not current:
            fits = True
        if not fits:
            units.append(current)
            current = []
            vertices = set()
            texcoords = set()
            normals = set()
        current.append(ti)
        vertices.update(v)
        texcoords.update(vt)
        normals.update(vn)
    if current:
        units.append(current)
    return units


def _pack_nocull_units(mesh: ObjMesh, units: list[list[int]], opt: MeshletBuildOptions) -> list[Meshlet]:
    meshlets: list[Meshlet] = []
    unit_data = []
    for unit in units:
        vertices: set[int] = set()
        texcoords: set[int] = set()
        normals: set[int] = set()
        for ti in unit:
            v, vt, vn = _triangle_sets(mesh, ti)
            vertices.update(v)
            texcoords.update(vt)
            normals.update(vn)
        unit_data.append((unit, mesh.triangles[unit[0]].material, vertices, texcoords, normals))

    # Large units are hardest to place. Smaller chains are then packed into the best compatible bin.
    unit_data.sort(key=lambda item: (len(item[2]), len(item[4]), len(item[3]), len(item[0])), reverse=True)

    bins: list[dict] = []
    for unit, material, vertices, texcoords, normals in unit_data:
        best: tuple[tuple[int, int, int, int, int], int] | None = None
        for bi, bin_item in enumerate(bins):
            if bin_item["material"] != material:
                continue
            tri_count = len(bin_item["triangles"]) + len(unit)
            if tri_count > opt.max_triangles:
                continue
            new_vertices = bin_item["vertices"] | vertices
            if len(new_vertices) > opt.max_vertices:
                continue
            new_texcoords = bin_item["texcoords"] | texcoords
            if len(new_texcoords) > opt.max_texcoords:
                continue
            new_normals = bin_item["normals"] | normals
            if len(new_normals) > opt.max_normals:
                continue
            score = (
                len(bin_item["vertices"] & vertices),
                len(bin_item["normals"] & normals),
                len(bin_item["texcoords"] & texcoords),
                -len(new_vertices) - len(new_normals) - len(new_texcoords),
                tri_count,
            )
            if best is None or score > best[0]:
                best = (score, bi)
        if best is None:
            bins.append(
                {
                    "material": material,
                    "triangles": list(unit),
                    "vertices": set(vertices),
                    "texcoords": set(texcoords),
                    "normals": set(normals),
                }
            )
            continue
        _, bi = best
        bin_item = bins[bi]
        bin_item["triangles"].extend(unit)
        bin_item["vertices"].update(vertices)
        bin_item["texcoords"].update(texcoords)
        bin_item["normals"].update(normals)

    for bin_item in bins:
        meshlets.append(_make_nocull_meshlet(mesh, bin_item["triangles"]))
    return meshlets


def _split_large_component_nocull(mesh: ObjMesh, component: list[int], opt: MeshletBuildOptions) -> list[list[int]]:
    edge_to_tris = build_compatible_edge_to_triangles(mesh)
    adjacency = {ti: set() for ti in component}
    component_set = set(component)
    for tris in edge_to_tris.values():
        local = [ti for ti in tris if ti in component_set]
        if len(local) < 2:
            continue
        for a in local:
            adjacency[a].update(b for b in local if b != a and mesh.triangles[a].material == mesh.triangles[b].material)

    remaining = set(component)
    blocks: list[list[int]] = []
    centers = compute_triangle_centers(mesh)

    while remaining:
        seed = max(remaining, key=lambda ti: len(adjacency[ti] & remaining))
        current = [seed]
        remaining.remove(seed)
        vertices, texcoords, normals = _triangle_sets(mesh, seed)
        frontier = set(adjacency[seed] & remaining)

        while frontier:
            scored = []
            current_set = set(current)
            current_center = np.mean(centers[current], axis=0)
            for ti in sorted(frontier):
                v, vt, vn = _triangle_sets(mesh, ti)
                if len(current) >= opt.max_triangles or len(current) >= opt.nocull_lkh_component_limit:
                    continue
                if len(vertices | v) > opt.max_vertices or len(texcoords | vt) > opt.max_texcoords or len(normals | vn) > opt.max_normals:
                    continue
                links = len(adjacency[ti] & current_set)
                future_links = len(adjacency[ti] & remaining)
                new_indices = len((vertices | v)) - len(vertices)
                distance = float(np.linalg.norm(centers[ti] - current_center))
                scored.append((-links, -future_links, new_indices, distance, ti))
            if not scored:
                break
            *_, chosen = min(scored)
            frontier.discard(chosen)
            if chosen not in remaining:
                continue
            current.append(chosen)
            remaining.remove(chosen)
            v, vt, vn = _triangle_sets(mesh, chosen)
            vertices.update(v)
            texcoords.update(vt)
            normals.update(vn)
            frontier.update(adjacency[chosen] & remaining)

        blocks.append(current)
    return blocks


def _merge_nocull_meshlets(mesh: ObjMesh, meshlets: list[Meshlet], opt: MeshletBuildOptions) -> list[Meshlet]:
    merges = 0
    while True:
        best: tuple[tuple[int, int, int], int, int] | None = None
        for i, a in enumerate(meshlets):
            for j in range(i + 1, len(meshlets)):
                b = meshlets[j]
                if a.material != b.material:
                    continue
                tri_count = len(a.triangles) + len(b.triangles)
                if tri_count > opt.max_triangles:
                    continue
                vertices = a.vertices | b.vertices
                if len(vertices) > opt.max_vertices:
                    continue
                normals = a.normals | b.normals
                if len(normals) > opt.max_normals:
                    continue
                texcoords = a.texcoords | b.texcoords
                if len(texcoords) > opt.max_texcoords:
                    continue

                payload_saved = 12 * len(a.vertices & b.vertices) + 12 * len(a.normals & b.normals) + 8 * len(a.texcoords & b.texcoords)
                fill = len(vertices) + len(normals) + len(texcoords)
                score = (payload_saved, tri_count, -fill)
                if best is None or score > best[0]:
                    best = (score, i, j)

        if best is None:
            print(f"nocull merge: {merges} merges, {len(meshlets)} meshlets")
            break
        _, i, j = best
        meshlets[i] = _make_nocull_meshlet(mesh, meshlets[i].triangles + meshlets[j].triangles)
        del meshlets[j]
        merges += 1
        if (merges % 50) == 0:
            print(f"nocull merge: {merges} merges, {len(meshlets)} meshlets")
    return meshlets


def _meshlets_share_compatible_edge(mesh: ObjMesh, a: Meshlet, b: Meshlet) -> bool:
    edge_to_tris = build_compatible_edge_to_triangles(mesh)
    b_tris = set(b.triangles)
    for ti in a.triangles:
        tri = mesh.triangles[ti]
        c = tri.corners
        for key in (_corner_edge_key(c[0], c[1]), _corner_edge_key(c[1], c[2]), _corner_edge_key(c[2], c[0])):
            if any(tj in b_tris for tj in edge_to_tris.get(key, [])):
                return True
    return False


def build_meshlets_hybrid(mesh: ObjMesh, options: MeshletBuildOptions | None = None) -> list[Meshlet]:
    """Build meshlets with a Meshlete-inspired border growth strategy.

    The original greedy builder scores all frontier triangles similarly. This variant is closer
    to Meshlete's generator: triangles are seeded in spatial order along the mesh major axis, then
    each meshlet grows from its active border, strongly preferring triangles sharing two or three
    vertices with the current meshlet. TGX-specific constraints keep normal cones and local index
    counts under control.
    """
    opt = options or MeshletBuildOptions(builder="hybrid")
    centers = compute_triangle_centers(mesh)
    tri_normals = compute_triangle_normals(mesh)
    adjacency = build_triangle_adjacency(mesh)
    edge_to_triangles = build_edge_to_triangles(mesh)
    compatible_edge_to_triangles = build_compatible_edge_to_triangles(mesh)
    scale = mesh.scale_hint()

    bb_min, bb_max = mesh.bounding_box()
    axis_id = int(np.argmax(bb_max - bb_min))
    axis = np.zeros(3, dtype=np.float64)
    axis[axis_id] = 1.0

    tri_order: list[tuple[float, int]] = []
    for ti in range(len(mesh.triangles)):
        pts = mesh.triangle_vertices(ti)
        tri_order.append((float(np.min(pts @ axis)), ti))
    tri_order.sort()

    remaining: set[int] = set(range(len(mesh.triangles)))
    meshlets: list[Meshlet] = []
    max_normal_cost = 1.0 - float(np.cos(np.radians(opt.max_normal_angle_deg)))

    def triangle_edges(ti: int) -> list[tuple[int, int]]:
        v = [c.v for c in mesh.triangles[ti].corners]
        return [_edge_key(v[0], v[1]), _edge_key(v[1], v[2]), _edge_key(v[2], v[0])]

    def geometric_link_count(ti: int, current_set: set[int]) -> int:
        count = 0
        for key in triangle_edges(ti):
            if any(tj in current_set for tj in edge_to_triangles.get(key, [])):
                count += 1
        return count

    def compatible_link_count(ti: int, current_set: set[int]) -> int:
        tri = mesh.triangles[ti]
        c = tri.corners
        count = 0
        for key in (_corner_edge_key(c[0], c[1]), _corner_edge_key(c[1], c[2]), _corner_edge_key(c[2], c[0])):
            if any(tj in current_set for tj in compatible_edge_to_triangles.get(key, [])):
                count += 1
        return count

    def next_seed() -> int:
        while tri_order:
            _, ti = tri_order.pop(0)
            if ti in remaining:
                return ti
        return min(remaining)

    def score_candidate(
        current: list[int],
        vertices: set[int],
        texcoords: set[int],
        normal_indices: set[int],
        current_set: set[int],
        test_pos: np.ndarray,
        old_radius: float,
        ti: int,
    ) -> tuple[float, set[int], set[int], set[int], int] | None:
        tri = mesh.triangles[ti]
        if tri.material != mesh.triangles[current[0]].material:
            return None

        v, vt, vn = _triangle_sets(mesh, ti)
        shared = len(v & vertices)
        new_vertices = vertices | v
        new_texcoords = texcoords | vt
        new_normals = normal_indices | vn
        if len(new_vertices) > opt.max_vertices:
            return None
        if len(new_texcoords) > opt.max_texcoords:
            return None
        if len(new_normals) > opt.max_normals:
            return None
        if len(current) >= opt.max_triangles:
            return None

        normal_axis, normal_min_dot = _normal_cone_fast(tri_normals[current + [ti]])
        _ = normal_axis
        normal_cost = 1.0 - normal_min_dot
        if normal_cost > max_normal_cost:
            return None

        _, new_radius = _sphere_from_vertices(mesh.vertices[list(new_vertices)])
        radius_cost = max(0.0, new_radius - old_radius) / scale
        test_cost = min(float(np.linalg.norm(mesh.vertices[vi] - test_pos)) for vi in v) / scale
        new_vertex_cost = len(new_vertices) - len(vertices)
        centroid_cost = float(np.linalg.norm(centers[ti] - np.mean(centers[current], axis=0))) / scale
        compatible_links = compatible_link_count(ti, current_set)
        incompatible_links = max(0, geometric_link_count(ti, current_set) - compatible_links)
        score = (
            opt.vertex_weight * new_vertex_cost
            + opt.radius_weight * radius_cost
            + opt.normal_weight * normal_cost
            + opt.test_pos_weight * test_cost
            + 0.75 * centroid_cost
            + opt.incompatible_edge_penalty * incompatible_links
            - opt.shared_vertex_weight * shared
            - opt.compatible_edge_weight * compatible_links
        )
        return score, new_vertices, new_texcoords, new_normals, shared + compatible_links

    while remaining:
        seed = next_seed()
        current = [seed]
        remaining.remove(seed)
        vertices, texcoords, normals_set = _triangle_sets(mesh, seed)
        current_set = {seed}
        test_pos = centers[seed].copy()
        border_edges = set(triangle_edges(seed))

        while border_edges:
            _, old_radius = _sphere_from_vertices(mesh.vertices[list(vertices)])
            candidates: dict[int, tuple[float, set[int], set[int], set[int], int]] = {}
            dead_edges = []
            for edge in sorted(border_edges):
                found = False
                for ti in edge_to_triangles.get(edge, []):
                    if ti not in remaining:
                        continue
                    found = True
                    scored = score_candidate(current, vertices, texcoords, normals_set, current_set, test_pos, old_radius, ti)
                    if scored is not None and (ti not in candidates or scored[0] < candidates[ti][0]):
                        candidates[ti] = scored
                if not found:
                    dead_edges.append(edge)
            for edge in dead_edges:
                border_edges.discard(edge)
            if not candidates:
                break

            chosen, data = min(candidates.items(), key=lambda item: (item[1][0], -item[1][4], item[0]))
            score, new_vertices, new_texcoords, new_normals, shared = data
            if len(vertices) >= opt.target_vertices and score > opt.stop_score and shared < 3:
                break

            current.append(chosen)
            remaining.remove(chosen)
            current_set.add(chosen)
            vertices, texcoords, normals_set = new_vertices, new_texcoords, new_normals
            alpha = 1.0 / (len(current) + 1.0)
            test_pos = (1.0 - alpha) * test_pos + alpha * centers[chosen]
            for edge in triangle_edges(chosen):
                border_edges.add(edge)

        if opt.fill_holes:
            # Cheap Meshlete-like cleanup: add adjacent unassigned triangles whose vertices are
            # already local. This usually improves stripification without increasing vertex count.
            changed = True
            while changed and len(current) < opt.max_triangles:
                changed = False
                for base in list(current):
                    for ti in sorted(adjacency[base]):
                        if ti not in remaining:
                            continue
                        if opt.compatible_edge_weight > 0.0 and compatible_link_count(ti, current_set) <= 0:
                            continue
                        v, vt, vn = _triangle_sets(mesh, ti)
                        if not v.issubset(vertices):
                            continue
                        if len(texcoords | vt) > opt.max_texcoords or len(normals_set | vn) > opt.max_normals:
                            continue
                        normal_axis, normal_min_dot = _normal_cone_fast(tri_normals[current + [ti]])
                        _ = normal_axis
                        if 1.0 - normal_min_dot > max_normal_cost:
                            continue
                        current.append(ti)
                        remaining.remove(ti)
                        current_set.add(ti)
                        texcoords.update(vt)
                        normals_set.update(vn)
                        changed = True
                        if len(current) >= opt.max_triangles:
                            break
                    if len(current) >= opt.max_triangles:
                        break

        meshlets.append(_make_meshlet(mesh, current, tri_normals))

    return meshlets


def refine_meshlets(mesh: ObjMesh, meshlets: list[Meshlet], options: MeshletBuildOptions) -> list[Meshlet]:
    """Optional post-process inspired by the old prototype.

    Merging removes tiny adjacent meshlets when the combined meshlet still satisfies the
    local-index and normal-coherence constraints. Smoothing then moves boundary triangles to
    neighboring meshlets if it improves local connectivity while preserving constraints.
    """
    tri_normals = compute_triangle_normals(mesh)
    for _ in range(max(0, options.merge_passes)):
        meshlets, changed = _merge_meshlet_pass(mesh, meshlets, tri_normals, options)
        if not changed:
            break
    for _ in range(max(0, options.smooth_passes)):
        meshlets, changed = _smooth_meshlet_pass(mesh, meshlets, tri_normals, options)
        if not changed:
            break
    return meshlets


def _meshlet_from_tris_if_valid(
    mesh: ObjMesh,
    tri_indices: list[int],
    tri_normals: np.ndarray,
    options: MeshletBuildOptions,
    max_normal_angle_deg: float | None = None,
) -> Meshlet | None:
    if not tri_indices:
        return None
    if len(tri_indices) > options.max_triangles:
        return None
    material = mesh.triangles[tri_indices[0]].material
    vertices: set[int] = set()
    texcoords: set[int] = set()
    normals: set[int] = set()
    for ti in tri_indices:
        if mesh.triangles[ti].material != material:
            return None
        v, vt, vn = _triangle_sets(mesh, ti)
        vertices.update(v)
        texcoords.update(vt)
        normals.update(vn)
    if len(vertices) > options.max_vertices or len(texcoords) > options.max_texcoords or len(normals) > options.max_normals:
        return None
    normal_axis, normal_min_dot = _normal_cone(tri_normals[tri_indices])
    angle = float(np.degrees(np.arccos(np.clip(normal_min_dot, -1.0, 1.0))))
    if angle > (max_normal_angle_deg if max_normal_angle_deg is not None else options.max_normal_angle_deg):
        return None
    center, radius = _sphere_from_vertices(mesh.vertices[list(vertices)])
    cull_axis, cull_cos = _backface_cull_cone(normal_axis, normal_min_dot)
    return Meshlet(tri_indices.copy(), material, vertices, texcoords, normals, center, radius, normal_axis, normal_min_dot, cull_axis, cull_cos)


def _meshlet_adjacency(mesh: ObjMesh, meshlets: list[Meshlet], tri_to_meshlet: dict[int, int]) -> list[set[int]]:
    tri_adj = build_triangle_adjacency(mesh)
    adj = [set() for _ in meshlets]
    for mi, meshlet in enumerate(meshlets):
        for ti in meshlet.triangles:
            for tj in tri_adj[ti]:
                mj = tri_to_meshlet.get(tj)
                if mj is not None and mj != mi:
                    adj[mi].add(mj)
    return adj


def _compatible_meshlet_links(mesh: ObjMesh, a: Meshlet, b: Meshlet) -> int:
    edge_to_triangles = build_compatible_edge_to_triangles(mesh)
    b_tris = set(b.triangles)
    links: set[tuple[int, int]] = set()
    for ti in a.triangles:
        tri = mesh.triangles[ti]
        c = tri.corners
        for key in (_corner_edge_key(c[0], c[1]), _corner_edge_key(c[1], c[2]), _corner_edge_key(c[2], c[0])):
            for tj in edge_to_triangles.get(key, []):
                if tj in b_tris:
                    links.add((min(ti, tj), max(ti, tj)))
    return len(links)


def _tri_to_meshlet(meshlets: list[Meshlet]) -> dict[int, int]:
    return {ti: mi for mi, meshlet in enumerate(meshlets) for ti in meshlet.triangles}


def _is_connected_subset(adjacency: list[set[int]], triangles: list[int]) -> bool:
    if len(triangles) <= 1:
        return True
    allowed = set(triangles)
    stack = [triangles[0]]
    seen = {triangles[0]}
    while stack:
        ti = stack.pop()
        for tj in adjacency[ti]:
            if tj in allowed and tj not in seen:
                seen.add(tj)
                stack.append(tj)
    return len(seen) == len(allowed)


def _merge_meshlet_pass(
    mesh: ObjMesh,
    meshlets: list[Meshlet],
    tri_normals: np.ndarray,
    options: MeshletBuildOptions,
) -> tuple[list[Meshlet], bool]:
    tri_to_meshlet = _tri_to_meshlet(meshlets)
    mesh_adj = _meshlet_adjacency(mesh, meshlets, tri_to_meshlet)
    alive = [True] * len(meshlets)
    changed = False

    for mi in sorted(range(len(meshlets)), key=lambda k: len(meshlets[k].triangles)):
        if not alive[mi]:
            continue
        best = None
        for mj in sorted(mesh_adj[mi], key=lambda k: len(meshlets[k].triangles)):
            if not alive[mj] or mi == mj:
                continue
            merged = meshlets[mj].triangles + meshlets[mi].triangles
            candidate = _meshlet_from_tris_if_valid(mesh, merged, tri_normals, options, options.merge_max_normal_angle_deg)
            if candidate is None:
                continue
            # Prefer compact merges with fewer vertices per triangle.
            compatible_links = _compatible_meshlet_links(mesh, meshlets[mi], meshlets[mj])
            score = candidate.radius / mesh.scale_hint() + 0.01 * len(candidate.vertices) - options.compatible_merge_weight * compatible_links
            if best is None or score < best[0]:
                best = (score, mj, candidate)
        if best is None:
            continue
        _, mj, candidate = best
        meshlets[mj] = candidate
        alive[mi] = False
        changed = True
        for ti in candidate.triangles:
            tri_to_meshlet[ti] = mj

    if not changed:
        return meshlets, False
    return [m for i, m in enumerate(meshlets) if alive[i]], True


def _smooth_meshlet_pass(
    mesh: ObjMesh,
    meshlets: list[Meshlet],
    tri_normals: np.ndarray,
    options: MeshletBuildOptions,
) -> tuple[list[Meshlet], bool]:
    tri_adj = build_triangle_adjacency(mesh)
    tri_to_meshlet = _tri_to_meshlet(meshlets)
    scale = mesh.scale_hint()
    moved = False

    def local_degree(ti: int, mi: int) -> int:
        return sum(1 for tj in tri_adj[ti] if tri_to_meshlet.get(tj) == mi)

    candidates = sorted(tri_to_meshlet, key=lambda ti: (local_degree(ti, tri_to_meshlet[ti]), len(meshlets[tri_to_meshlet[ti]].triangles)))
    for ti in candidates:
        src = tri_to_meshlet.get(ti)
        if src is None or len(meshlets[src].triangles) <= 1:
            continue
        neighbor_meshlets = sorted({tri_to_meshlet[tj] for tj in tri_adj[ti] if tri_to_meshlet.get(tj) != src})
        if not neighbor_meshlets:
            continue

        src_without = [t for t in meshlets[src].triangles if t != ti]
        if not _is_connected_subset(tri_adj, src_without):
            continue

        old_src = meshlets[src]
        best = None
        for dst in neighbor_meshlets:
            dst_with = meshlets[dst].triangles + [ti]
            new_dst = _meshlet_from_tris_if_valid(mesh, dst_with, tri_normals, options)
            if new_dst is None:
                continue
            new_src = _meshlet_from_tris_if_valid(mesh, src_without, tri_normals, options)
            if new_src is None:
                continue
            old_degree = local_degree(ti, src)
            new_degree = local_degree(ti, dst)
            if new_degree < old_degree:
                continue
            old_score = old_src.radius + meshlets[dst].radius
            new_score = new_src.radius + new_dst.radius
            improvement = (new_degree - old_degree) + 0.25 * ((old_score - new_score) / scale)
            if improvement <= 0.0:
                continue
            if best is None or improvement > best[0]:
                best = (improvement, dst, new_src, new_dst)
        if best is None:
            continue
        _, dst, new_src, new_dst = best
        meshlets[src] = new_src
        meshlets[dst] = new_dst
        tri_to_meshlet[ti] = dst
        moved = True

    if not moved:
        return meshlets, False
    return [m for m in meshlets if len(m.triangles) > 0], True


def meshlet_stats(meshlets: list[Meshlet]) -> dict[str, float]:
    if not meshlets:
        return {}
    tri_counts = np.asarray([len(m.triangles) for m in meshlets], dtype=np.float64)
    vertex_counts = np.asarray([len(m.vertices) for m in meshlets], dtype=np.float64)
    radii = np.asarray([m.radius for m in meshlets], dtype=np.float64)
    angles = np.asarray([m.normal_angle_deg() for m in meshlets], dtype=np.float64)
    cull_angles = np.asarray([m.cull_angle_deg() for m in meshlets], dtype=np.float64)
    cull_empty = np.asarray([m.cull_cos > 1.0 for m in meshlets], dtype=bool)
    return {
        "meshlets": float(len(meshlets)),
        "triangles_min": float(np.min(tri_counts)),
        "triangles_avg": float(np.mean(tri_counts)),
        "triangles_max": float(np.max(tri_counts)),
        "vertices_min": float(np.min(vertex_counts)),
        "vertices_avg": float(np.mean(vertex_counts)),
        "vertices_max": float(np.max(vertex_counts)),
        "radius_avg": float(np.mean(radii)),
        "normal_angle_avg": float(np.mean(angles)),
        "normal_angle_max": float(np.max(angles)),
        "cull_angle_avg": float(np.mean(cull_angles)),
        "cull_angle_max": float(np.max(cull_angles)),
        "cull_angle_min": float(np.min(cull_angles)),
        "cull_empty": float(np.sum(cull_empty)),
    }
