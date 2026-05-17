from __future__ import annotations

from dataclasses import dataclass, field
from pathlib import Path
from typing import Iterable

import numpy as np


MISSING_INDEX = -1


@dataclass(frozen=True)
class FaceVertex:
    v: int
    vt: int = MISSING_INDEX
    vn: int = MISSING_INDEX


@dataclass
class Triangle:
    corners: tuple[FaceVertex, FaceVertex, FaceVertex]
    material: str = ""
    group: str = ""


@dataclass
class Material:
    name: str
    diffuse: tuple[float, float, float] = (0.75, 0.75, 0.75)
    texture_path: Path | None = None
    ambiant_strength: float = 0.1
    diffuse_strength: float = 0.7
    specular_strength: float = 0.6
    specular_exponent: int = 32


@dataclass
class ObjMesh:
    vertices: np.ndarray
    texcoords: np.ndarray
    normals: np.ndarray
    triangles: list[Triangle]
    name: str = ""
    materials: dict[str, Material] = field(default_factory=dict)
    source_path: Path | None = None

    def has_texcoords(self) -> bool:
        return self.texcoords.size != 0 and all(c.vt >= 0 for t in self.triangles for c in t.corners)

    def has_normals(self) -> bool:
        return self.normals.size != 0 and all(c.vn >= 0 for t in self.triangles for c in t.corners)

    def triangle_vertex_indices(self, tri_index: int) -> tuple[int, int, int]:
        return tuple(c.v for c in self.triangles[tri_index].corners)

    def triangle_vertices(self, tri_index: int) -> np.ndarray:
        return self.vertices[list(self.triangle_vertex_indices(tri_index))]

    def bounding_box(self) -> tuple[np.ndarray, np.ndarray]:
        if len(self.vertices) == 0:
            z = np.zeros(3, dtype=np.float64)
            return z, z
        return np.min(self.vertices, axis=0), np.max(self.vertices, axis=0)

    def scale_hint(self) -> float:
        bb_min, bb_max = self.bounding_box()
        d = float(np.linalg.norm(bb_max - bb_min))
        return d if d > 0.0 else 1.0


@dataclass
class Meshlet:
    triangles: list[int]
    material: str
    vertices: set[int]
    texcoords: set[int]
    normals: set[int]
    center: np.ndarray
    radius: float
    normal_axis: np.ndarray
    normal_min_dot: float
    cull_axis: np.ndarray
    cull_cos: float
    visibility_axis: np.ndarray | None = None
    visibility_cos: float = -1.0
    visibility_cull_axis: np.ndarray | None = None
    visibility_cull_cos: float = 2.0
    visibility_samples: int = 0

    def normal_angle_deg(self) -> float:
        return float(np.degrees(np.arccos(np.clip(self.normal_min_dot, -1.0, 1.0))))

    def cull_angle_deg(self) -> float:
        if self.cull_cos > 1.0:
            return 0.0
        return float(np.degrees(np.arccos(np.clip(self.cull_cos, -1.0, 1.0))))

    def visibility_angle_deg(self) -> float:
        return float(np.degrees(np.arccos(np.clip(self.visibility_cos, -1.0, 1.0))))

    def selected_cull_cone(self, source: str) -> tuple[np.ndarray, float]:
        if source == "normal":
            return self.cull_axis, self.cull_cos
        if source == "visibility":
            if self.visibility_cull_axis is None:
                return self.cull_axis, self.cull_cos
            return self.visibility_cull_axis, self.visibility_cull_cos
        raise ValueError(f"unknown cull cone source: {source}")


def compute_triangle_normals(mesh: ObjMesh) -> np.ndarray:
    normals = np.zeros((len(mesh.triangles), 3), dtype=np.float64)
    for i in range(len(mesh.triangles)):
        p = mesh.triangle_vertices(i)
        n = np.cross(p[1] - p[0], p[2] - p[0])
        length = float(np.linalg.norm(n))
        if length > 0.0:
            n /= length
        normals[i] = n
    return normals


def compute_triangle_centers(mesh: ObjMesh) -> np.ndarray:
    centers = np.zeros((len(mesh.triangles), 3), dtype=np.float64)
    for i in range(len(mesh.triangles)):
        centers[i] = np.mean(mesh.triangle_vertices(i), axis=0)
    return centers


def unique_valid(values: Iterable[int]) -> set[int]:
    return {v for v in values if v >= 0}
