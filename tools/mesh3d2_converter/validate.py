from __future__ import annotations

import numpy as np

from .mesh import Meshlet, ObjMesh


def validate_mesh_for_export(mesh: ObjMesh, meshlets: list[Meshlet]) -> None:
    """Validate the final preprocessed mesh and meshlets before C++ header export."""
    if len(mesh.triangles) > 0xFFFF:
        raise ValueError(f"Mesh3D2 supports at most 65535 triangles, got {len(mesh.triangles)}")
    if len(meshlets) > 0xFFFF:
        raise ValueError(f"Mesh3D2 supports at most 65535 meshlets, got {len(meshlets)}")

    materials = {m.material for m in meshlets}
    if len(materials) > 0x100:
        raise ValueError(f"Mesh3D2 supports at most 256 materials, got {len(materials)}")

    _validate_array(mesh.vertices, "vertices", 3)
    _validate_array(mesh.texcoords, "texcoords", 2)
    _validate_array(mesh.normals, "normals", 3)

    if len(mesh.normals) > 0:
        norms = np.linalg.norm(mesh.normals, axis=1)
        bad = np.where((norms < 0.9999) | (norms > 1.0001))[0]
        if len(bad):
            i = int(bad[0])
            raise ValueError(f"normal {i} is not normalized: norm={float(norms[i]):.9g}")

    has_texcoords = mesh.has_texcoords()
    has_normals = mesh.has_normals()

    for ti, tri in enumerate(mesh.triangles):
        for ci, corner in enumerate(tri.corners):
            if corner.v < 0 or corner.v >= len(mesh.vertices):
                raise ValueError(f"triangle {ti} corner {ci} has vertex index out of range: {corner.v}")
            if has_texcoords and (corner.vt < 0 or corner.vt >= len(mesh.texcoords)):
                raise ValueError(f"triangle {ti} corner {ci} has texcoord index out of range: {corner.vt}")
            if (not has_texcoords) and corner.vt != -1:
                raise ValueError(f"triangle {ti} corner {ci} has an unexpected texcoord index: {corner.vt}")
            if has_normals and (corner.vn < 0 or corner.vn >= len(mesh.normals)):
                raise ValueError(f"triangle {ti} corner {ci} has normal index out of range: {corner.vn}")
            if (not has_normals) and corner.vn != -1:
                raise ValueError(f"triangle {ti} corner {ci} has an unexpected normal index: {corner.vn}")

    assigned = set()
    for mi, meshlet in enumerate(meshlets):
        if not meshlet.triangles:
            raise ValueError(f"meshlet {mi} is empty")
        if len(meshlet.vertices) > 128:
            raise ValueError(f"meshlet {mi} has {len(meshlet.vertices)} vertices, max is 128")
        if len(meshlet.texcoords) > 255:
            raise ValueError(f"meshlet {mi} has {len(meshlet.texcoords)} texcoords, max is 255")
        if len(meshlet.normals) > 255:
            raise ValueError(f"meshlet {mi} has {len(meshlet.normals)} normals, max is 255")
        if not np.all(np.isfinite(meshlet.center)):
            raise ValueError(f"meshlet {mi} has a non-finite sphere center")
        if not np.isfinite(meshlet.radius) or meshlet.radius < 0.0:
            raise ValueError(f"meshlet {mi} has an invalid sphere radius: {meshlet.radius}")
        _validate_unit_vector(meshlet.normal_axis, f"meshlet {mi} normal_axis", allow_none=False)
        _validate_unit_vector(meshlet.cull_axis, f"meshlet {mi} cull_axis", allow_none=False)
        if meshlet.visibility_axis is not None:
            _validate_unit_vector(meshlet.visibility_axis, f"meshlet {mi} visibility_axis", allow_none=False)
            if not np.isfinite(meshlet.visibility_cos) or meshlet.visibility_cos < -1.0001 or meshlet.visibility_cos > 1.0001:
                raise ValueError(f"meshlet {mi} has invalid visibility cosine: {meshlet.visibility_cos}")

        for ti in meshlet.triangles:
            if ti < 0 or ti >= len(mesh.triangles):
                raise ValueError(f"meshlet {mi} references triangle out of range: {ti}")
            if ti in assigned:
                raise ValueError(f"triangle {ti} is assigned to more than one meshlet")
            assigned.add(ti)

    if len(assigned) != len(mesh.triangles):
        raise ValueError(f"{len(mesh.triangles) - len(assigned)} triangles are not assigned to a meshlet")


def _validate_array(values: np.ndarray, name: str, width: int) -> None:
    if values.ndim != 2 or values.shape[1] != width:
        raise ValueError(f"{name} array must have shape (n,{width}), got {values.shape}")
    if len(values) and not np.all(np.isfinite(values)):
        raise ValueError(f"{name} array contains NaN or Inf")


def _validate_unit_vector(vec, name: str, *, allow_none: bool) -> None:
    if vec is None:
        if allow_none:
            return
        raise ValueError(f"{name} is missing")
    if not np.all(np.isfinite(vec)):
        raise ValueError(f"{name} contains NaN or Inf")
    norm = float(np.linalg.norm(vec))
    if norm < 0.9999 or norm > 1.0001:
        raise ValueError(f"{name} is not normalized: norm={norm:.9g}")
