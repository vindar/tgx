from __future__ import annotations

import numpy as np

from .mesh import Meshlet


def fibonacci_sphere(samples: int) -> np.ndarray:
    if samples <= 0:
        return np.zeros((0, 3), dtype=np.float64)
    points = np.zeros((samples, 3), dtype=np.float64)
    inc = np.pi * (3.0 - np.sqrt(5.0))
    offset = 2.0 / samples
    for k in range(samples):
        y = k * offset - 1.0 + offset * 0.5
        r = np.sqrt(max(0.0, 1.0 - y * y))
        theta = k * inc
        points[k] = (np.cos(theta) * r, y, np.sin(theta) * r)
    return points


def meshlet_cull_ratios(
    meshlets: list[Meshlet],
    samples: int = 128,
    meshlet_cost: float = 0.0,
    cone_source: str = "normal",
) -> tuple[np.ndarray, np.ndarray]:
    """Return gross and net fractions of triangles discarded by meshlet cone culling.

    The net ratio subtracts a fixed per-meshlet runtime cost expressed in triangle equivalents.
    This keeps the metric honest when comparing a few large meshlets against many tiny meshlets.
    """
    if not meshlets:
        z = np.zeros((0,), dtype=np.float64)
        return z, z
    total_triangles = sum(len(m.triangles) for m in meshlets)
    if total_triangles == 0:
        z = np.zeros((0,), dtype=np.float64)
        return z, z

    directions = fibonacci_sphere(samples)
    gross_ratios = np.zeros((samples,), dtype=np.float64)
    net_ratios = np.zeros((samples,), dtype=np.float64)
    overhead = meshlet_cost * len(meshlets)
    for di, view_dir in enumerate(directions):
        culled = 0
        for meshlet in meshlets:
            if _is_culled_orthographic(meshlet, view_dir, cone_source):
                culled += len(meshlet.triangles)
        gross_ratios[di] = culled / total_triangles
        net_ratios[di] = (culled - overhead) / total_triangles
    return gross_ratios, net_ratios


def cull_ratio_stats(meshlets: list[Meshlet], samples: int = 128, meshlet_cost: float = 2.0, cone_source: str = "normal") -> dict[str, float]:
    gross, net = meshlet_cull_ratios(meshlets, samples, meshlet_cost, cone_source)
    if len(gross) == 0:
        return {
            "samples": float(samples),
            "meshlet_cost": float(meshlet_cost),
            "gross_mean": 0.0,
            "gross_min": 0.0,
            "gross_max": 0.0,
            "gross_p10": 0.0,
            "gross_p50": 0.0,
            "gross_p90": 0.0,
            "net_mean": 0.0,
            "net_min": 0.0,
            "net_max": 0.0,
            "net_p10": 0.0,
            "net_p50": 0.0,
            "net_p90": 0.0,
        }
    return {
        "samples": float(samples),
        "meshlet_cost": float(meshlet_cost),
        "cone_source": cone_source,
        "gross_mean": float(np.mean(gross)),
        "gross_min": float(np.min(gross)),
        "gross_max": float(np.max(gross)),
        "gross_p10": float(np.percentile(gross, 10)),
        "gross_p50": float(np.percentile(gross, 50)),
        "gross_p90": float(np.percentile(gross, 90)),
        "net_mean": float(np.mean(net)),
        "net_min": float(np.min(net)),
        "net_max": float(np.max(net)),
        "net_p10": float(np.percentile(net, 10)),
        "net_p50": float(np.percentile(net, 50)),
        "net_p90": float(np.percentile(net, 90)),
    }


def _is_culled_orthographic(meshlet: Meshlet, view_dir: np.ndarray, cone_source: str) -> bool:
    if cone_source == "normal":
        return meshlet.cull_cos <= 1.0 and float(np.dot(view_dir, meshlet.cull_axis)) >= meshlet.cull_cos
    if cone_source == "visibility":
        if meshlet.visibility_axis is None:
            return False
        # view_dir is camera->object; the visibility cone stores object->camera.
        camera_dir = -view_dir
        return float(np.dot(meshlet.visibility_axis, camera_dir)) < meshlet.visibility_cos
    raise ValueError(f"unknown cull cone source: {cone_source}")
