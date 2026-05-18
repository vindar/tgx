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
    "nocull": dict(builder="nocull", target_vertices=127, max_triangles=65535, max_normal_angle_deg=180.0, radius_weight=0.0, normal_weight=0.0, vertex_weight=0.0, stop_score=999.0, merge_passes=1, smooth_passes=0, merge_max_normal_angle_deg=180.0),
    "visibility_merge": dict(builder="visibility_merge", target_vertices=52, max_triangles=127, max_normal_angle_deg=42.0, radius_weight=7.0, normal_weight=18.0, vertex_weight=2.0, stop_score=4.8, compatible_edge_weight=0.0, incompatible_edge_penalty=0.0, visibility_merge_cone_weight=45.0, visibility_merge_samples=1024, visibility_merge_size=1024, visibility_merge_margin_deg=-1.0, merge_passes=1, smooth_passes=0, merge_max_normal_angle_deg=48.0),
}


def profile_names(*, include_obj: bool = True, include_legacy: bool = True) -> tuple[str, ...]:
    names = ["auto"]
    if include_obj:
        names.extend(["balanced", "smooth", "coarse", "fast", "visibility_merge"])
    if include_legacy:
        names.extend(["balanced", "material", "non_culling", "nocull", "visibility_merge"])
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
    """Resolve the profile without clobbering explicit command-line overrides."""
    profile = _base_profile_name(args, mesh, source=source)
    values = _profile_values(args, mesh, source=source)
    if values is not None:
        for key, value in values.items():
            arg_key = _arg_name(key)
            if getattr(args, arg_key, None) is None:
                setattr(args, arg_key, value)
    return profile


def meshlet_options_from_args(args, mesh: ObjMesh | None = None, *, source: str = "obj") -> MeshletBuildOptions:
    values = _profile_values(args, mesh, source=source)

    def opt(name: str, default=None):
        cli_value = getattr(args, _arg_name(name), None)
        if cli_value is not None:
            return cli_value
        if values is not None and name in values:
            return values[name]
        return default

    return MeshletBuildOptions(
        builder=str(opt("builder", "greedy")),
        target_vertices=int(opt("target_vertices", 52)),
        max_vertices=int(opt("max_vertices", 127)),
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
        nocull_lkh_component_limit=int(opt("nocull_lkh_component_limit", 500)),
        nocull_lkh_time_limit=_optional_float(opt("nocull_lkh_time_limit", None)),
        visibility_merge_samples=int(opt("visibility_merge_samples", 1024)),
        visibility_merge_size=int(opt("visibility_merge_size", 1024)),
        visibility_merge_margin_deg=float(opt("visibility_merge_margin_deg", -1.0)),
        visibility_merge_cone_weight=float(opt("visibility_merge_cone_weight", 45.0)),
        merge_passes=int(opt("merge_passes", 1)),
        smooth_passes=int(opt("smooth_passes", 0)),
        merge_max_normal_angle_deg=float(opt("merge_max_normal_angle_deg", 48.0)),
    )


def _optional_float(value) -> float | None:
    if value is None:
        return None
    return float(value)


def _arg_name(profile_key: str) -> str:
    if profile_key.endswith("_deg"):
        profile_key = profile_key[:-4]
    return profile_key


def _base_profile_name(args, mesh: ObjMesh | None, *, source: str) -> str:
    requested = getattr(args, "profile", "custom")
    profile = resolve_profile(mesh, requested, source=source)
    builder = getattr(args, "builder", None)
    if profile == "custom" and builder in MESHLET_PROFILES:
        # A command such as "--builder visibility_merge --max-normal-angle 73"
        # should use the visibility_merge defaults and override only the angle.
        return builder
    return profile


def _profile_values(args, mesh: ObjMesh | None, *, source: str):
    return MESHLET_PROFILES.get(_base_profile_name(args, mesh, source=source))
