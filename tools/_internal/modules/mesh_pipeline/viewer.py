from __future__ import annotations

import re
from pathlib import Path

import matplotlib
import numpy as np
from matplotlib import pyplot as plt
from mpl_toolkits.mplot3d.art3d import Poly3DCollection

from .mesh import Meshlet, ObjMesh


def _load_pyvista_texture(pv, path: Path, *, resize: tuple[int, int] | None = None, resample: str = "lanczos"):
    """Load a texture through Pillow so previews support formats VTK may not read."""
    return pv.Texture(_load_texture_rgb_array(path, resize=resize, resample=resample))


def _resample_filter(name: str) -> int:
    from PIL import Image

    resampling = getattr(Image, "Resampling", Image)
    name = name.lower()
    if name == "nearest":
        return resampling.NEAREST
    if name == "bilinear":
        return resampling.BILINEAR
    if name == "bicubic":
        return resampling.BICUBIC
    return resampling.LANCZOS


def _first_tgx_image_symbol(path: Path) -> str:
    header = path if path.suffix.lower() in (".h", ".hpp") else path.with_suffix(".h")
    files = [header, header.with_suffix(".cpp"), header.with_suffix(".cxx"), header.with_suffix(".cc")]
    raw_parts: list[str] = []
    seen: set[Path] = set()
    for file in files:
        try:
            resolved = file.resolve()
        except Exception:
            resolved = file
        if resolved in seen or not file.exists():
            continue
        seen.add(resolved)
        raw_parts.append(file.read_text(encoding="utf-8", errors="replace"))
    raw = "\n".join(raw_parts)
    raw = re.sub(r"/\*.*?\*/", "", raw, flags=re.S)
    raw = re.sub(r"//.*?$", "", raw, flags=re.M)
    match = re.search(r"(?:static\s+)?(?:extern\s+)?(?:const\s+)?tgx::Image<[^>]+>\s+(\w+)\s*(?:\(|;)", raw)
    return match.group(1) if match else ""


def _load_texture_rgb_array(
    path: Path,
    *,
    symbol: str = "",
    resize: tuple[int, int] | None = None,
    resample: str = "lanczos",
) -> np.ndarray:
    from PIL import Image

    if path.suffix.lower() in (".h", ".hpp", ".cpp", ".cxx", ".cc"):
        from .mesh_inspect import _parse_texture_image

        symbol = symbol or _first_tgx_image_symbol(path)
        texture = _parse_texture_image(path, symbol) if symbol else None
        if texture is None:
            raise ValueError(f"could not parse TGX texture image from {path}")
        image = Image.fromarray(texture.rgb, "RGB")
    else:
        with Image.open(path) as img:
            image = img.convert("RGB")
    if resize is not None:
        image = image.resize(resize, _resample_filter(resample))
    return np.asarray(image, dtype=np.uint8)


def _resize_texture_array(data: np.ndarray, size: tuple[int, int], resample: str) -> np.ndarray:
    from PIL import Image

    image = Image.fromarray(data.astype(np.uint8), "RGB")
    image = image.resize(size, _resample_filter(resample))
    return np.asarray(image, dtype=np.uint8)


def _material_color(desc: object | None, attr: str, default: tuple[float, float, float]) -> np.ndarray:
    values = getattr(desc, attr, default) if desc is not None else default
    try:
        return np.asarray(values[:3], dtype=np.float32)
    except Exception:
        return np.asarray(default, dtype=np.float32)


def _material_emissive_strength(desc: object | None) -> float:
    if desc is None:
        return 0.0
    try:
        return max(0.0, float(getattr(desc, "emissive_strength", 0.0)))
    except Exception:
        return 0.0


def _solid_color_image(color: np.ndarray, shape: tuple[int, int]) -> np.ndarray:
    height, width = shape
    rgb = np.clip(color, 0.0, 1.0) * 255.0
    return np.broadcast_to(rgb.reshape(1, 1, 3), (height, width, 3)).astype(np.float32)


def _texture_option(options: dict[str, tuple[tuple[int, int] | None, str]] | None, material: str) -> tuple[tuple[int, int] | None, str]:
    if options is None:
        return None, "lanczos"
    return options.get(material, (None, "lanczos"))


def _load_material_texture(
    desc: object | None,
    material: str,
    *,
    path_attr: str,
    symbol_attr: str,
    options: dict[str, tuple[tuple[int, int] | None, str]] | None,
) -> tuple[np.ndarray | None, str]:
    if desc is None:
        return None, "lanczos"
    path = getattr(desc, path_attr, None)
    if path is None or not Path(path).exists():
        return None, "lanczos"
    resize, resample = _texture_option(options, material)
    symbol = str(getattr(desc, symbol_attr, "") or "")
    return _load_texture_rgb_array(Path(path), symbol=symbol, resize=resize, resample=resample), resample


def _compose_material_preview_texture(
    pv,
    desc: object | None,
    material: str,
    *,
    texture_options: dict[str, tuple[tuple[int, int] | None, str]] | None,
    emissive_texture_options: dict[str, tuple[tuple[int, int] | None, str]] | None,
):
    diffuse_texture, diffuse_resample = _load_material_texture(
        desc,
        material,
        path_attr="texture_path",
        symbol_attr="texture_symbol_hint",
        options=texture_options,
    )
    emissive_texture, emissive_resample = _load_material_texture(
        desc,
        material,
        path_attr="emissive_texture_path",
        symbol_attr="emissive_texture_symbol_hint",
        options=emissive_texture_options,
    )
    if diffuse_texture is None and emissive_texture is None:
        return None

    reference = diffuse_texture if diffuse_texture is not None else emissive_texture
    assert reference is not None
    height, width = reference.shape[:2]
    diffuse_color = _material_color(desc, "diffuse", (0.75, 0.75, 0.75))
    emissive_color = _material_color(desc, "emissive", (0.0, 0.0, 0.0))
    strength = _material_emissive_strength(desc)

    if diffuse_texture is None:
        diffuse = _solid_color_image(diffuse_color, (height, width))
    else:
        diffuse = diffuse_texture.astype(np.float32)

    if emissive_texture is None:
        emissive = _solid_color_image(emissive_color * strength, (height, width))
    else:
        if emissive_texture.shape[:2] != (height, width):
            emissive_texture = _resize_texture_array(emissive_texture, (width, height), emissive_resample or diffuse_resample)
        emissive = emissive_texture.astype(np.float32) * strength

    combined = np.clip(diffuse + emissive, 0.0, 255.0).astype(np.uint8)
    return pv.Texture(combined)


def _material_preview_color(desc: object | None) -> tuple[float, float, float]:
    diffuse = _material_color(desc, "diffuse", (0.65, 0.68, 0.72))
    emissive = _material_color(desc, "emissive", (0.0, 0.0, 0.0)) * _material_emissive_strength(desc)
    color = np.clip(diffuse + emissive, 0.0, 1.0)
    return float(color[0]), float(color[1]), float(color[2])


def _set_axes_equal(ax, vertices: np.ndarray) -> None:
    bb_min = np.min(vertices, axis=0)
    bb_max = np.max(vertices, axis=0)
    center = (bb_min + bb_max) * 0.5
    radius = float(np.max(bb_max - bb_min) * 0.55)
    if radius <= 0.0:
        radius = 1.0
    ax.set_xlim(center[0] - radius, center[0] + radius)
    ax.set_ylim(center[1] - radius, center[1] + radius)
    ax.set_zlim(center[2] - radius, center[2] + radius)


def show_meshlets(
    mesh: ObjMesh,
    meshlets: list[Meshlet] | None = None,
    *,
    only_meshlet: int | None = None,
    max_triangles: int | None = None,
    save: str | Path | None = None,
    color_by: str = "meshlet",
) -> None:
    if save is not None:
        matplotlib.use("Agg")

    fig = plt.figure(figsize=(10, 8))
    ax = fig.add_subplot(111, projection="3d")

    if meshlets is None:
        tri_indices = list(range(len(mesh.triangles)))
        if max_triangles is not None:
            tri_indices = tri_indices[:max_triangles]
        polys = [mesh.triangle_vertices(i) for i in tri_indices]
        collection = Poly3DCollection(polys, facecolor=(0.6, 0.68, 0.76, 0.85), edgecolor=(0.05, 0.05, 0.05, 0.18), linewidth=0.15)
        ax.add_collection3d(collection)
        title = f"{mesh.name}: {len(mesh.triangles)} triangles"
    else:
        cmap = plt.colormaps["tab20"]
        selected = meshlets
        if only_meshlet is not None:
            selected = [meshlets[only_meshlet]]
        drawn = 0
        metric_values = _meshlet_metric_values(meshlets, color_by)
        norm = None
        if color_by != "meshlet" and len(metric_values) > 0:
            lo = float(np.min(metric_values))
            hi = float(np.max(metric_values))
            norm = (lo, hi if hi > lo else lo + 1.0)
        for mi, meshlet in enumerate(selected):
            source_index = only_meshlet if only_meshlet is not None else mi
            tri_indices = meshlet.triangles
            if max_triangles is not None and drawn >= max_triangles:
                break
            if max_triangles is not None:
                tri_indices = tri_indices[: max(0, max_triangles - drawn)]
            polys = [mesh.triangle_vertices(i) for i in tri_indices]
            if color_by == "meshlet":
                color = cmap(source_index % 20)
            else:
                value = metric_values[source_index]
                color = plt.colormaps["viridis"]((value - norm[0]) / (norm[1] - norm[0]))
            collection = Poly3DCollection(polys, facecolor=color[:3] + (0.88,), edgecolor=(0, 0, 0, 0.22), linewidth=0.12)
            ax.add_collection3d(collection)
            drawn += len(tri_indices)
        title = f"{mesh.name}: {len(meshlets)} meshlets / {len(mesh.triangles)} triangles"
        if only_meshlet is not None:
            title += f" / meshlet {only_meshlet}"
        if color_by != "meshlet":
            title += f" / color: {color_by}"

    _set_axes_equal(ax, mesh.vertices)
    ax.set_xlabel("X")
    ax.set_ylabel("Y")
    ax.set_zlabel("Z")
    ax.set_title(title)
    ax.view_init(elev=22, azim=-58)
    plt.tight_layout()

    if save is not None:
        fig.savefig(save, dpi=180)
        plt.close(fig)
    else:
        plt.show()


def show_meshlets_pyvista(
    mesh: ObjMesh,
    meshlets: list[Meshlet] | None = None,
    *,
    save: str | Path | None = None,
    show_edges: bool = True,
    color_by: str = "meshlet",
    view_dir: np.ndarray | None = None,
    cull_by_view: bool = False,
    cone_source: str = "normal",
    camera_pos: np.ndarray | None = None,
    camera_target: np.ndarray | None = None,
    cull_from_camera: bool = False,
    texture: bool = False,
    texture_options: dict[str, tuple[tuple[int, int] | None, str]] | None = None,
    emissive_texture_options: dict[str, tuple[tuple[int, int] | None, str]] | None = None,
) -> None:
    import pyvista as pv

    if meshlets is not None and cull_by_view:
        if cull_from_camera:
            if camera_pos is None:
                raise ValueError("camera_pos is required when cull_from_camera is enabled")
            camera_pos = np.asarray(camera_pos, dtype=np.float64)
            meshlets = [
                meshlet for meshlet in meshlets
                if not _is_culled_from_camera(meshlet, camera_pos, cone_source)
            ]
        else:
            if view_dir is None:
                raise ValueError("view_dir is required when cull_by_view is enabled")
            view_dir = np.asarray(view_dir, dtype=np.float64)
            n = float(np.linalg.norm(view_dir))
            if n <= 1e-12:
                raise ValueError("view_dir must be non-zero")
            view_dir = view_dir / n
            meshlets = [
                meshlet for meshlet in meshlets
                if not _is_culled(meshlet, view_dir, cone_source)
            ]

    if meshlets is None or (texture and mesh.has_texcoords()):
        _show_textured_pyvista(
            pv,
            mesh,
            meshlets,
            save=save,
            show_edges=show_edges,
            camera_pos=camera_pos,
            camera_target=camera_target,
            view_dir=view_dir,
            texture_options=texture_options,
            emissive_texture_options=emissive_texture_options,
            use_textures=texture and mesh.has_texcoords(),
        )
        return

    faces = []
    cell_values = []
    metric_values = _meshlet_metric_values(meshlets, color_by) if meshlets is not None else np.asarray([])
    if meshlets is None:
        for tri in mesh.triangles:
            faces.extend([3, tri.corners[0].v, tri.corners[1].v, tri.corners[2].v])
            cell_values.append(0.0)
    else:
        for mi, meshlet in enumerate(meshlets):
            for ti in meshlet.triangles:
                tri = mesh.triangles[ti]
                faces.extend([3, tri.corners[0].v, tri.corners[1].v, tri.corners[2].v])
                cell_values.append(float(mi if color_by == "meshlet" else metric_values[mi]))

    pv_mesh = pv.PolyData(mesh.vertices, np.asarray(faces, dtype=np.int64))
    scalar_name = color_by if meshlets is not None else "mesh"
    pv_mesh.cell_data[scalar_name] = np.asarray(cell_values, dtype=np.float64)

    plotter = pv.Plotter(off_screen=save is not None)
    plotter.add_mesh(
        pv_mesh,
        scalars=scalar_name if meshlets is not None else None,
        cmap="tab20" if color_by == "meshlet" else "viridis",
        show_edges=show_edges,
        edge_color=(0.05, 0.05, 0.05),
        line_width=0.25,
        lighting=True,
    )
    plotter.add_axes()
    plotter.add_title(
        f"{mesh.name}: {len(meshlets) if meshlets is not None else 1} meshlets / {len(mesh.triangles)} triangles",
        font_size=12,
    )
    plotter.reset_camera()
    if camera_pos is not None:
        center = np.mean(mesh.vertices, axis=0)
        target = np.asarray(camera_target, dtype=np.float64) if camera_target is not None else center
        pos = np.asarray(camera_pos, dtype=np.float64)
        view = target - pos
        view_len = float(np.linalg.norm(view))
        if view_len <= 1e-12:
            raise ValueError("camera position and target must be distinct")
        view /= view_len
        up = np.array([0.0, 1.0, 0.0], dtype=np.float64)
        if abs(float(np.dot(up, view))) > 0.92:
            up = np.array([0.0, 0.0, 1.0], dtype=np.float64)
        plotter.camera_position = (tuple(pos), tuple(target), tuple(up))
    elif view_dir is not None:
        center = np.mean(mesh.vertices, axis=0)
        radius = float(np.max(np.linalg.norm(mesh.vertices - center, axis=1)))
        pos = center - view_dir * max(radius * 3.0, 1.0)
        up = np.array([0.0, 0.0, 1.0], dtype=np.float64)
        if abs(float(np.dot(up, view_dir))) > 0.92:
            up = np.array([0.0, 1.0, 0.0], dtype=np.float64)
        plotter.camera_position = (tuple(pos), tuple(center), tuple(up))
    else:
        plotter.camera_position = "iso"
    if save is not None:
        plotter.screenshot(str(save), window_size=(1600, 1200))
        plotter.close()
    else:
        plotter.show()


def _show_textured_pyvista(
    pv,
    mesh: ObjMesh,
    meshlets: list[Meshlet] | None,
    *,
    save: str | Path | None,
    show_edges: bool,
    camera_pos: np.ndarray | None,
    camera_target: np.ndarray | None,
    view_dir: np.ndarray | None,
    texture_options: dict[str, tuple[tuple[int, int] | None, str]] | None = None,
    emissive_texture_options: dict[str, tuple[tuple[int, int] | None, str]] | None = None,
    use_textures: bool = True,
) -> None:
    tri_indices = list(range(len(mesh.triangles))) if meshlets is None else [ti for m in meshlets for ti in m.triangles]
    by_material: dict[str, list[int]] = {}
    for ti in tri_indices:
        by_material.setdefault(mesh.triangles[ti].material, []).append(ti)

    plotter = pv.Plotter(off_screen=save is not None)
    for material, indices in sorted(by_material.items()):
        points = []
        tcoords = []
        faces = []
        for ti in indices:
            base = len(points)
            tri = mesh.triangles[ti]
            for corner in tri.corners:
                points.append(mesh.vertices[corner.v])
                if corner.vt >= 0:
                    tcoords.append(mesh.texcoords[corner.vt])
                else:
                    tcoords.append((0.0, 0.0))
            faces.extend([3, base, base + 1, base + 2])
        if not points:
            continue
        pv_mesh = pv.PolyData(np.asarray(points, dtype=np.float64), np.asarray(faces, dtype=np.int64))
        if use_textures:
            pv_mesh.active_texture_coordinates = np.asarray(tcoords, dtype=np.float64)
        desc = mesh.materials.get(material)
        tex = None
        if use_textures:
            tex = _compose_material_preview_texture(
                pv,
                desc,
                material,
                texture_options=texture_options,
                emissive_texture_options=emissive_texture_options,
            )
        if tex is not None:
            plotter.add_mesh(pv_mesh, texture=tex, show_edges=show_edges, edge_color=(0.05, 0.05, 0.05), line_width=0.2, lighting=True)
        else:
            color = _material_preview_color(desc)
            plotter.add_mesh(pv_mesh, color=color, show_edges=show_edges, edge_color=(0.05, 0.05, 0.05), line_width=0.2, lighting=True)

    plotter.add_axes()
    title_mode = "textured preview" if use_textures else f"{len(meshlets) if meshlets is not None else 1} meshlets"
    plotter.add_title(f"{mesh.name}: {title_mode} / {len(tri_indices)} triangles", font_size=12)
    if camera_pos is not None:
        center = np.mean(mesh.vertices, axis=0)
        target = np.asarray(camera_target, dtype=np.float64) if camera_target is not None else center
        pos = np.asarray(camera_pos, dtype=np.float64)
        view = target - pos
        view_len = float(np.linalg.norm(view))
        if view_len > 1e-12:
            view /= view_len
            up = np.array([0.0, 1.0, 0.0], dtype=np.float64)
            if abs(float(np.dot(up, view))) > 0.92:
                up = np.array([0.0, 0.0, 1.0], dtype=np.float64)
            plotter.camera_position = (tuple(pos), tuple(target), tuple(up))
    elif view_dir is not None:
        center = np.mean(mesh.vertices, axis=0)
        radius = float(np.max(np.linalg.norm(mesh.vertices - center, axis=1)))
        view_dir = view_dir / max(float(np.linalg.norm(view_dir)), 1e-12)
        plotter.camera_position = (tuple(center - view_dir * max(radius * 3.0, 1.0)), tuple(center), (0.0, 0.0, 1.0))
    else:
        plotter.camera_position = "iso"
    plotter.reset_camera()
    if save is not None:
        plotter.screenshot(str(save), window_size=(1600, 1200))
        plotter.close()
    else:
        plotter.show()


def _meshlet_metric_values(meshlets: list[Meshlet] | None, color_by: str) -> np.ndarray:
    if meshlets is None:
        return np.asarray([], dtype=np.float64)
    if color_by == "meshlet":
        return np.arange(len(meshlets), dtype=np.float64)
    if color_by == "normal_angle":
        return np.asarray([m.normal_angle_deg() for m in meshlets], dtype=np.float64)
    if color_by == "cull_angle":
        return np.asarray([m.cull_angle_deg() for m in meshlets], dtype=np.float64)
    if color_by == "visibility_angle":
        return np.asarray([m.visibility_angle_deg() for m in meshlets], dtype=np.float64)
    if color_by == "visibility_samples":
        return np.asarray([m.visibility_samples for m in meshlets], dtype=np.float64)
    if color_by == "radius":
        return np.asarray([m.radius for m in meshlets], dtype=np.float64)
    if color_by == "triangles":
        return np.asarray([len(m.triangles) for m in meshlets], dtype=np.float64)
    if color_by == "vertices":
        return np.asarray([len(m.vertices) for m in meshlets], dtype=np.float64)
    raise ValueError(f"unknown meshlet metric: {color_by}")


def _is_culled(meshlet: Meshlet, view_dir: np.ndarray, cone_source: str) -> bool:
    if cone_source in ("normal", "visibility"):
        axis, cos_angle = meshlet.selected_cull_cone(cone_source)
        if cos_angle > 1.0:
            return False
        # view_dir is camera->object; runtime cones store object->camera.
        return float(np.dot(axis, -view_dir)) < cos_angle
    raise ValueError(f"unknown cull cone source: {cone_source}")


def _is_culled_from_camera(meshlet: Meshlet, camera_pos: np.ndarray, cone_source: str) -> bool:
    if cone_source in ("normal", "visibility"):
        axis, cos_angle = meshlet.selected_cull_cone(cone_source)
        if cos_angle > 1.0:
            return False
        # Match the runtime's conservative perspective test.
        anchor = meshlet.center - axis * meshlet.radius
        camera_dir = camera_pos - anchor
        n = float(np.linalg.norm(camera_dir))
        if n <= 1e-12:
            return False
        camera_dir /= n
        return float(np.dot(axis, camera_dir)) < cos_angle
    raise ValueError(f"unknown cull cone source: {cone_source}")
