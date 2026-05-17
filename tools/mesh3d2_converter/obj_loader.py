from __future__ import annotations

from pathlib import Path

import numpy as np

from .mesh import FaceVertex, Material, ObjMesh, Triangle


def _parse_index(token: str, current_count: int, line_no: int) -> int:
    try:
        value = int(token)
    except ValueError as exc:
        raise ValueError(f"invalid OBJ index {token!r} at line {line_no}") from exc
    if value == 0:
        raise ValueError(f"OBJ indices are 1-based; got 0 at line {line_no}")
    return value - 1 if value > 0 else current_count + value


def _parse_face_vertex(token: str, counts: tuple[int, int, int], line_no: int) -> FaceVertex:
    parts = token.split("/")
    if not 1 <= len(parts) <= 3:
        raise ValueError(f"invalid face token {token!r} at line {line_no}")
    parts += [""] * (3 - len(parts))

    v = _parse_index(parts[0], counts[0], line_no) if parts[0] else -1
    vt = _parse_index(parts[1], counts[1], line_no) if parts[1] else -1
    vn = _parse_index(parts[2], counts[2], line_no) if parts[2] else -1
    if v < 0 or v >= counts[0]:
        raise ValueError(f"vertex index out of range at line {line_no}")
    if vt >= counts[1]:
        raise ValueError(f"texture coordinate index out of range at line {line_no}")
    if vn >= counts[2]:
        raise ValueError(f"normal index out of range at line {line_no}")
    return FaceVertex(v, vt, vn)


def _triangulate(face: list[FaceVertex]) -> list[tuple[FaceVertex, FaceVertex, FaceVertex]]:
    if len(face) < 3:
        return []
    return [(face[0], face[i], face[i + 1]) for i in range(1, len(face) - 1)]


def _parse_mtl(path: Path) -> dict[str, Material]:
    materials: dict[str, Material] = {}
    current: Material | None = None
    if not path.exists():
        return materials

    with path.open("r", encoding="utf-8", errors="replace") as f:
        for raw in f:
            line = raw.split("#", 1)[0].strip()
            if not line:
                continue
            parts = line.split()
            tag = parts[0].lower()
            if tag == "newmtl":
                name = " ".join(parts[1:]) if len(parts) > 1 else ""
                current = Material(name)
                materials[name] = current
            elif current is not None and tag == "kd" and len(parts) >= 4:
                current.diffuse = (float(parts[1]), float(parts[2]), float(parts[3]))
            elif current is not None and tag == "map_kd" and len(parts) >= 2:
                # OBJ exporters sometimes put options before the texture path. Keep the last
                # token, which is the common and most robust interpretation for simple MTL files.
                current.texture_path = (path.parent / parts[-1]).resolve()
    return materials


def load_obj(path: str | Path) -> ObjMesh:
    """Load a Wavefront OBJ file into the small internal Mesh3D2 converter model."""
    path = Path(path)
    vertices: list[tuple[float, float, float]] = []
    texcoords: list[tuple[float, float]] = []
    normals: list[tuple[float, float, float]] = []
    triangles: list[Triangle] = []
    mtllibs: list[str] = []

    current_group = ""
    current_material = ""

    with path.open("r", encoding="utf-8", errors="replace") as f:
        for line_no, raw in enumerate(f, start=1):
            line = raw.split("#", 1)[0].strip()
            if not line:
                continue
            parts = line.split()
            tag = parts[0]

            if tag == "v":
                if len(parts) < 4:
                    raise ValueError(f"vertex needs 3 components at line {line_no}")
                vertices.append(tuple(float(x) for x in parts[1:4]))
            elif tag == "vt":
                if len(parts) < 3:
                    raise ValueError(f"texture coordinate needs 2 components at line {line_no}")
                texcoords.append(tuple(float(x) for x in parts[1:3]))
            elif tag == "vn":
                if len(parts) < 4:
                    raise ValueError(f"normal needs 3 components at line {line_no}")
                normals.append(tuple(float(x) for x in parts[1:4]))
            elif tag in ("o", "g"):
                current_group = " ".join(parts[1:]) if len(parts) > 1 else ""
            elif tag == "mtllib":
                mtllibs.extend(parts[1:])
            elif tag == "usemtl":
                current_material = " ".join(parts[1:]) if len(parts) > 1 else ""
            elif tag == "f":
                counts = (len(vertices), len(texcoords), len(normals))
                face = [_parse_face_vertex(tok, counts, line_no) for tok in parts[1:]]
                for tri in _triangulate(face):
                    triangles.append(Triangle(tri, current_material, current_group))

    if not vertices:
        raise ValueError(f"{path} does not contain vertices")
    if not triangles:
        raise ValueError(f"{path} does not contain faces")

    materials: dict[str, Material] = {}
    for item in mtllibs:
        materials.update(_parse_mtl(path.parent / item))
    for tri in triangles:
        if tri.material and tri.material not in materials:
            materials[tri.material] = Material(tri.material)
    if "" in {t.material for t in triangles} and "" not in materials:
        materials[""] = Material("")

    return ObjMesh(
        vertices=np.asarray(vertices, dtype=np.float64),
        texcoords=np.asarray(texcoords, dtype=np.float64).reshape((-1, 2)) if texcoords else np.zeros((0, 2), dtype=np.float64),
        normals=np.asarray(normals, dtype=np.float64).reshape((-1, 3)) if normals else np.zeros((0, 3), dtype=np.float64),
        triangles=triangles,
        name=path.stem,
        materials=materials,
        source_path=path,
    )
