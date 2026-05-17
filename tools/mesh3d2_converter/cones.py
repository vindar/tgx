from __future__ import annotations

from dataclasses import replace

import numpy as np

from .mesh import Meshlet
from .meshlets import _normal_cone


def auto_visibility_margin_deg(samples: int, safety: float = 2.0, extra_deg: float = 2.0) -> float:
    """Estimate the angular margin needed around a finite set of view samples.

    The base angle is the radius of a spherical cap whose area is the average
    area represented by one sample on the unit sphere. The safety factor covers
    the fact that Fibonacci samples are not a perfect covering, and extra_deg
    leaves a small allowance for raster/pixel and floating point effects.
    """
    if samples <= 1:
        return 180.0
    cap_cos = 1.0 - 2.0 / float(samples)
    base = float(np.degrees(np.arccos(np.clip(cap_cos, -1.0, 1.0))))
    return safety * base + extra_deg


def direction_cone(directions: np.ndarray) -> tuple[np.ndarray, float]:
    directions = np.asarray(directions, dtype=np.float64)
    if len(directions) == 0:
        return np.array([0.0, 0.0, 1.0], dtype=np.float64), -1.0
    return _normal_cone(directions)


def expand_cone_cos(cos_angle: float, margin_deg: float) -> float:
    angle = float(np.arccos(np.clip(cos_angle, -1.0, 1.0)))
    angle = min(np.pi, angle + np.radians(max(0.0, margin_deg)))
    return float(np.cos(angle))


def apply_visibility_cones(
    meshlets: list[Meshlet],
    views: np.ndarray,
    visibility: np.ndarray,
    *,
    margin_deg: float = 12.0,
) -> list[Meshlet]:
    if visibility.shape != (len(views), len(meshlets)):
        raise ValueError(f"visibility shape mismatch: got {visibility.shape}, expected {(len(views), len(meshlets))}")

    updated: list[Meshlet] = []
    for mi, meshlet in enumerate(meshlets):
        # The TGX helper renders along camera->object view directions. Meshlete and
        # EmberGL store visibility cones in the opposite convention: object->camera.
        visible_camera_dirs = -views[visibility[:, mi] != 0]
        if len(visible_camera_dirs) == 0:
            # Keep the meshlet conservative. Fully invisible/internal meshlets can be
            # diagnosed later, but the converter must not discard them automatically.
            updated.append(
                replace(
                    meshlet,
                    visibility_axis=np.array([0.0, 0.0, 1.0], dtype=np.float64),
                    visibility_cos=-1.0,
                    visibility_cull_axis=meshlet.cull_axis,
                    visibility_cull_cos=2.0,
                    visibility_samples=0,
                )
            )
            continue
        axis, cos_angle = direction_cone(visible_camera_dirs)
        cos_angle = expand_cone_cos(cos_angle, margin_deg)
        updated.append(
            replace(
                meshlet,
                visibility_axis=axis,
                visibility_cos=cos_angle,
                visibility_cull_axis=-axis,
                visibility_cull_cos=-cos_angle,
                visibility_samples=len(visible_camera_dirs),
            )
        )
    return updated
