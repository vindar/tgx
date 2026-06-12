from __future__ import annotations

import math
from dataclasses import dataclass
from typing import Iterable, Sequence

import numpy as np


# 65535 = 3 * 5 * 17 * 257.  Using only these divisors makes every local
# vertex lattice a sub-lattice of E / (65535 * 32767), and keeps the current
# Mesh3Dv2 runtime decode convention unchanged.
DIVISORS_65535: tuple[int, ...] = (
    1,
    3,
    5,
    15,
    17,
    51,
    85,
    255,
    257,
    771,
    1285,
    3855,
    4369,
    13107,
    21845,
    65535,
)


@dataclass
class Mesh3Dv2WatertightOptions:
    enabled: bool = True
    report: bool = False
    fail_threshold: float | None = None
    max_iterations: int = 12
    bbox_padding: float = 2.0e-4


@dataclass
class Mesh3Dv2WatertightMetadata:
    q_center: list[int]
    q_radius: int
    decoded_center: np.ndarray
    decoded_radius: float


@dataclass
class Mesh3Dv2WatertightStats:
    checked: bool = False
    enabled: bool = False
    iterations: int = 0
    shared_vertices: int = 0
    shared_occurrences: int = 0
    shared_vertex_decode_max_delta: float = 0.0
    shared_vertex_decode_rms_delta: float = 0.0
    shared_vertex_decode_float32_max_delta: float = 0.0
    shared_vertex_decode_float32_rms_delta: float = 0.0
    shared_vertex_decode_bad_count: int = 0
    worst_shared_vertex: int = -1
    worst_meshlet_pair: tuple[int, int] = (-1, -1)
    max_payload_decode_error: float = 0.0
    max_payload_decode_error_float32: float = 0.0
    max_geometry_displacement: float = 0.0
    rms_geometry_displacement: float = 0.0
    avg_radius_inflation: float = 1.0
    max_radius_inflation: float = 1.0
    bbox_extent: float = 1.0
    threshold: float = 0.0


@dataclass
class Mesh3Dv2WatertightPlan:
    positions: np.ndarray
    metadata: list[Mesh3Dv2WatertightMetadata]
    bbox_min: np.ndarray
    bbox_max: np.ndarray
    bbox_center: np.ndarray
    bbox_extent: float
    center_scale: float
    radius_scale: float
    snorm_scale: float
    stats: Mesh3Dv2WatertightStats


def _clamp_i16(value: int) -> int:
    return max(-32767, min(32767, int(value)))


def _clamp_i16_even(value: int) -> int:
    # Keep centers even so bbox_center + q_center * (E / 65534) lies on the
    # base lattice E / (65535 * 32767).  Use +/-32766 to keep the result even.
    return max(-32766, min(32766, int(value)))


def _clamp_u16(value: int) -> int:
    return max(0, min(65535, int(value)))


def _nearest_even(value: float) -> int:
    return _clamp_i16_even(2 * int(round(float(value) * 0.5)))


def _next_divisor_65535(q_needed: int) -> int:
    q_needed = max(1, int(q_needed))
    for divisor in DIVISORS_65535:
        if divisor >= q_needed:
            return divisor
    raise ValueError(
        "Mesh3Dv2 watertight export cannot encode a meshlet radius larger "
        "than the global extent"
    )


def _lcm_many(values: Iterable[int]) -> int:
    out = 1
    for value in values:
        out = math.lcm(out, int(value))
    return out


def mesh3dv2_quant_frame(mesh, *, bbox_padding: float = 0.0) -> tuple[np.ndarray, np.ndarray, np.ndarray, float, float, float, float]:
    """Return the bounding box/frame used by Mesh3Dv2 metadata quantization.

    The normal exporter uses the tight mesh bounding box.  Watertight export uses
    a small centered cube padding so the final snapped vertices remain inside the
    emitted bbox while the solver and runtime still share the exact same center
    and extent.
    """
    bb_min, bb_max = mesh.bounding_box()
    bb_min = np.asarray(bb_min, dtype=np.float64)
    bb_max = np.asarray(bb_max, dtype=np.float64)
    center = 0.5 * (bb_min + bb_max)
    extent = float(np.max(bb_max - bb_min))
    if (not math.isfinite(extent)) or extent <= 1.0e-20:
        extent = 1.0
    if bbox_padding > 0.0:
        extent *= 1.0 + float(bbox_padding)
        bb_min = center - 0.5 * extent
        bb_max = center + 0.5 * extent
    center_scale = extent / 65534.0
    radius_scale = extent / 65535.0
    snorm_scale = 1.0 / 32767.0
    return bb_min, bb_max, center, extent, center_scale, radius_scale, snorm_scale


def _meshlet_vertex_indices(encoded_item) -> list[int]:
    return [int(v) for v in encoded_item.vertices]


def _vertex_to_meshlets(encoded: Sequence) -> dict[int, list[int]]:
    mapping: dict[int, list[int]] = {}
    for meshlet_index, item in enumerate(encoded):
        for vertex_index in _meshlet_vertex_indices(item):
            mapping.setdefault(vertex_index, []).append(meshlet_index)
    return mapping


def _metadata_for_positions(
    mesh,
    meshlets: Sequence,
    encoded: Sequence,
    positions: np.ndarray,
    bbox_center: np.ndarray,
    center_scale: float,
    radius_scale: float,
    *,
    force_watertight_lattice: bool,
) -> list[Mesh3Dv2WatertightMetadata]:
    metadata: list[Mesh3Dv2WatertightMetadata] = []
    for meshlet, item in zip(meshlets, encoded):
        vertex_indices = _meshlet_vertex_indices(item)
        if vertex_indices:
            pts = positions[vertex_indices]
            ideal_center = 0.5 * (np.min(pts, axis=0) + np.max(pts, axis=0))
        else:
            ideal_center = np.asarray(meshlet.center, dtype=np.float64)

        if force_watertight_lattice:
            q_center = [_nearest_even(float((ideal_center[i] - bbox_center[i]) / center_scale)) for i in range(3)]
        else:
            q_center = [
                _clamp_i16(int(round(float((ideal_center[i] - bbox_center[i]) / center_scale))))
                for i in range(3)
            ]

        decoded_center = bbox_center + np.asarray(q_center, dtype=np.float64) * center_scale

        if vertex_indices:
            radius_needed = max(float(np.linalg.norm(positions[index] - decoded_center)) for index in vertex_indices)
        else:
            radius_needed = float(meshlet.radius)
        if not force_watertight_lattice:
            # Preserve the existing exporter convention for baseline stats.
            radius_needed = max(radius_needed, float(meshlet.radius), 1.0e-12)
        else:
            # In watertight mode the hard requirement is containment of the final
            # snapped payload vertices under the constrained decoded center.
            radius_needed = max(radius_needed, 1.0e-12)
        q_needed = _clamp_u16(int(math.ceil(radius_needed / radius_scale)))
        q_radius = _next_divisor_65535(q_needed) if force_watertight_lattice else max(1, q_needed)
        decoded_radius = max(float(q_radius) * radius_scale, 1.0e-12)
        metadata.append(Mesh3Dv2WatertightMetadata(q_center, q_radius, decoded_center, decoded_radius))
    return metadata


def _snap_positions_to_common_lattices(
    original_positions: np.ndarray,
    q_radii: Sequence[int],
    vertex_to_meshlets: dict[int, list[int]],
    bbox_center: np.ndarray,
    base_step: float,
) -> np.ndarray:
    snapped = original_positions.copy()
    for vertex_index, incident_meshlets in vertex_to_meshlets.items():
        if not incident_meshlets:
            continue
        lattice_multiplier = _lcm_many(q_radii[mi] for mi in incident_meshlets)
        step = float(lattice_multiplier) * base_step
        if step <= 0.0 or not math.isfinite(step):
            continue
        p = original_positions[vertex_index]
        snapped[vertex_index] = bbox_center + np.rint((p - bbox_center) / step) * step
    return snapped


def _geometry_displacement(original_positions: np.ndarray, positions: np.ndarray) -> tuple[float, float]:
    if len(original_positions) == 0:
        return 0.0, 0.0
    delta = positions - original_positions
    norms = np.linalg.norm(delta, axis=1)
    max_disp = float(np.max(norms)) if norms.size else 0.0
    rms_disp = float(np.sqrt(np.mean(norms * norms))) if norms.size else 0.0
    return max_disp, rms_disp


def _radius_inflation_stats(
    base_metadata: Sequence[Mesh3Dv2WatertightMetadata],
    watertight_metadata: Sequence[Mesh3Dv2WatertightMetadata],
) -> tuple[float, float]:
    if not watertight_metadata:
        return 1.0, 1.0
    ratios: list[float] = []
    for base, wt in zip(base_metadata, watertight_metadata):
        ratios.append(float(wt.q_radius) / max(1.0, float(base.q_radius)))
    return float(np.mean(ratios)), float(np.max(ratios))


def solve_mesh3dv2_watertight(
    mesh,
    meshlets: Sequence,
    encoded: Sequence,
    options: Mesh3Dv2WatertightOptions | None = None,
) -> Mesh3Dv2WatertightPlan:
    """Build a watertight Mesh3Dv2 export plan without changing the runtime format.

    The plan enforces:
      * even quantized meshlet centers;
      * meshlet radii promoted to divisors of 65535;
      * every payload vertex snapped to the common lattice of all meshlets that
        reference that source vertex.
    """
    options = options or Mesh3Dv2WatertightOptions(enabled=True)
    original_positions = np.asarray(mesh.vertices, dtype=np.float64)
    positions = original_positions.copy()

    (
        bbox_min,
        bbox_max,
        bbox_center,
        bbox_extent,
        center_scale,
        radius_scale,
        snorm_scale,
    ) = mesh3dv2_quant_frame(mesh, bbox_padding=options.bbox_padding)

    vertex_to_meshlets = _vertex_to_meshlets(encoded)
    base_step = bbox_extent / (65535.0 * 32767.0)

    base_metadata = _metadata_for_positions(
        mesh,
        meshlets,
        encoded,
        original_positions,
        bbox_center,
        center_scale,
        radius_scale,
        force_watertight_lattice=False,
    )

    metadata: list[Mesh3Dv2WatertightMetadata] = []
    last_q_centers: list[tuple[int, int, int]] | None = None
    last_q_radii: list[int] | None = None
    iterations = 0

    for iteration in range(max(1, int(options.max_iterations))):
        iterations = iteration + 1
        metadata = _metadata_for_positions(
            mesh,
            meshlets,
            encoded,
            positions,
            bbox_center,
            center_scale,
            radius_scale,
            force_watertight_lattice=True,
        )
        q_centers = [tuple(int(x) for x in item.q_center) for item in metadata]
        q_radii = [int(item.q_radius) for item in metadata]
        snapped = _snap_positions_to_common_lattices(
            original_positions,
            q_radii,
            vertex_to_meshlets,
            bbox_center,
            base_step,
        )

        stable = (
            last_q_centers == q_centers
            and last_q_radii == q_radii
            and np.array_equal(snapped, positions)
        )
        positions = snapped
        last_q_centers = q_centers
        last_q_radii = q_radii
        if stable:
            break
    else:
        raise ValueError("Mesh3Dv2 watertight solve did not converge")

    metadata = _metadata_for_positions(
        mesh,
        meshlets,
        encoded,
        positions,
        bbox_center,
        center_scale,
        radius_scale,
        force_watertight_lattice=True,
    )

    max_disp, rms_disp = _geometry_displacement(original_positions, positions)
    avg_inflation, max_inflation = _radius_inflation_stats(base_metadata, metadata)
    stats = Mesh3Dv2WatertightStats(
        checked=False,
        enabled=True,
        iterations=iterations,
        max_geometry_displacement=max_disp,
        rms_geometry_displacement=rms_disp,
        avg_radius_inflation=avg_inflation,
        max_radius_inflation=max_inflation,
        bbox_extent=bbox_extent,
    )

    return Mesh3Dv2WatertightPlan(
        positions=positions,
        metadata=metadata,
        bbox_min=bbox_min,
        bbox_max=bbox_max,
        bbox_center=bbox_center,
        bbox_extent=bbox_extent,
        center_scale=center_scale,
        radius_scale=radius_scale,
        snorm_scale=snorm_scale,
        stats=stats,
    )


def _payload_q_for_occurrence(position: np.ndarray, metadata: Mesh3Dv2WatertightMetadata) -> np.ndarray:
    """Return the exact int16 payload q that exporter.py will write.

    The float32 checker must reuse this q.  The TGX renderer does not recompute
    q from the source position; it only decodes the already-emitted int16 payload.
    """
    scale = 32767.0 / metadata.decoded_radius
    q = np.rint((position - metadata.decoded_center) * scale).astype(np.int64)
    return np.asarray([_clamp_i16(int(x)) for x in q], dtype=np.int64)


def _decode_occurrence64(position: np.ndarray, metadata: Mesh3Dv2WatertightMetadata) -> tuple[np.ndarray, float, np.ndarray]:
    q = _payload_q_for_occurrence(position, metadata)
    decoded = metadata.decoded_center + q.astype(np.float64) * (metadata.decoded_radius / 32767.0)
    error = float(np.max(np.abs(decoded - position)))
    return decoded, error, q


def _decode_occurrence32(position: np.ndarray, metadata: Mesh3Dv2WatertightMetadata, q: np.ndarray) -> tuple[np.ndarray, float]:
    c32 = np.asarray(metadata.decoded_center, dtype=np.float32)
    r32 = np.float32(metadata.decoded_radius)
    decoded = c32 + q.astype(np.float32) * (r32 * np.float32(1.0 / 32767.0))
    error = float(np.max(np.abs(decoded.astype(np.float64) - position)))
    return decoded.astype(np.float64), error


def _pairwise_max_delta(values: np.ndarray) -> tuple[float, tuple[int, int]]:
    if len(values) < 2:
        return 0.0, (-1, -1)
    best = 0.0
    best_pair = (0, 1)
    for i in range(len(values)):
        for j in range(i + 1, len(values)):
            delta = float(np.linalg.norm(values[i] - values[j]))
            if delta > best:
                best = delta
                best_pair = (i, j)
    return best, best_pair


def check_mesh3dv2_watertightness(
    mesh,
    meshlets: Sequence,
    encoded: Sequence,
    metadata: Sequence[Mesh3Dv2WatertightMetadata],
    positions: np.ndarray | None = None,
    *,
    enabled: bool = False,
    bbox_extent: float | None = None,
    threshold: float | None = None,
    raise_on_failure: bool = False,
) -> Mesh3Dv2WatertightStats:
    """Decode all local vertex occurrences and compare shared source vertices."""
    del meshlets  # only the encoded local vertex tables and metadata are needed here.
    positions = np.asarray(mesh.vertices if positions is None else positions, dtype=np.float64)
    if bbox_extent is None:
        _, _, _, bbox_extent, _, _, _ = mesh3dv2_quant_frame(mesh)
    effective_threshold = float(threshold) if threshold is not None else max(1.0e-12, 1.0e-7 * float(bbox_extent))

    by_vertex64: dict[int, list[tuple[int, np.ndarray]]] = {}
    by_vertex32: dict[int, list[tuple[int, np.ndarray]]] = {}
    max_payload_error64 = 0.0
    max_payload_error32 = 0.0

    for meshlet_index, (item, meta) in enumerate(zip(encoded, metadata)):
        for vertex_index in _meshlet_vertex_indices(item):
            decoded64, error64, payload_q = _decode_occurrence64(positions[vertex_index], meta)
            decoded32, error32 = _decode_occurrence32(positions[vertex_index], meta, payload_q)
            max_payload_error64 = max(max_payload_error64, error64)
            max_payload_error32 = max(max_payload_error32, error32)
            by_vertex64.setdefault(vertex_index, []).append((meshlet_index, decoded64))
            by_vertex32.setdefault(vertex_index, []).append((meshlet_index, decoded32))

    shared_vertices = 0
    shared_occurrences = 0
    bad_count = 0
    max_delta64 = 0.0
    max_delta32 = 0.0
    sum_sq64 = 0.0
    sum_sq32 = 0.0
    sample_count = 0
    worst_vertex = -1
    worst_meshlets = (-1, -1)

    for vertex_index, occurrences in by_vertex64.items():
        if len(occurrences) < 2:
            continue
        shared_vertices += 1
        shared_occurrences += len(occurrences)
        meshlet_indices = [mi for mi, _ in occurrences]
        arr64 = np.vstack([value for _, value in occurrences])
        arr32 = np.vstack([value for _, value in by_vertex32[vertex_index]])
        delta64, pair = _pairwise_max_delta(arr64)
        delta32, _ = _pairwise_max_delta(arr32)
        max_delta32 = max(max_delta32, delta32)
        if delta64 > max_delta64:
            max_delta64 = delta64
            worst_vertex = int(vertex_index)
            worst_meshlets = (meshlet_indices[pair[0]], meshlet_indices[pair[1]])
        if max(delta64, delta32) > effective_threshold:
            bad_count += 1
        mean64 = np.mean(arr64, axis=0)
        mean32 = np.mean(arr32, axis=0)
        sum_sq64 += float(np.sum(np.linalg.norm(arr64 - mean64, axis=1) ** 2))
        sum_sq32 += float(np.sum(np.linalg.norm(arr32 - mean32, axis=1) ** 2))
        sample_count += len(occurrences)

    rms64 = math.sqrt(sum_sq64 / sample_count) if sample_count else 0.0
    rms32 = math.sqrt(sum_sq32 / sample_count) if sample_count else 0.0
    stats = Mesh3Dv2WatertightStats(
        checked=True,
        enabled=enabled,
        shared_vertices=shared_vertices,
        shared_occurrences=shared_occurrences,
        shared_vertex_decode_max_delta=max_delta64,
        shared_vertex_decode_rms_delta=rms64,
        shared_vertex_decode_float32_max_delta=max_delta32,
        shared_vertex_decode_float32_rms_delta=rms32,
        shared_vertex_decode_bad_count=bad_count,
        worst_shared_vertex=worst_vertex,
        worst_meshlet_pair=worst_meshlets,
        max_payload_decode_error=max_payload_error64,
        max_payload_decode_error_float32=max_payload_error32,
        bbox_extent=float(bbox_extent),
        threshold=effective_threshold,
    )
    if raise_on_failure and (bad_count or max_payload_error64 > effective_threshold):
        raise ValueError(
            "Mesh3Dv2 watertight check failed: "
            f"{bad_count} shared vertices exceed {effective_threshold:.9g}; "
            f"max delta64 is {max_delta64:.9g}, max delta32 is {max_delta32:.9g}, "
            f"max payload decode error is {max_payload_error64:.9g}, "
            f"worst vertex is {worst_vertex}"
        )
    return stats


def merge_watertight_stats(base: Mesh3Dv2WatertightStats, checked: Mesh3Dv2WatertightStats) -> Mesh3Dv2WatertightStats:
    """Merge solve/displacement stats with checker stats."""
    checked.enabled = base.enabled
    checked.iterations = base.iterations
    checked.max_geometry_displacement = base.max_geometry_displacement
    checked.rms_geometry_displacement = base.rms_geometry_displacement
    checked.avg_radius_inflation = base.avg_radius_inflation
    checked.max_radius_inflation = base.max_radius_inflation
    checked.bbox_extent = base.bbox_extent
    return checked


def format_watertight_stats(stats: Mesh3Dv2WatertightStats, *, prefix: str = "") -> list[str]:
    if not stats.checked and not stats.enabled:
        return []
    label = "enabled" if stats.enabled else "diagnostic-only"
    return [
        f"{prefix}Mesh3Dv2 watertight: {label}",
        f"{prefix} watertight iterations: {stats.iterations}",
        f"{prefix} shared vertices: {stats.shared_vertices} ({stats.shared_occurrences} occurrences)",
        f"{prefix} shared decode max delta: {stats.shared_vertex_decode_max_delta:.9g}",
        f"{prefix} shared decode RMS delta: {stats.shared_vertex_decode_rms_delta:.9g}",
        f"{prefix} shared decode float32 max delta: {stats.shared_vertex_decode_float32_max_delta:.9g}",
        f"{prefix} shared decode bad vertices: {stats.shared_vertex_decode_bad_count} over threshold {stats.threshold:.9g}",
        f"{prefix} max payload decode error: {stats.max_payload_decode_error:.9g}",
        f"{prefix} max geometry displacement: {stats.max_geometry_displacement:.9g}",
        f"{prefix} RMS geometry displacement: {stats.rms_geometry_displacement:.9g}",
        f"{prefix} radius inflation avg/max: {stats.avg_radius_inflation:.6g}/{stats.max_radius_inflation:.6g}",
    ]
