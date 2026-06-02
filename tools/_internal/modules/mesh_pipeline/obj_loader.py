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


def _float3(parts: list[str]) -> tuple[float, float, float]:
    return float(parts[1]), float(parts[2]), float(parts[3])


def _mean3(values: tuple[float, float, float]) -> float:
    return max(0.0, min(1.0, float(sum(values) / 3.0)))


def _texture_token(parts: list[str]) -> str:
    """Return the texture path part of an MTL map line.

    MTL map statements may contain options before the file name, for example
    `map_Kd -s 1 1 1 diffuse texture.png`. We skip common options and then keep
    the remaining tokens as the path so file names with spaces still work.
    """
    tokens = parts[1:]
    i = 0
    option_args = {
        "-blendu": 1,
        "-blendv": 1,
        "-boost": 1,
        "-mm": 2,
        "-o": 3,
        "-s": 3,
        "-t": 3,
        "-texres": 1,
        "-clamp": 1,
        "-bm": 1,
        "-type": 1,
    }
    while i < len(tokens):
        token = tokens[i]
        if not token.startswith("-"):
            return " ".join(tokens[i:])
        option = token.lower()
        i += 1
        count = option_args.get(option, 0)
        for _ in range(count):
            if i >= len(tokens) or tokens[i].startswith("-"):
                break
            i += 1
    return tokens[-1] if tokens else ""


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
            tag = parts[0].lstrip("\ufeff").lower()
            if tag == "newmtl":
                name = " ".join(parts[1:]) if len(parts) > 1 else ""
                current = Material(name)
                materials[name] = current
            elif current is not None and tag == "kd" and len(parts) >= 4:
                current.diffuse = _float3(parts)
            elif current is not None and tag == "ke" and len(parts) >= 4:
                current.emissive = _float3(parts)
                current.emissive_strength = 1.0
                current.material_extra_present = True
            elif current is not None and tag == "ka" and len(parts) >= 4:
                current.ambiant_strength = _mean3(_float3(parts))
            elif current is not None and tag == "ks" and len(parts) >= 4:
                current.specular_strength = _mean3(_float3(parts))
            elif current is not None and tag == "ns" and len(parts) >= 2:
                current.specular_exponent = max(1, min(128, int(round(float(parts[1])))))
            elif current is not None and (tag.startswith("map_") or tag in ("bump", "disp", "decal", "refl")) and len(parts) >= 2:
                texture_path = (path.parent / _texture_token(parts)).resolve()
                current.texture_refs[tag] = texture_path
                if tag == "map_kd":
                    current.texture_path = texture_path
                elif tag == "map_ke":
                    current.emissive_texture_path = texture_path
                    current.emissive_strength = 1.0
    return materials


def load_obj(path: str | Path) -> ObjMesh:
    """Load a Wavefront OBJ file into the small internal meshlet converter model."""
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
            tag = parts[0].lstrip("\ufeff")

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
