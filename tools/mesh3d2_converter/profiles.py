from __future__ import annotations

from .mesh import ObjMesh
from .meshlets import MeshletBuildOptions
from .stripifier import DEFAULT_LKH_EXE


MESHLET_PROFILES: dict[str, dict[str, float | int | str]] = {
    "balanced": dict(target_vertices=52, max_triangles=70, max_normal_angle_deg=42.0, radius_weight=7.0, normal_weight=18.0, vertex_weight=2.0, stop_score=4.8, merge_passes=1, smooth_passes=0, merge_max_normal_angle_deg=48.0),
    "smooth": dict(target_vertices=52, max_triangles=70, max_normal_angle_deg=42.0, radius_weight=7.0, normal_weight=18.0, vertex_weight=2.0, stop_score=4.8, merge_passes=1, smooth_passes=3, merge_max_normal_angle_deg=48.0),
    "coarse": dict(target_vertices=127, max_triangles=800, max_normal_angle_deg=120.0, radius_weight=0.2, normal_weight=0.5, vertex_weight=0.1, stop_score=1000.0, merge_passes=1, smooth_passes=1, merge_max_normal_angle_deg=120.0),
    "fast": dict(target_vertices=96, max_triangles=180, max_normal_angle_deg=58.0, radius_weight=0.7, normal_weight=3.0, vertex_weight=0.5, stop_score=30.0, merge_passes=1, smooth_passes=0, merge_max_normal_angle_deg=64.0),
    "material": dict(target_vertices=112, max_triangles=220, max_normal_angle_deg=125.0, radius_weight=0.5, normal_weight=0.5, vertex_weight=0.1, stop_score=999.0, merge_passes=2, smooth_passes=0, merge_max_normal_angle_deg=140.0),
    "non_culling": dict(builder="hybrid", target_vertices=127, max_triangles=512, max_normal_angle_deg=180.0, radius_weight=0.8, normal_weight=0.0, vertex_weight=0.1, stop_score=999.0, compatible_edge_weight=8.0, incompatible_edge_penalty=4.0, compatible_merge_weight=0.05, merge_passes=3, smooth_passes=0, merge_max_normal_angle_deg=180.0),
    "nocull": dict(builder="nocull", target_vertices=127, max_triangles=65535, max_normal_angle_deg=180.0, radius_weight=0.0, normal_weight=0.0, vertex_weight=0.0, stop_score=999.0, nocull_lkh_time_limit=30, merge_passes=1, smooth_passes=0, merge_max_normal_angle_deg=180.0),
}


def profile_names(*, include_obj: bool = True, include_legacy: bool = True) -> tuple[str, ...]:
    names = ["auto"]
    if include_obj:
        names.extend(["balanced", "smooth", "coarse", "fast"])
    if include_legacy:
        names.extend(["balanced", "material", "non_culling", "nocull"])
    names.append("custom")
    return tuple(dict.fromkeys(names))


def resolve_profile(mesh: ObjMesh | None, requested: str, *, source: str) -> str:
    if requested != "auto":
        return requested
    if source == "legacy":
        return "material" if mesh is not None and len(mesh.materials) >= 4 else "balanced"

    triangles = len(mesh.triangles) if mesh is not None else 0
    if triangles >= 12000:
        return "coarse"
    if mesh is not None and len({t.material for t in mesh.triangles}) >= 4:
        return "material"
    if triangles >= 7000:
        return "fast"
    return "smooth"


def apply_profile_to_args(args, mesh: ObjMesh | None, *, source: str) -> str:
    profile = resolve_profile(mesh, getattr(args, "profile", "custom"), source=source)
    values = MESHLET_PROFILES.get(profile)
    if values is not None:
        for key, value in values.items():
            arg_key = key[:-4] if key.endswith("_deg") else key
            setattr(args, arg_key, value)
    return profile


def meshlet_options_from_args(args, mesh: ObjMesh | None = None, *, source: str = "obj") -> MeshletBuildOptions:
    profile = resolve_profile(mesh, getattr(args, "profile", "custom"), source=source)
    values = MESHLET_PROFILES.get(profile)

    def opt(name: str, default=None):
        if values is not None and name in values:
            return values[name]
        legacy_name = name[:-4] if name.endswith("_deg") else name
        return getattr(args, legacy_name, default)

    return MeshletBuildOptions(
        builder=str(opt("builder", getattr(args, "builder", "greedy"))),
        target_vertices=int(opt("target_vertices", 52)),
        max_vertices=int(getattr(args, "max_vertices", 127)),
        max_triangles=int(opt("max_triangles", 70)),
        max_normal_angle_deg=float(opt("max_normal_angle_deg", 42.0)),
        radius_weight=float(opt("radius_weight", 7.0)),
        normal_weight=float(opt("normal_weight", 18.0)),
        vertex_weight=float(opt("vertex_weight", 2.0)),
        stop_score=float(opt("stop_score", 4.8)),
        compatible_edge_weight=float(opt("compatible_edge_weight", 0.0)),
        incompatible_edge_penalty=float(opt("incompatible_edge_penalty", 0.0)),
        compatible_merge_weight=float(opt("compatible_merge_weight", 0.0)),
        lkh_exe=str(getattr(args, "lkh", DEFAULT_LKH_EXE)),
        nocull_lkh_component_limit=int(opt("nocull_lkh_component_limit", getattr(args, "nocull_lkh_component_limit", 500))),
        nocull_lkh_time_limit=int(opt("nocull_lkh_time_limit", getattr(args, "nocull_lkh_time_limit", 30))),
        merge_passes=int(opt("merge_passes", 1)),
        smooth_passes=int(opt("smooth_passes", 0)),
        merge_max_normal_angle_deg=float(opt("merge_max_normal_angle_deg", 48.0)),
    )
