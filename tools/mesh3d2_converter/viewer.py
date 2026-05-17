from __future__ import annotations

from pathlib import Path

import matplotlib
import numpy as np
from matplotlib import pyplot as plt
from mpl_toolkits.mplot3d.art3d import Poly3DCollection

from .mesh import Meshlet, ObjMesh


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

    if texture and mesh.has_texcoords():
        _show_textured_pyvista(
            pv,
            mesh,
            meshlets,
            save=save,
            show_edges=show_edges,
            camera_pos=camera_pos,
            camera_target=camera_target,
            view_dir=view_dir,
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
        pv_mesh.active_texture_coordinates = np.asarray(tcoords, dtype=np.float64)
        desc = mesh.materials.get(material)
        tex = None
        if desc is not None and desc.texture_path is not None and desc.texture_path.exists():
            tex = pv.Texture(str(desc.texture_path))
        if tex is not None:
            plotter.add_mesh(pv_mesh, texture=tex, show_edges=show_edges, edge_color=(0.05, 0.05, 0.05), line_width=0.2, lighting=True)
        else:
            color = desc.diffuse if desc is not None else (0.65, 0.68, 0.72)
            plotter.add_mesh(pv_mesh, color=color, show_edges=show_edges, edge_color=(0.05, 0.05, 0.05), line_width=0.2, lighting=True)

    plotter.add_axes()
    plotter.add_title(f"{mesh.name}: textured preview / {len(tri_indices)} triangles", font_size=12)
    plotter.reset_camera()
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
    if cone_source == "normal":
        cull_axis, cull_cos = meshlet.selected_cull_cone("normal")
        return cull_cos <= 1.0 and float(np.dot(view_dir, cull_axis)) >= cull_cos
    if cone_source == "visibility":
        if meshlet.visibility_axis is None:
            return False
        # view_dir is camera->object; visibility_axis is object->camera.
        return float(np.dot(meshlet.visibility_axis, -view_dir)) < meshlet.visibility_cos
    raise ValueError(f"unknown cull cone source: {cone_source}")


def _is_culled_from_camera(meshlet: Meshlet, camera_pos: np.ndarray, cone_source: str) -> bool:
    if cone_source == "normal":
        view_dir = meshlet.center - camera_pos
        n = float(np.linalg.norm(view_dir))
        if n <= 1e-12:
            return False
        return _is_culled(meshlet, view_dir / n, cone_source)
    if cone_source == "visibility":
        if meshlet.visibility_axis is None:
            return False
        # EmberGL/Meshlete convention: test from an anchor point behind the
        # meshlet bounding sphere, not simply from the sphere center.
        anchor = meshlet.center - meshlet.visibility_axis * meshlet.radius
        camera_dir = camera_pos - anchor
        n = float(np.linalg.norm(camera_dir))
        if n <= 1e-12:
            return False
        camera_dir /= n
        return float(np.dot(meshlet.visibility_axis, camera_dir)) < meshlet.visibility_cos
    raise ValueError(f"unknown cull cone source: {cone_source}")
