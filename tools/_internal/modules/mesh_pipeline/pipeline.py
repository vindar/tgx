from __future__ import annotations

import argparse
from pathlib import Path

import numpy as np

from .cones import apply_visibility_cones, auto_visibility_margin_deg
from .exporter import MeshletExportResult, export_mesh3dv2_header
from .mesh import Meshlet, ObjMesh, compute_triangle_normals, unique_valid
from .meshlets import MeshletBuildOptions, _make_meshlet, build_meshlets, meshlet_stats, sort_meshlets_by_material
from .preprocess import PreprocessStats
from .progress import log, step
from .profiles import DEFAULT_MESHLET_PROFILE, DEFAULT_MAX_NORMAL_ANGLE_DEG, DEFAULT_TARGET_VERTICES, meshlet_options_from_args, profile_names
from .quality import cull_ratio_stats, fibonacci_sphere
from .stripifier import DEFAULT_LKH_EXE, solver_stats_text
from .validate import validate_mesh_for_export
from .visibility import compute_tgx_visibility


def add_build_options(parser: argparse.ArgumentParser, *, source: str = "obj") -> None:
    include_obj = source != "legacy"
    include_legacy = True
    parser.add_argument("--profile", choices=profile_names(include_obj=include_obj, include_legacy=include_legacy), default=DEFAULT_MESHLET_PROFILE, help=f"meshlet tuning profile; default is {DEFAULT_MESHLET_PROFILE} (target vertices {DEFAULT_TARGET_VERTICES}, max normal angle {DEFAULT_MAX_NORMAL_ANGLE_DEG:g})")
    parser.add_argument("--builder", choices=("greedy", "hybrid", "nocull", "visibility_merge"), default=None)
    parser.add_argument("--target-vertices", type=int, default=None)
    parser.add_argument("--max-vertices", type=int, default=None)
    parser.add_argument("--max-triangles", type=int, default=None)
    parser.add_argument("--max-normal-angle", type=float, default=None)
    parser.add_argument("--radius-weight", type=float, default=None)
    parser.add_argument("--normal-weight", type=float, default=None)
    parser.add_argument("--vertex-weight", type=float, default=None)
    parser.add_argument("--stop-score", type=float, default=None)
    parser.add_argument("--compatible-edge-weight", type=float, default=None)
    parser.add_argument("--incompatible-edge-penalty", type=float, default=None)
    parser.add_argument("--compatible-merge-weight", type=float, default=None)
    parser.add_argument("--nocull-lkh-component-limit", type=int, default=None)
    parser.add_argument("--nocull-lkh-time-limit", type=float, default=None, help="override automatic stripifier budget for nocull components")
    parser.add_argument("--visibility-merge-samples", type=int, default=None)
    parser.add_argument("--visibility-merge-size", type=int, default=None)
    parser.add_argument("--visibility-merge-margin", type=float, default=None)
    parser.add_argument("--visibility-merge-cone-weight", type=float, default=None)
    parser.add_argument("--merge-passes", type=int, default=None)
    parser.add_argument("--smooth-passes", type=int, default=None)
    parser.add_argument("--merge-max-normal-angle", type=float, default=None)


def add_visibility_options(parser: argparse.ArgumentParser, *, default_samples: int = 2048, default_size: int = 768) -> None:
    parser.add_argument("--visibility-cones", action="store_true", help="compute true visibility cones with the TGX C++ helper")
    parser.add_argument("--visibility-samples", type=int, default=default_samples)
    parser.add_argument("--visibility-size", type=int, default=default_size)
    parser.add_argument("--visibility-margin", type=float, default=-1.0, help="extra cone margin in degrees; negative selects an automatic margin")
    parser.add_argument("--visibility-helper")
    parser.add_argument("--keep-visibility-files", action="store_true")
    parser.add_argument("--cone-source", choices=("normal", "visibility"), default="normal")


def options_from_args(args: argparse.Namespace, mesh: ObjMesh | None = None, *, source: str = "obj") -> MeshletBuildOptions:
    return meshlet_options_from_args(args, mesh, source=source)


def maybe_compute_visibility_cones(args: argparse.Namespace, mesh: ObjMesh, meshlets: list[Meshlet]) -> list[Meshlet]:
    if not getattr(args, "visibility_cones", False):
        return meshlets
    views = fibonacci_sphere(args.visibility_samples)
    margin = args.visibility_margin
    if margin < 0.0:
        margin = auto_visibility_margin_deg(len(views))
    print("TGX visibility cones:")
    print(f"  views         : {len(views)}")
    print(f"  render size   : {args.visibility_size}x{args.visibility_size}")
    print(f"  margin        : {margin:.2f} deg" + (" (auto)" if args.visibility_margin < 0.0 else ""))
    with step("TGX visibility render", f"{len(meshlets)} meshlets, {len(views)} views"):
        visibility = compute_tgx_visibility(
            mesh,
            meshlets,
            views,
            width=args.visibility_size,
            height=args.visibility_size,
            helper=args.visibility_helper,
            keep_files=getattr(args, "keep_visibility_files", False),
        )
    visible_counts = np.sum(visibility, axis=0)
    print(f"  visible views : min {int(np.min(visible_counts))}, avg {float(np.mean(visible_counts)):.1f}, max {int(np.max(visible_counts))}")
    print(f"  never visible : {int(np.sum(visible_counts == 0))}")
    return apply_visibility_cones(meshlets, views, visibility, margin_deg=margin)


def triangle_index_sets(mesh: ObjMesh, tri_indices: list[int]) -> tuple[set[int], set[int], set[int]]:
    vertices: set[int] = set()
    texcoords: set[int] = set()
    normals: set[int] = set()
    for ti in tri_indices:
        tri = mesh.triangles[ti]
        vertices.update(c.v for c in tri.corners)
        texcoords.update(unique_valid(c.vt for c in tri.corners))
        normals.update(unique_valid(c.vn for c in tri.corners))
    return vertices, texcoords, normals


def merge_small_material_meshlets(mesh: ObjMesh, meshlets: list[Meshlet], options: MeshletBuildOptions) -> list[Meshlet]:
    """Merge material groups that fit in one local meshlet table."""
    tri_normals = compute_triangle_normals(mesh)
    by_material: dict[str, list[int]] = {}
    material_order: list[str] = []
    for meshlet in meshlets:
        if meshlet.material not in by_material:
            by_material[meshlet.material] = []
            material_order.append(meshlet.material)
        by_material[meshlet.material].extend(meshlet.triangles)

    merged: list[Meshlet] = []
    consumed: set[str] = set()
    for material in material_order:
        if material in consumed:
            continue
        tri_indices = by_material[material]
        vertices, texcoords, normals = triangle_index_sets(mesh, tri_indices)
        if (
            len(vertices) <= options.max_vertices
            and len(texcoords) <= options.max_texcoords
            and len(normals) <= options.max_normals
        ):
            merged.append(_make_meshlet(mesh, tri_indices, tri_normals))
            consumed.add(material)
            continue
        for meshlet in meshlets:
            if meshlet.material == material:
                merged.append(meshlet)
        consumed.add(material)
    return merged


def build_meshlets_for_args(args: argparse.Namespace, mesh: ObjMesh, *, source: str = "obj") -> tuple[list[Meshlet], MeshletBuildOptions]:
    options = options_from_args(args, mesh, source=source)
    setattr(args, "_resolved_meshlet_options", options)
    with step("meshlet build", f"builder={options.builder}, triangles={len(mesh.triangles)}"):
        meshlets = build_meshlets(mesh, options)
    log(f"meshlet build result: {len(meshlets)} meshlets")
    if getattr(args, "merge_small_materials", False):
        before = len(meshlets)
        with step("merge small material meshlets", f"{before} meshlets"):
            meshlets = merge_small_material_meshlets(mesh, meshlets, options)
        if len(meshlets) != before:
            log(f"merge small material meshlets: {before} -> {len(meshlets)}")
    return meshlets, options


def finalize_meshlets_for_export(args: argparse.Namespace, mesh: ObjMesh, meshlets: list[Meshlet], *, cone_source: str | None = None) -> tuple[list[Meshlet], str]:
    meshlets = maybe_compute_visibility_cones(args, mesh, meshlets)
    if getattr(args, "sort_by_material", True):
        with step("sort meshlets by material", f"{len(meshlets)} meshlets"):
            meshlets = sort_meshlets_by_material(meshlets)
    with step("validate meshlets", f"{len(meshlets)} meshlets"):
        validate_mesh_for_export(mesh, meshlets)
    if cone_source is not None:
        return meshlets, cone_source
    resolved_options = getattr(args, "_resolved_meshlet_options", None)
    resolved_builder = getattr(resolved_options, "builder", getattr(args, "builder", ""))
    resolved = "visibility" if getattr(args, "visibility_cones", False) else "normal"
    if resolved_builder == "nocull":
        resolved = "nocull"
    elif resolved_builder == "visibility_merge":
        resolved = "visibility"
    return meshlets, resolved


def export_common(
    args: argparse.Namespace,
    mesh: ObjMesh,
    meshlets: list[Meshlet],
    *,
    output: Path,
    color_type: str,
    cone_source: str = "normal",
    texture_symbols: dict[str, str] | None = None,
    extra_includes: list[str] | None = None,
) -> MeshletExportResult:
    fmt = getattr(args, "mesh_format", "mesh3dv2")
    if fmt != "mesh3dv2":
        raise ValueError(f"unsupported meshlet format: {fmt}")
    exporter = export_mesh3dv2_header
    kwargs = dict(
        name=args.name or output.stem,
        color_type=color_type,
        use_lkh=True,
        lkh_exe=getattr(args, "lkh", DEFAULT_LKH_EXE),
        texture_symbols=texture_symbols or {},
        extra_includes=extra_includes or [],
        header_filename=output.name,
        single_header=getattr(args, "single_header", False),
    )
    kwargs.update(cone_source=cone_source)
    with step("encode/export mesh", f"{fmt}, {len(meshlets)} meshlets"):
        result = exporter(
            mesh,
            meshlets,
            **kwargs,
        )
    log(f"stripifier choices: {solver_stats_text()}")
    output.parent.mkdir(parents=True, exist_ok=True)
    with step("write header", str(output)):
        with output.open("w", encoding="utf-8", newline="\n") as handle:
            handle.write(result.header)
    if result.source is not None:
        source_output = output.with_suffix(".cpp")
        with step("write source", str(source_output)):
            with source_output.open("w", encoding="utf-8", newline="\n") as handle:
                handle.write(result.source)
    elif getattr(args, "single_header", False):
        stale_source = output.with_suffix(".cpp")
        if stale_source.exists():
            log(f"single-header output: not writing companion source; remove old split source if unused: {stale_source}")
    return result


def print_mesh_stats(mesh: ObjMesh, meshlets: list[Meshlet] | None = None) -> None:
    bb_min, bb_max = mesh.bounding_box()
    print(f"mesh: {mesh.name}")
    print(f"  vertices : {len(mesh.vertices)}")
    print(f"  texcoords: {len(mesh.texcoords)}")
    print(f"  normals  : {len(mesh.normals)}")
    print(f"  triangles: {len(mesh.triangles)}")
    print(f"  bbox     : [{bb_min[0]:.6g},{bb_max[0]:.6g}] x [{bb_min[1]:.6g},{bb_max[1]:.6g}] x [{bb_min[2]:.6g},{bb_max[2]:.6g}]")
    materials = sorted({t.material for t in mesh.triangles})
    print(f"  materials: {len(materials)}")
    if meshlets is not None:
        stats = meshlet_stats(meshlets)
        print("meshlets:")
        print(f"  count         : {int(stats['meshlets'])}")
        print(f"  triangles     : min {stats['triangles_min']:.0f}, avg {stats['triangles_avg']:.1f}, max {stats['triangles_max']:.0f}")
        print(f"  vertices      : min {stats['vertices_min']:.0f}, avg {stats['vertices_avg']:.1f}, max {stats['vertices_max']:.0f}")
        print(f"  radius avg    : {stats['radius_avg']:.6g}")
        print(f"  normal angle  : avg {stats['normal_angle_avg']:.1f} deg, max {stats['normal_angle_max']:.1f} deg")
        print(f"  cull angle    : min {stats['cull_angle_min']:.1f} deg, avg {stats['cull_angle_avg']:.1f} deg, max {stats['cull_angle_max']:.1f} deg")
        print(f"  empty cull    : {int(stats['cull_empty'])}")
        visible_samples = [m.visibility_samples for m in meshlets if m.visibility_samples > 0]
        if visible_samples:
            print(f"  vis samples   : min {min(visible_samples)}, avg {sum(visible_samples) / len(visible_samples):.1f}, max {max(visible_samples)}")


def print_preprocess_stats(stats: PreprocessStats) -> None:
    print("preprocess:")
    print(f"  vertices : {stats.vertices_before} -> {stats.vertices_after}")
    print(f"  texcoords: {stats.texcoords_before} -> {stats.texcoords_after}")
    print(f"  normals  : {stats.normals_before} -> {stats.normals_after}" + (" (generated)" if stats.normals_generated else ""))
    if stats.degenerate_triangles_removed:
        print(f"  degenerate triangles removed: {stats.degenerate_triangles_removed}")
    print(f"  normalize: {'yes' if stats.normalized else 'no'}")


def print_cull_quality(meshlets: list[Meshlet], samples: int, meshlet_cost: float, cone_source: str) -> None:
    q = cull_ratio_stats(meshlets, samples, meshlet_cost, cone_source)
    print("meshlet cone culling:")
    print(f"  cone source   : {q['cone_source']}")
    print(f"  samples       : {int(q['samples'])}")
    print(f"  meshlet cost  : {q['meshlet_cost']:.2f} triangle-equivalent per meshlet")
    print(f"  gross culled  : avg {100.0 * q['gross_mean']:.1f}%, min {100.0 * q['gross_min']:.1f}%, max {100.0 * q['gross_max']:.1f}%")
    print(f"  gross distrib : p10 {100.0 * q['gross_p10']:.1f}%, median {100.0 * q['gross_p50']:.1f}%, p90 {100.0 * q['gross_p90']:.1f}%")
    print(f"  net gain      : avg {100.0 * q['net_mean']:.1f}%, min {100.0 * q['net_min']:.1f}%, max {100.0 * q['net_max']:.1f}%")
    print(f"  net distrib   : p10 {100.0 * q['net_p10']:.1f}%, median {100.0 * q['net_p50']:.1f}%, p90 {100.0 * q['net_p90']:.1f}%")
