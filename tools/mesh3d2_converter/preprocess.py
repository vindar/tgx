from __future__ import annotations

from dataclasses import dataclass

import numpy as np

from .mesh import FaceVertex, ObjMesh, Triangle


@dataclass
class PreprocessStats:
    vertices_before: int
    vertices_after: int
    texcoords_before: int
    texcoords_after: int
    normals_before: int
    normals_after: int
    normals_generated: bool
    normalized: bool
    degenerate_triangles_removed: int = 0


def _dedupe_array(values: np.ndarray, eps: float) -> tuple[np.ndarray, list[int]]:
    if len(values) == 0:
        return values.copy(), []
    scale = 1.0 / eps if eps > 0.0 else 1e12
    remap: list[int] = []
    unique: list[np.ndarray] = []
    seen: dict[tuple[int, ...], int] = {}
    for row in values:
        key = tuple(int(round(float(x) * scale)) for x in row)
        index = seen.get(key)
        if index is None:
            index = len(unique)
            seen[key] = index
            unique.append(row.copy())
        remap.append(index)
    return np.asarray(unique, dtype=np.float64).reshape((-1, values.shape[1])), remap


def _normalize_normals(normals: np.ndarray) -> np.ndarray:
    if len(normals) == 0:
        return normals.copy()
    out = normals.astype(np.float64, copy=True)
    lengths = np.linalg.norm(out, axis=1)
    valid = lengths > 1e-20
    out[valid] /= lengths[valid, None]
    out[~valid] = np.asarray([0.0, 0.0, 1.0], dtype=np.float64)
    return out


def _generate_smooth_normals(mesh: ObjMesh) -> tuple[np.ndarray, list[Triangle]]:
    normals = np.zeros_like(mesh.vertices, dtype=np.float64)
    for tri in mesh.triangles:
        ids = [c.v for c in tri.corners]
        p0, p1, p2 = mesh.vertices[ids]
        normal = np.cross(p1 - p0, p2 - p0)
        for vi in ids:
            normals[vi] += normal
    normals = _normalize_normals(normals)

    triangles: list[Triangle] = []
    for tri in mesh.triangles:
        corners = tuple(FaceVertex(c.v, c.vt, c.v) for c in tri.corners)
        triangles.append(Triangle(corners, tri.material, tri.group))
    return normals, triangles


def _remove_degenerate_triangles(vertices: np.ndarray, triangles: list[Triangle], eps: float = 1e-24) -> tuple[list[Triangle], int]:
    kept: list[Triangle] = []
    removed = 0
    for tri in triangles:
        ids = [c.v for c in tri.corners]
        p0, p1, p2 = vertices[ids]
        n = np.cross(p1 - p0, p2 - p0)
        if float(np.dot(n, n)) <= eps:
            removed += 1
        else:
            kept.append(tri)
    if not kept:
        raise ValueError("all triangles are degenerate")
    return kept, removed


def preprocess_mesh(
    mesh: ObjMesh,
    *,
    normalize: bool = False,
    normalize_size: float = 2.0,
    dedupe_epsilon: float = 1e-9,
    force_normals: bool = False,
) -> tuple[ObjMesh, PreprocessStats]:
    """Return a cleaned mesh suitable for Mesh3D2 export."""
    vertices_before = len(mesh.vertices)
    texcoords_before = len(mesh.texcoords)
    normals_before = len(mesh.normals)

    vertices = mesh.vertices.astype(np.float64, copy=True)
    texcoords = mesh.texcoords.astype(np.float64, copy=True)
    normals = _normalize_normals(mesh.normals)
    triangles = [Triangle(t.corners, t.material, t.group) for t in mesh.triangles]

    if normalize:
        bb_min = np.min(vertices, axis=0)
        bb_max = np.max(vertices, axis=0)
        center = (bb_min + bb_max) * 0.5
        extent = float(np.max(bb_max - bb_min))
        if extent > 0.0:
            vertices = (vertices - center) * (float(normalize_size) / extent)

    vertices, v_remap = _dedupe_array(vertices, dedupe_epsilon)
    texcoords, vt_remap = _dedupe_array(texcoords, dedupe_epsilon)
    normals, vn_remap = _dedupe_array(normals, dedupe_epsilon)

    remapped: list[Triangle] = []
    for tri in triangles:
        new_corners = []
        for c in tri.corners:
            vt = vt_remap[c.vt] if c.vt >= 0 and vt_remap else c.vt
            vn = vn_remap[c.vn] if c.vn >= 0 and vn_remap else c.vn
            new_corners.append(FaceVertex(v_remap[c.v], vt, vn))
        remapped.append(Triangle(tuple(new_corners), tri.material, tri.group))
    triangles = remapped
    triangles, degenerate_removed = _remove_degenerate_triangles(vertices, triangles)

    tmp = ObjMesh(vertices, texcoords, normals, triangles, mesh.name, mesh.materials.copy(), mesh.source_path)
    if not tmp.has_texcoords():
        texcoords = np.zeros((0, 2), dtype=np.float64)
        triangles = [
            Triangle(tuple(FaceVertex(c.v, -1, c.vn) for c in tri.corners), tri.material, tri.group)
            for tri in triangles
        ]
        tmp = ObjMesh(vertices, texcoords, normals, triangles, mesh.name, mesh.materials.copy(), mesh.source_path)

    normals_generated = False
    if force_normals or not tmp.has_normals():
        normals, triangles = _generate_smooth_normals(tmp)
        normals_generated = True
        normals, vn_remap = _dedupe_array(normals, dedupe_epsilon)
        remapped = []
        for tri in triangles:
            remapped.append(Triangle(tuple(FaceVertex(c.v, c.vt, vn_remap[c.vn]) for c in tri.corners), tri.material, tri.group))
        triangles = remapped

    cleaned = ObjMesh(vertices, texcoords, normals, triangles, mesh.name, mesh.materials.copy(), mesh.source_path)
    stats = PreprocessStats(
        vertices_before=vertices_before,
        vertices_after=len(cleaned.vertices),
        texcoords_before=texcoords_before,
        texcoords_after=len(cleaned.texcoords),
        normals_before=normals_before,
        normals_after=len(cleaned.normals),
        normals_generated=normals_generated,
        normalized=normalize,
        degenerate_triangles_removed=degenerate_removed,
    )
    return cleaned, stats
