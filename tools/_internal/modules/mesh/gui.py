#!/usr/bin/env python
from __future__ import annotations

import subprocess
import re
import sys
import threading
import tkinter as tk
from pathlib import Path
from tkinter import colorchooser, filedialog, font as tkfont, messagebox, ttk

from PIL import Image, ImageTk

REPO_ROOT = Path(__file__).resolve().parents[4]
TOOLS_ROOT = Path(__file__).resolve().parents[3]
if str(REPO_ROOT) not in sys.path:
    sys.path.insert(0, str(REPO_ROOT))
if str(TOOLS_ROOT) not in sys.path:
    sys.path.insert(0, str(TOOLS_ROOT))

from _internal.modules.image.core import SUPPORTED_LAYOUTS, SUPPORTED_RESAMPLING, parse_resize, sanitize_identifier
from _internal.modules.setup.checks import check_environment
from _internal.modules.mesh.textures import DIFFUSE_ROLES, EMISSIVE_ROLES, IMAGE_EXTENSIONS, resolve_mesh_textures
from tools._internal.modules.mesh_pipeline.mesh3d_to_mesh3d2 import legacy_to_objmesh, parse_legacy_header
from tools._internal.modules.mesh_pipeline.mesh_inspect import _parse_texture_headers, _parse_texture_image, detect_mesh_type, parse_mesh3d2_header
from tools._internal.modules.mesh_pipeline.mesh import Material
from tools._internal.modules.mesh_pipeline.obj_loader import load_obj
from tools._internal.modules.mesh_pipeline.profiles import DEFAULT_MAX_NORMAL_ANGLE_DEG, DEFAULT_TARGET_VERTICES


COLOR_TYPES = ("RGB565", "RGB24", "RGB32")
RESIZE_VALUES = ("", "4", "8", "16", "32", "64", "128", "256", "512", "1024", "2048")
LAYOUT_CHOICES = ("header (.h)", "split (.h + .cpp)")
LAYOUT_TO_CLI = {
    "header (.h)": "header",
    "split (.h + .cpp)": "split",
    "header": "header",
    "split": "split",
}
TEXTURE_ROLE_CHANNELS = {role: "Kd" for role in DIFFUSE_ROLES}
TEXTURE_ROLE_CHANNELS.update({role: "Ke" for role in EMISSIVE_ROLES})
TGX_SOURCE_EXTENSIONS = (".h", ".hpp", ".cpp", ".cxx", ".cc")
DEFAULT_MATERIAL_COLOR = (0.75, 0.75, 0.75)
DEFAULT_EMISSIVE_COLOR = (0.0, 0.0, 0.0)
DEFAULT_AMBIANT_STRENGTH = 0.1
DEFAULT_DIFFUSE_STRENGTH = 0.7
DEFAULT_SPECULAR_STRENGTH = 0.6
DEFAULT_SPECULAR_EXPONENT = 32
DEFAULT_EMISSIVE_STRENGTH = 0.0


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


class ToolTip:
    def __init__(self, widget: tk.Widget, text: str, delay_ms: int = 450):
        self.widget = widget
        self.text = text
        self.delay_ms = delay_ms
        self.after_id: str | None = None
        self.window: tk.Toplevel | None = None
        widget.bind("<Enter>", self._schedule, add="+")
        widget.bind("<Leave>", self._hide, add="+")
        widget.bind("<ButtonPress>", self._hide, add="+")

    def _schedule(self, _event=None):
        self._cancel()
        self.after_id = self.widget.after(self.delay_ms, self._show)

    def _cancel(self):
        if self.after_id is not None:
            self.widget.after_cancel(self.after_id)
            self.after_id = None

    def _show(self):
        self.after_id = None
        if self.window is not None or not self.text:
            return
        x = self.widget.winfo_rootx() + 18
        y = self.widget.winfo_rooty() + self.widget.winfo_height() + 8
        self.window = tk.Toplevel(self.widget)
        self.window.wm_overrideredirect(True)
        self.window.wm_geometry(f"+{x}+{y}")
        label = tk.Label(
            self.window,
            text=self.text,
            justify="left",
            background="#ffffe8",
            foreground="#202020",
            relief="solid",
            borderwidth=1,
            padx=6,
            pady=4,
            wraplength=360,
        )
        label.pack()

    def _hide(self, _event=None):
        self._cancel()
        if self.window is not None:
            self.window.destroy()
            self.window = None


class MaterialTable(ttk.Frame):
    def __init__(self, master: tk.Widget, columns: tuple[tuple[str, str, int, bool], ...], *, color_columns: set[str], height: int = 7):
        super().__init__(master)
        self.columns = columns
        self.color_columns = color_columns
        self.row_height = 26
        self.header_height = 24
        self.items: dict[str, tuple[object, ...]] = {}
        self.order: list[str] = []
        self.selected_iid: str | None = None
        self.font = tkfont.nametofont("TkDefaultFont")

        self.columnconfigure(0, weight=1)
        self.rowconfigure(1, weight=1)
        self.header = tk.Canvas(self, height=self.header_height, highlightthickness=0, background="#ececec")
        self.header.grid(row=0, column=0, sticky="ew")
        self.body = tk.Canvas(self, height=max(1, height) * self.row_height, highlightthickness=1, highlightbackground="#b8b8b8", background="#ffffff")
        self.body.grid(row=1, column=0, sticky="nsew")
        self.scrollbar = ttk.Scrollbar(self, orient="vertical", command=self.body.yview)
        self.scrollbar.grid(row=1, column=1, sticky="ns")
        self.body.configure(yscrollcommand=self.scrollbar.set)

        self.header.bind("<Configure>", lambda _event: self._redraw())
        self.body.bind("<Configure>", lambda _event: self._redraw())
        self.body.bind("<Button-1>", self._on_click, add="+")
        for widget in (self.header, self.body):
            widget.bind("<MouseWheel>", self._on_mousewheel, add="+")
            widget.bind("<Button-4>", self._on_mousewheel, add="+")
            widget.bind("<Button-5>", self._on_mousewheel, add="+")

    def bind(self, sequence=None, func=None, add=None):
        if sequence is not None and func is not None:
            self.body.bind(sequence, func, add=add)
        return super().bind(sequence, func, add=add)

    def get_children(self):
        return tuple(self.order)

    def delete(self, *iids: str) -> None:
        if not iids:
            return
        for iid in iids:
            self.items.pop(iid, None)
            if iid in self.order:
                self.order.remove(iid)
            if self.selected_iid == iid:
                self.selected_iid = None
        self._redraw()

    def insert(self, _parent: str, _index: str, *, iid: str, values: tuple[object, ...], **_kwargs) -> str:
        self.items[iid] = values
        if iid not in self.order:
            self.order.append(iid)
        self._redraw()
        return iid

    def item(self, iid: str, **kwargs):
        if "values" in kwargs:
            self.items[iid] = tuple(kwargs["values"])
            self._redraw()
        return {"values": self.items.get(iid, ())}

    def selection(self):
        if self.selected_iid and self.selected_iid in self.items:
            return (self.selected_iid,)
        return ()

    def selection_set(self, iid: str) -> None:
        if iid in self.items:
            self.selected_iid = iid
            self._redraw()

    def see(self, iid: str) -> None:
        if iid not in self.order:
            return
        total = max(1, len(self.order) * self.row_height)
        index = self.order.index(iid)
        self.body.yview_moveto(max(0.0, min(1.0, (index * self.row_height) / total)))

    def _on_click(self, event) -> None:
        y = self.body.canvasy(event.y)
        index = int(y // self.row_height)
        if 0 <= index < len(self.order):
            self.selected_iid = self.order[index]
            self._redraw()

    def _on_mousewheel(self, event) -> str:
        if getattr(event, "num", None) == 4:
            units = -3
        elif getattr(event, "num", None) == 5:
            units = 3
        else:
            delta = getattr(event, "delta", 0)
            units = -int(delta / 120) if delta else 0
            if units == 0 and delta:
                units = -1 if delta > 0 else 1
        if units:
            self.body.yview_scroll(units, "units")
        return "break"

    def _column_layout(self, width: int) -> list[tuple[str, str, int, int]]:
        base = sum(column_width for _column, _label, column_width, _stretch in self.columns)
        stretch_columns = sum(1 for _column, _label, _width, stretch in self.columns if stretch)
        extra = max(0, width - base)
        extra_each = extra // stretch_columns if stretch_columns else 0
        x = 0
        layout: list[tuple[str, str, int, int]] = []
        for column, label, column_width, stretch in self.columns:
            width_px = column_width + (extra_each if stretch else 0)
            layout.append((column, label, x, width_px))
            x += width_px
        return layout

    def _fit_text(self, text: object, width_px: int) -> str:
        value = str(text)
        if width_px <= 8:
            return ""
        if self.font.measure(value) <= width_px:
            return value
        suffix = "..."
        limit = max(0, width_px - self.font.measure(suffix))
        while value and self.font.measure(value) > limit:
            value = value[:-1]
        return value + suffix if value else ""

    def _draw_color_cell(self, x: int, y: int, width_px: int, value: object) -> None:
        text = str(value)
        parts = text.split(maxsplit=1)
        hex_color = parts[0] if parts and self._is_hex_color(parts[0]) else ""
        label = parts[1] if hex_color and len(parts) > 1 else text
        swatch_x = x + 6
        swatch_y = y + 6
        if hex_color:
            self.body.create_rectangle(swatch_x, swatch_y, swatch_x + 14, swatch_y + 14, fill=hex_color, outline="#5f5f5f")
        else:
            self.body.create_rectangle(swatch_x, swatch_y, swatch_x + 14, swatch_y + 14, fill="#d0d0d0", outline="#8a8a8a")
            self.body.create_line(swatch_x + 3, swatch_y + 3, swatch_x + 11, swatch_y + 11, fill="#8a8a8a")
        text_x = x + 26
        self.body.create_text(text_x, y + self.row_height // 2, text=self._fit_text(label, width_px - 32), anchor="w", fill="#202020", font=self.font)

    @staticmethod
    def _is_hex_color(text: str) -> bool:
        if len(text) != 7 or not text.startswith("#"):
            return False
        return all(char in "0123456789abcdefABCDEF" for char in text[1:])

    def _redraw(self) -> None:
        width = max(self.winfo_width(), sum(column_width for _column, _label, column_width, _stretch in self.columns))
        layout = self._column_layout(width)
        total_width = sum(column_width for _column, _label, _x, column_width in layout)
        total_height = max(len(self.order) * self.row_height, int(self.body.winfo_height()))
        self.header.delete("all")
        self.body.delete("all")
        self.header.configure(scrollregion=(0, 0, total_width, self.header_height))
        self.body.configure(scrollregion=(0, 0, total_width, total_height))

        for column, label, x, column_width in layout:
            self.header.create_rectangle(x, 0, x + column_width, self.header_height, fill="#ececec", outline="#c2c2c2")
            self.header.create_text(x + 6, self.header_height // 2, text=self._fit_text(label, column_width - 12), anchor="w", fill="#202020", font=self.font)

        for row_index, iid in enumerate(self.order):
            row_y = row_index * self.row_height
            fill = "#dbeafe" if iid == self.selected_iid else ("#ffffff" if row_index % 2 == 0 else "#f7f7f7")
            self.body.create_rectangle(0, row_y, total_width, row_y + self.row_height, fill=fill, outline="#e1e1e1")
            values = self.items.get(iid, ())
            for value_index, (column, _label, x, column_width) in enumerate(layout):
                self.body.create_line(x, row_y, x, row_y + self.row_height, fill="#e1e1e1")
                value = values[value_index] if value_index < len(values) else ""
                if column in self.color_columns:
                    self._draw_color_cell(x, row_y, column_width, value)
                else:
                    self.body.create_text(x + 6, row_y + self.row_height // 2, text=self._fit_text(value, column_width - 12), anchor="w", fill="#202020", font=self.font)
        if layout:
            last_x = layout[-1][2] + layout[-1][3]
            self.body.create_line(last_x, 0, last_x, total_height, fill="#e1e1e1")


class TGXMeshConverterApp(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("TGX Mesh Converter")
        self.minsize(1120, 720)

        self.input_path = tk.StringVar()
        self.output_path = tk.StringVar()
        self.object_name = tk.StringVar()
        self.mesh_format = tk.StringVar(value="Mesh3Dv2")
        self.layout = tk.StringVar(value="header (.h)")
        self.color_type = tk.StringVar(value="RGB565")
        self.normalize = tk.BooleanVar(value=True)
        self.meshlet_mode = tk.StringVar(value="compact")
        self.compact_compression = tk.IntVar(value=0)
        self.target_vertices = tk.StringVar(value=str(DEFAULT_TARGET_VERTICES))
        self.max_normal_angle = tk.StringVar(value=str(DEFAULT_MAX_NORMAL_ANGLE_DEG))
        self.texture_layout = tk.StringVar(value="header (.h)")
        self.force_normals = tk.BooleanVar(value=False)
        self.remove_normals = tk.BooleanVar(value=False)
        self.meshlet_mode_widgets: list[tk.Widget] = []
        self.classic_meshlet_widgets: list[tk.Widget] = []
        self.compact_meshlet_widgets: list[tk.Widget] = []

        self.material_rows: dict[str, dict[str, str]] = {}
        self.material_iid_to_key: dict[str, str] = {}
        self.material_key_to_iid: dict[str, str] = {}
        self.texture_assets: dict[str, dict[str, str]] = {}
        self.texture_iid_to_id: dict[str, str] = {}
        self.texture_id_to_iid: dict[str, str] = {}
        self.texture_key_to_id: dict[str, str] = {}
        self.next_texture_id = 1
        self.texture_preview_image: ImageTk.PhotoImage | None = None
        self.conversion_running = False
        self.texture_size_icons = self._make_texture_size_icons()
        self.mesh_info_vars: dict[str, tk.StringVar] = {}
        self.mesh_info_value_labels: dict[str, ttk.Label] = {}
        self.mesh_size_indicator: tk.Canvas | None = None
        self._tooltips: list[ToolTip] = []

        self._build_ui()
        self.input_path.trace_add("write", lambda *_args: self._update_mesh_actions())
        self.mesh_format.trace_add("write", lambda *_args: self._update_meshlet_options_state())
        self.meshlet_mode.trace_add("write", lambda *_args: self._update_meshlet_options_state())
        self._update_mesh_actions()
        self._update_meshlet_options_state()

    def _tip(self, widget: tk.Widget, text: str) -> None:
        self._tooltips.append(ToolTip(widget, text))

    def _build_ui(self):
        root = ttk.Frame(self, padding=12)
        root.grid(row=0, column=0, sticky="nsew")
        self.columnconfigure(0, weight=1)
        self.rowconfigure(0, weight=1)
        root.columnconfigure(1, weight=1)
        root.rowconfigure(8, weight=3)
        root.rowconfigure(10, weight=2)
        root.rowconfigure(13, weight=1)

        input_label = ttk.Label(root, text="Input OBJ / TGX mesh")
        input_label.grid(row=0, column=0, sticky="w")
        self._tip(input_label, "OBJ file, legacy Mesh3D header, or Mesh3Dv2 header to convert.")
        input_entry = ttk.Entry(root, textvariable=self.input_path)
        input_entry.grid(row=0, column=1, columnspan=3, sticky="ew", padx=6)
        browse_input = ttk.Button(root, text="Browse...", command=self.choose_input)
        browse_input.grid(row=0, column=4)
        self._tip(browse_input, "Choose the source mesh file.")

        output_label = ttk.Label(root, text="Output mesh .h")
        output_label.grid(row=1, column=0, sticky="w")
        self._tip(output_label, "Header path for the generated TGX mesh. Split layout also creates a matching .cpp file.")
        output_entry = ttk.Entry(root, textvariable=self.output_path)
        output_entry.grid(row=1, column=1, columnspan=3, sticky="ew", padx=6)
        browse_output = ttk.Button(root, text="Browse...", command=self.choose_output)
        browse_output.grid(row=1, column=4)
        self._tip(browse_output, "Choose where to write the generated mesh header.")

        object_label = ttk.Label(root, text="Object name")
        object_label.grid(row=2, column=0, sticky="w")
        self._tip(object_label, "C++ symbol name for the generated mesh object.")
        ttk.Entry(root, textvariable=self.object_name).grid(row=2, column=1, columnspan=3, sticky="ew", padx=6)

        format_box = ttk.Frame(root)
        format_box.grid(row=3, column=0, columnspan=5, sticky="ew")
        format_box.columnconfigure(1, minsize=140)
        format_box.columnconfigure(3, minsize=140)
        format_box.columnconfigure(5, minsize=140)
        format_box.columnconfigure(7, minsize=140)
        format_box.columnconfigure(8, weight=1)
        format_label = ttk.Label(format_box, text="Output format")
        format_label.grid(row=0, column=0, sticky="w")
        self._tip(format_label, "Mesh3Dv2 is the recommended compact runtime format. Mesh3D keeps the legacy format.")
        format_combo = ttk.Combobox(format_box, textvariable=self.mesh_format, values=("Mesh3Dv2", "Mesh3D"), state="readonly", width=12)
        format_combo.grid(row=0, column=1, sticky="w", padx=(6, 20))
        color_label = ttk.Label(format_box, text="Color")
        color_label.grid(row=0, column=2, sticky="w")
        self._tip(color_label, "Pixel type used by materials and generated textures. RGB565 is recommended on embedded boards.")
        ttk.Combobox(format_box, textvariable=self.color_type, values=COLOR_TYPES, state="readonly", width=12).grid(row=0, column=3, sticky="w", padx=(6, 20))
        mesh_files_label = ttk.Label(format_box, text="Mesh files")
        mesh_files_label.grid(row=0, column=4, sticky="w")
        self._tip(mesh_files_label, "header creates one .h file. split creates a small .h plus a .cpp with arrays.")
        ttk.Combobox(format_box, textvariable=self.layout, values=LAYOUT_CHOICES, state="readonly", width=12).grid(row=0, column=5, sticky="w", padx=(6, 20))
        texture_files_label = ttk.Label(format_box, text="Texture files")
        texture_files_label.grid(row=0, column=6, sticky="w")
        self._tip(texture_files_label, "Layout used for generated texture images referenced by the mesh.")
        ttk.Combobox(format_box, textvariable=self.texture_layout, values=LAYOUT_CHOICES, state="readonly", width=12).grid(row=0, column=7, sticky="w", padx=(6, 0))
        self.lkh_warning = ttk.Label(format_box, text="", foreground="red")
        self.lkh_warning.grid(row=1, column=0, columnspan=9, sticky="w", pady=(4, 0))
        self._update_lkh_warning()

        info_box = ttk.LabelFrame(root, text="Mesh info", padding=(8, 4))
        info_box.grid(row=4, column=3, columnspan=2, rowspan=2, sticky="nsew", padx=(12, 0), pady=(4, 0))
        for column_index in (2, 4, 6):
            info_box.columnconfigure(column_index, weight=1)
        self.mesh_size_indicator = tk.Canvas(info_box, width=16, height=16, highlightthickness=0)
        self.mesh_size_indicator.grid(row=0, column=0, rowspan=2, sticky="n", padx=(0, 8), pady=(2, 0))
        for index, key in enumerate(("type", "vertices", "triangles", "uv", "normals", "materials")):
            row_index = index // 3
            column_index = 1 + (index % 3) * 2
            self.mesh_info_vars[key] = tk.StringVar(value="-")
            ttk.Label(info_box, text=key).grid(row=row_index, column=column_index, sticky="w", padx=(0, 4))
            label = ttk.Label(info_box, textvariable=self.mesh_info_vars[key], anchor="e")
            label.grid(row=row_index, column=column_index + 1, sticky="e", padx=(0, 10))
            self.mesh_info_value_labels[key] = label

        normal_box = ttk.Frame(root)
        normal_box.grid(row=4, column=0, columnspan=3, sticky="w", pady=(8, 0))
        force_normals = ttk.Checkbutton(normal_box, text="Force smooth normal recomputation", variable=self.force_normals, command=self._on_force_normals)
        force_normals.pack(side="left")
        self._tip(force_normals, "Ignore existing normals and rebuild smooth vertex normals.")
        remove_normals = ttk.Checkbutton(normal_box, text="Remove normals", variable=self.remove_normals, command=self._on_remove_normals)
        remove_normals.pack(side="left", padx=(18, 0))
        self._tip(remove_normals, "Export a mesh without normals. This reduces data and disables Gouraud shading for this mesh.")

        normalize_check = ttk.Checkbutton(root, text="Normalize unit bounding box", variable=self.normalize)
        normalize_check.grid(row=5, column=0, columnspan=3, sticky="w", pady=(8, 0))
        self._tip(normalize_check, "Center and scale OBJ input so the mesh fits inside the [-1,1]^3 box.")

        meshlet_box = ttk.LabelFrame(root, text="Mesh3Dv2 meshlet generation", padding=(8, 6))
        meshlet_box.grid(row=6, column=0, columnspan=5, sticky="ew", pady=(10, 2))
        meshlet_box.columnconfigure(5, weight=1)
        compact_radio = ttk.Radiobutton(meshlet_box, text="Compact", variable=self.meshlet_mode, value="compact")
        compact_radio.grid(row=0, column=0, sticky="w")
        self._tip(compact_radio, "Default mode. Builds larger meshlets to reduce memory. Often the best choice on embedded boards.")
        compact_label = ttk.Label(meshlet_box, text="Non-adjacent packing")
        compact_label.grid(row=0, column=1, sticky="w", padx=(16, 4))
        self._tip(compact_label, "Extra pass that may merge distant meshlets. In general, 0 is the best choice.")
        compact_scale = ttk.Scale(meshlet_box, variable=self.compact_compression, from_=0, to=100, orient="horizontal", length=180)
        compact_scale.grid(row=0, column=2, sticky="w")
        self._tip(compact_scale, "0 is usually best. Increase only when smaller flash size matters more than culling quality.")
        compact_value = ttk.Label(meshlet_box, textvariable=self.compact_compression, width=4, anchor="e")
        compact_value.grid(row=0, column=3, sticky="w", padx=(4, 0))
        self.compact_meshlet_widgets.extend([compact_label, compact_scale, compact_value])

        classic_radio = ttk.Radiobutton(meshlet_box, text="Visibility culling", variable=self.meshlet_mode, value="visibility")
        classic_radio.grid(row=1, column=0, sticky="w", pady=(6, 0))
        self._tip(classic_radio, "Classic visibility-optimized mode. Usually creates more meshlets but better culling.")
        target_label = ttk.Label(meshlet_box, text="target vertices")
        target_label.grid(row=1, column=1, sticky="w", padx=(16, 4), pady=(6, 0))
        self._tip(target_label, "Preferred meshlet vertex count in visibility culling mode.")
        target_entry = ttk.Entry(meshlet_box, textvariable=self.target_vertices, width=6)
        target_entry.grid(row=1, column=2, sticky="w", pady=(6, 0))
        angle_label = ttk.Label(meshlet_box, text="max normal angle")
        angle_label.grid(row=1, column=3, sticky="w", padx=(18, 4), pady=(6, 0))
        self._tip(angle_label, "Maximum normal spread accepted while merging meshlets in visibility culling mode.")
        angle_entry = ttk.Entry(meshlet_box, textvariable=self.max_normal_angle, width=6)
        angle_entry.grid(row=1, column=4, sticky="w", pady=(6, 0))
        self.classic_meshlet_widgets.extend([target_label, target_entry, angle_label, angle_entry])
        self.meshlet_mode_widgets.extend([compact_radio, classic_radio])

        material_header = ttk.Frame(root)
        material_header.grid(row=7, column=0, columnspan=5, sticky="ew", pady=(10, 2))
        material_header.columnconfigure(0, weight=1)
        material_label = ttk.Label(material_header, text="Materials")
        material_label.grid(row=0, column=0, sticky="w")
        self._tip(material_label, "Double-click a material to edit colors, strengths, and texture links.")
        self.reload_button = ttk.Button(material_header, text="Reload assets", command=self.reload_textures)
        self.reload_button.grid(row=0, column=1, sticky="e")
        self._tip(self.reload_button, "Rescan materials and texture references from the selected input mesh.")

        material_columns = (
            ("name", "name", 130, True),
            ("mode", "mode", 80, False),
            ("triangles", "tris", 70, False),
            ("diffuse_color", "diffuse color", 165, False),
            ("lighting", "ka/kd/ks", 120, False),
            ("exp", "exp", 55, False),
            ("diffuse_texture", "diffuse texture", 145, True),
            ("emissive_color", "emissive color", 165, False),
            ("emissive_strength", "strength", 80, False),
            ("emissive_texture", "emissive texture", 145, True),
        )
        self.material_table = MaterialTable(root, material_columns, color_columns={"diffuse_color", "emissive_color"}, height=7)
        self.material_table.grid(row=8, column=0, columnspan=5, sticky="nsew")
        self.material_table.bind("<Double-1>", self.edit_selected_material)

        texture_header = ttk.Frame(root)
        texture_header.grid(row=9, column=0, columnspan=5, sticky="ew", pady=(8, 2))
        texture_header.columnconfigure(0, weight=1)
        texture_label = ttk.Label(texture_header, text="Textures")
        texture_label.grid(row=0, column=0, sticky="w")
        self._tip(texture_label, "Global texture list. Materials point to these named textures.")
        add_texture = ttk.Button(texture_header, text="+", width=3, command=self.add_texture_asset)
        add_texture.grid(row=0, column=1, sticky="e", padx=(0, 4))
        self._tip(add_texture, "Add a texture from an image or TGX texture header.")
        remove_texture = ttk.Button(texture_header, text="-", width=3, command=self.remove_selected_texture)
        remove_texture.grid(row=0, column=2, sticky="e")
        self._tip(remove_texture, "Remove the selected texture and unlink it from all materials.")

        texture_columns = ("name", "size", "resize", "filter", "users", "path")
        texture_table_box = ttk.Frame(root)
        texture_table_box.grid(row=10, column=0, columnspan=5, sticky="nsew")
        texture_table_box.columnconfigure(0, weight=1)
        texture_table_box.rowconfigure(0, weight=1)
        self.texture_table = ttk.Treeview(texture_table_box, columns=texture_columns, show=("tree", "headings"), height=6)
        self.texture_scrollbar = ttk.Scrollbar(texture_table_box, orient="vertical", command=self.texture_table.yview)
        self.texture_table.configure(yscrollcommand=self.texture_scrollbar.set)
        self.texture_table.heading("#0", text="")
        self.texture_table.column("#0", width=42, minwidth=42, stretch=False, anchor="center")
        for column, label, width in (
            ("name", "name", 150),
            ("size", "size", 90),
            ("resize", "resize", 90),
            ("filter", "filter", 80),
            ("users", "used by", 180),
            ("path", "path", 420),
        ):
            self.texture_table.heading(column, text=label)
            self.texture_table.column(column, width=width, stretch=column in ("users", "path"))
        self.texture_table.grid(row=0, column=0, sticky="nsew")
        self.texture_scrollbar.grid(row=0, column=1, sticky="ns")
        self.texture_table.bind("<Double-1>", self.edit_selected_texture)

        buttons = ttk.Frame(root)
        buttons.grid(row=11, column=0, columnspan=5, sticky="ew", pady=(8, 0))
        action_buttons = ttk.Frame(buttons)
        action_buttons.pack(anchor="center")
        self.preview_button = ttk.Button(action_buttons, text="Preview mesh", command=self.preview_mesh)
        self.preview_button.pack(side="left", padx=(0, 8))
        self._tip(self.preview_button, "Open a PyVista preview of the current mesh and selected textures.")
        self.convert_button = tk.Button(
            action_buttons,
            text="Convert",
            command=self.convert,
            padx=18,
            pady=2,
            relief="raised",
            borderwidth=1,
        )
        self.convert_button.pack(side="left")
        self._tip(self.convert_button, "Run the converter and write the generated TGX files.")

        ttk.Label(root, text="Log").grid(row=12, column=0, sticky="w", pady=(8, 0))
        self.log = tk.Text(root, height=7, wrap="word")
        self.log.grid(row=13, column=0, columnspan=5, sticky="nsew", pady=(2, 0))

    def _on_force_normals(self):
        if self.force_normals.get():
            self.remove_normals.set(False)

    def _on_remove_normals(self):
        if self.remove_normals.get():
            self.force_normals.set(False)

    def _update_meshlet_options_state(self):
        is_mesh3dv2 = self.mesh_format.get() == "Mesh3Dv2"
        use_compact = self.meshlet_mode.get() == "compact"
        for widget in self.meshlet_mode_widgets:
            widget.configure(state="normal" if is_mesh3dv2 else "disabled")
        for widget in self.compact_meshlet_widgets:
            widget.configure(state="normal" if is_mesh3dv2 and use_compact else "disabled")
        for widget in self.classic_meshlet_widgets:
            widget.configure(state="normal" if is_mesh3dv2 and not use_compact else "disabled")

    def _update_lkh_warning(self):
        status = check_environment(tool="mesh", require_config=True)
        if status.lkh_missing:
            self.lkh_warning.configure(text="LKH not installed: strip quality may be lower.")
        else:
            self.lkh_warning.configure(text="")

    def _mesh_is_loaded(self) -> bool:
        path_text = self.input_path.get().strip()
        return bool(path_text) and Path(path_text).is_file()

    def _update_mesh_actions(self):
        mesh_state = "normal" if self._mesh_is_loaded() else "disabled"
        convert_state = "disabled" if self.conversion_running else mesh_state
        for button in (getattr(self, "reload_button", None), getattr(self, "preview_button", None)):
            if button is not None:
                button.configure(state=mesh_state)
        if getattr(self, "convert_button", None) is not None:
            self._set_convert_button_state(convert_state)

    def _set_convert_button_state(self, state: str):
        if state == "normal":
            self.convert_button.configure(
                state="normal",
                background="#23823a",
                foreground="white",
                activebackground="#1b6d30",
                activeforeground="white",
                disabledforeground="#6b6b6b",
            )
        else:
            self.convert_button.configure(
                state="disabled",
                background="#c9d2c9",
                foreground="#6b6b6b",
                activebackground="#c9d2c9",
                activeforeground="#6b6b6b",
                disabledforeground="#6b6b6b",
            )

    def _texture_size_text(self, path_text: str, symbol: str = "") -> str:
        if not path_text:
            return ""
        path = Path(path_text)
        if not path.exists():
            return "missing"
        if path.suffix.lower() in TGX_SOURCE_EXTENSIONS:
            symbol = symbol or _first_tgx_image_symbol(path)
            if not symbol:
                return "?"
            texture = _parse_texture_image(path, symbol)
            if texture is None:
                return "?"
            return f"{texture.width}x{texture.height}"
        try:
            with Image.open(path) as img:
                return f"{img.width}x{img.height}"
        except Exception:
            return "?"

    def _make_texture_size_icons(self) -> dict[str, tk.PhotoImage]:
        colors = {
            "asset_material": "#8c8c8c",
            "tex_small": "#23a441",
            "tex_medium": "#2f75b5",
            "tex_large": "#e2b400",
            "tex_huge": "#d33333",
            "tex_none": "#b0b0b0",
        }
        icons: dict[str, tk.PhotoImage] = {}
        for name, color in colors.items():
            image = tk.PhotoImage(width=14, height=14)
            image.put(color, to=(1, 1, 13, 13))
            image.put("#666666", to=(0, 0, 14, 1))
            image.put("#666666", to=(0, 13, 14, 14))
            image.put("#666666", to=(0, 0, 1, 14))
            image.put("#666666", to=(13, 0, 14, 14))
            icons[name] = image
        return icons

    def _texture_size_class(self, size_text: str) -> str:
        try:
            width_text, height_text = size_text.lower().split("x", 1)
            width = int(width_text)
            height = int(height_text)
        except Exception:
            return "tex_huge" if size_text else "tex_none"
        largest = max(width, height)
        if largest <= 64:
            return "tex_small"
        if largest <= 256:
            return "tex_medium"
        if largest <= 1024:
            return "tex_large"
        return "tex_huge"

    def _texture_effective_size(self, row: dict[str, str]) -> str:
        return row["resize"] or row["size"]

    def _texture_size_display(self, row: dict[str, str]) -> str:
        if row["resize"]:
            return f"{row['size']} -> {row['resize']}"
        return row["size"]

    def _texture_size_icon(self, row: dict[str, str]) -> tk.PhotoImage:
        return self.texture_size_icons[self._texture_size_class(self._texture_effective_size(row))]

    def _display_texture_role(self, role: str) -> str:
        return TEXTURE_ROLE_CHANNELS.get(role.lower(), role)

    def _display_texture_refs(self, refs: dict[str, Path], roles: tuple[str, ...]) -> str:
        role_set = {role.lower() for role in roles}
        return ", ".join(
            f"{self._display_texture_role(role)}:{path.name}"
            for role, path in sorted(refs.items())
            if role.lower() in role_set
        )

    def _format_float(self, value: float) -> str:
        text = f"{float(value):.3g}"
        return "0" if text == "-0" else text

    def _format_material_color(self, values: tuple[float, ...] | None, *, strength: float | None = None) -> str:
        if values is None:
            return ""
        try:
            color = ", ".join(self._format_float(value) for value in values[:3])
        except Exception:
            return ""
        if strength is not None:
            color += f" * {self._format_float(strength)}"
        return color

    def _material_tuple(self, desc: object | None, attr: str, default: tuple[float, float, float]) -> tuple[float, float, float]:
        values = getattr(desc, attr, default) if desc is not None else default
        try:
            return (float(values[0]), float(values[1]), float(values[2]))
        except Exception:
            return default

    def _material_float(self, desc: object | None, attr: str, default: float) -> float:
        try:
            return float(getattr(desc, attr, default) if desc is not None else default)
        except Exception:
            return default

    def _material_int(self, desc: object | None, attr: str, default: int) -> int:
        try:
            return int(round(float(getattr(desc, attr, default) if desc is not None else default)))
        except Exception:
            return default

    def _is_nonzero_color(self, values: tuple[float, ...] | None) -> bool:
        if values is None:
            return False
        try:
            return any(abs(float(value)) > 1e-9 for value in values[:3])
        except Exception:
            return False

    def _material_has_emissive_data(
        self,
        desc: object | None,
        *,
        selected: str = "",
        requested: str = "",
        refs: str = "",
        symbol: str = "",
    ) -> bool:
        if selected or requested or refs or symbol:
            return True
        if desc is None:
            return False
        emissive = getattr(desc, "emissive", getattr(desc, "emissive_color", None))
        if self._is_nonzero_color(emissive):
            return True
        try:
            if abs(float(getattr(desc, "emissive_strength", 0.0))) > 1e-9:
                return True
        except Exception:
            pass
        return bool(
            getattr(desc, "emissive_texture_path", None)
            or getattr(desc, "emissive_texture_symbol", "")
            or getattr(desc, "material_extra_present", False)
            or getattr(desc, "material_extra_flags", 0)
        )

    def _texture_choice_has_data(self, selected: str, requested: str, refs: str, symbol: str = "") -> bool:
        return bool(selected or requested or refs or symbol)

    def _add_nearby_image_assets(self, folder: Path) -> int:
        images = [
            item
            for item in sorted(folder.iterdir(), key=lambda path: path.name.lower())
            if item.is_file() and item.suffix.lower() in IMAGE_EXTENSIONS
        ]
        added = 0
        for image in images[:32]:
            before = len(self.texture_assets)
            self._add_texture_asset(str(image), name=image.stem)
            if len(self.texture_assets) > before:
                added += 1
        return added

    def _obj_material_triangle_counts(self, mesh) -> dict[str, int]:
        counts: dict[str, int] = {}
        for tri in mesh.triangles:
            counts[tri.material] = counts.get(tri.material, 0) + 1
        if not counts:
            counts[""] = 0
        return counts

    def _decoded_mesh3d2_material_triangle_counts(self, mesh) -> dict[str, int]:
        counts: dict[str, int] = {}
        for meshlet in mesh.meshlets:
            if meshlet.material_index < 0 or meshlet.material_index >= len(mesh.materials):
                name = ""
            else:
                name = mesh.materials[meshlet.material_index].name or f"mat{meshlet.material_index}"
            counts[name] = counts.get(name, 0) + len(meshlet.triangles)
        if not counts:
            counts[""] = 0
        return counts

    def _unique_name(self, wanted: str, existing: set[str], *, fallback: str) -> str:
        base = sanitize_identifier(wanted or fallback) or fallback
        if base not in existing:
            return base
        index = 2
        while f"{base}_{index}" in existing:
            index += 1
        return f"{base}_{index}"

    def _unique_texture_name(self, wanted: str, *, exclude_id: str = "") -> str:
        existing = {item["name"] for texture_id, item in self.texture_assets.items() if texture_id != exclude_id}
        return self._unique_name(wanted, existing, fallback="texture")

    def _unique_material_name(self, wanted: str, *, exclude_key: str = "") -> str:
        existing = {item["name"] for key, item in self.material_rows.items() if key != exclude_key}
        return self._unique_name(wanted, existing, fallback="material")

    def _texture_key(self, path_text: str, symbol: str = "") -> str:
        if path_text:
            try:
                return "path:" + str(Path(path_text).expanduser().resolve()).lower()
            except Exception:
                return "path:" + path_text.lower()
        return "symbol:" + symbol

    def _add_texture_asset(self, path_text: str, *, name: str = "", symbol: str = "") -> str:
        if path_text and not symbol:
            path = Path(path_text)
            if path.suffix.lower() in TGX_SOURCE_EXTENSIONS:
                symbol = _first_tgx_image_symbol(path)
        key = self._texture_key(path_text, symbol)
        existing = self.texture_key_to_id.get(key)
        if existing:
            return existing
        texture_id = f"tex{self.next_texture_id}"
        self.next_texture_id += 1
        path = Path(path_text) if path_text else Path(name or symbol or "texture")
        texture_name = self._unique_texture_name(name or path.stem or symbol or texture_id)
        self.texture_assets[texture_id] = {
            "id": texture_id,
            "name": texture_name,
            "path": path_text,
            "symbol": symbol,
            "size": self._texture_size_text(path_text, symbol),
            "resize": "",
            "resize_w": "",
            "resize_h": "",
            "filter": "lanczos",
        }
        self.texture_key_to_id[key] = texture_id
        self._insert_texture_row(texture_id)
        return texture_id

    def _texture_name(self, texture_id: str) -> str:
        if not texture_id:
            return "none"
        return self.texture_assets.get(texture_id, {}).get("name", "missing")

    def _texture_users(self, texture_id: str) -> list[str]:
        users: list[str] = []
        for row in self.material_rows.values():
            label = row["name"] or "[default]"
            if row.get("diffuse_texture") == texture_id:
                users.append(f"{label}:diffuse")
            if row.get("emissive_texture") == texture_id:
                users.append(f"{label}:emissive")
        return users

    def _texture_users_text(self, texture_id: str) -> str:
        return ", ".join(self._texture_users(texture_id))

    def _material_color_cell(self, row: dict[str, str], prefix: str) -> str:
        red = row[f"{prefix}_r"]
        green = row[f"{prefix}_g"]
        blue = row[f"{prefix}_b"]
        try:
            hex_color = self._color_hex((float(red), float(green), float(blue)))
        except Exception:
            hex_color = "#000000"
        return f"{hex_color} {red}, {green}, {blue}"

    def _material_values(self, row: dict[str, str]) -> tuple[str, str, str, str, str, str, str, str, str, str]:
        mode = "extended" if row["extended"] == "1" else "standard"
        emissive = self._material_color_cell(row, "emissive") if row["extended"] == "1" else "-"
        emissive_strength = row["emissive_strength"] if row["extended"] == "1" else "-"
        emissive_tex = self._texture_name(row["emissive_texture"]) if row["extended"] == "1" else "-"
        return (
            row["name"] or "[default]",
            mode,
            row.get("triangles", "0"),
            self._material_color_cell(row, "color"),
            f"{row['ambiant_strength']}/{row['diffuse_strength']}/{row['specular_strength']}",
            row["specular_exponent"],
            self._texture_name(row["diffuse_texture"]),
            emissive,
            emissive_strength,
            emissive_tex,
        )

    def _texture_values(self, row: dict[str, str]) -> tuple[str, str, str, str, str, str]:
        return (
            row["name"],
            row["size"],
            row["resize"],
            row["filter"] if row["path"] else "",
            self._texture_users_text(row["id"]),
            row["path"],
        )

    def _insert_material_row(self, key: str) -> None:
        row = self.material_rows[key]
        iid = f"material_{len(self.material_iid_to_key)}"
        self.material_iid_to_key[iid] = key
        self.material_key_to_iid[key] = iid
        self.material_table.insert("", "end", iid=iid, values=self._material_values(row))

    def _insert_texture_row(self, texture_id: str) -> None:
        row = self.texture_assets[texture_id]
        iid = f"texture_{len(self.texture_iid_to_id)}"
        self.texture_iid_to_id[iid] = texture_id
        self.texture_id_to_iid[texture_id] = iid
        self.texture_table.insert("", "end", iid=iid, image=self._texture_size_icon(row), values=self._texture_values(row))

    def _refresh_material_row(self, key: str) -> None:
        iid = self.material_key_to_iid.get(key)
        if iid:
            row = self.material_rows[key]
            self.material_table.item(iid, values=self._material_values(row))

    def _refresh_texture_row(self, texture_id: str) -> None:
        iid = self.texture_id_to_iid.get(texture_id)
        if iid:
            row = self.texture_assets[texture_id]
            self.texture_table.item(iid, image=self._texture_size_icon(row), values=self._texture_values(row))

    def _refresh_all_materials(self) -> None:
        for key in list(self.material_rows):
            self._refresh_material_row(key)

    def _refresh_all_textures(self) -> None:
        for texture_id in list(self.texture_assets):
            self._refresh_texture_row(texture_id)

    def choose_input(self):
        path = filedialog.askopenfilename(
            title="Choose mesh",
            filetypes=[("Mesh files", "*.obj *.h *.hpp"), ("All files", "*.*")],
        )
        if not path:
            return
        self.input_path.set(path)
        stem = sanitize_identifier(Path(path).stem)
        self.object_name.set(stem)
        self.output_path.set(str(Path(path).with_name(stem + "_v2.h")))
        self.reload_textures()

    def choose_output(self):
        path = filedialog.asksaveasfilename(title="Output mesh header", defaultextension=".h", filetypes=[("Header", "*.h"), ("All files", "*.*")])
        if path:
            self.output_path.set(path)

    def _set_mesh_info(self, values: dict[str, object] | None = None):
        values = values or {}
        for key, variable in self.mesh_info_vars.items():
            variable.set(str(values.get(key, "-")))
        self._set_mesh_size_indicator(values.get("vertices_count"))
        self._set_mesh_info_value_colors(values)

    def _mesh_count_color(self, value: object | None) -> str:
        colors = {
            "none": "#b0b0b0",
            "small": "#23a441",
            "medium": "#2f75b5",
            "large": "#e2b400",
            "xlarge": "#e57b22",
            "huge": "#d33333",
        }
        try:
            count = int(value) if value is not None else -1
        except Exception:
            count = -1
        if count < 0:
            key = "none"
        elif count < 2000:
            key = "small"
        elif count < 5000:
            key = "medium"
        elif count < 10000:
            key = "large"
        elif count < 25000:
            key = "xlarge"
        else:
            key = "huge"
        return colors[key]

    def _set_mesh_size_indicator(self, vertices_count: object | None):
        if self.mesh_size_indicator is None:
            return
        color = self._mesh_count_color(vertices_count)
        canvas = self.mesh_size_indicator
        canvas.delete("all")
        canvas.create_rectangle(2, 2, 14, 14, fill=color, outline="#666666")

    def _set_mesh_info_value_colors(self, values: dict[str, object]):
        count_keys = ("vertices", "triangles", "uv", "normals")
        for key, label in self.mesh_info_value_labels.items():
            if key in count_keys:
                label.configure(foreground=self._mesh_count_color(values.get(f"{key}_count", values.get(key))))
            else:
                label.configure(foreground="")

    def _update_mesh_info(self, path: Path):
        if not path.exists():
            self._set_mesh_info()
            return
        try:
            if path.suffix.lower() == ".obj":
                mesh = load_obj(path)
                used_materials = {tri.material for tri in mesh.triangles} or {""}
                textured = sum(1 for material in used_materials if mesh.materials.get(material) is not None and mesh.materials[material].texture_path)
                emissive_textured = sum(1 for material in used_materials if mesh.materials.get(material) is not None and mesh.materials[material].emissive_texture_path)
                self._set_mesh_info(
                    {
                        "type": "OBJ",
                        "vertices": len(mesh.vertices),
                        "vertices_count": len(mesh.vertices),
                        "triangles": len(mesh.triangles),
                        "triangles_count": len(mesh.triangles),
                        "uv": len(mesh.texcoords),
                        "uv_count": len(mesh.texcoords),
                        "normals": len(mesh.normals),
                        "normals_count": len(mesh.normals),
                        "materials": f"{len(used_materials)} ({textured} tex, {emissive_textured} emit)",
                    }
                )
                return
            mesh_type = detect_mesh_type(path)
            if mesh_type == "Mesh3Dv2":
                mesh = parse_mesh3d2_header(path)
                triangles = sum(len(m.triangles) for m in mesh.meshlets)
                vertices = sum(m.nb_vertices for m in mesh.meshlets)
                texcoords = sum(m.nb_texcoords for m in mesh.meshlets)
                normals = sum(m.nb_normals for m in mesh.meshlets)
                textured = sum(1 for mat in mesh.materials if mat.texture_symbol)
                emissive_textured = sum(1 for mat in mesh.materials if mat.emissive_texture_symbol)
                self._set_mesh_info(
                    {
                        "type": "Mesh3Dv2",
                        "vertices": f"{vertices} local",
                        "vertices_count": vertices,
                        "triangles": triangles,
                        "triangles_count": triangles,
                        "uv": f"{texcoords} local",
                        "uv_count": texcoords,
                        "normals": f"{normals} local",
                        "normals_count": normals,
                        "materials": f"{len(mesh.materials)} ({textured} tex, {emissive_textured} emit)",
                    }
                )
                return
            parsed = parse_legacy_header(path)
            triangles = sum(mesh.nb_faces for mesh in parsed.meshes.values())
            vertex_arrays = {mesh.vertices for mesh in parsed.meshes.values() if mesh.vertices}
            texcoord_arrays = {mesh.texcoords for mesh in parsed.meshes.values() if mesh.texcoords}
            normal_arrays = {mesh.normals for mesh in parsed.meshes.values() if mesh.normals}
            vertices = sum(int(parsed.vertices[name].shape[0]) for name in vertex_arrays if name in parsed.vertices)
            texcoords = sum(int(parsed.texcoords[name].shape[0]) for name in texcoord_arrays if name in parsed.texcoords)
            normals = sum(int(parsed.normals[name].shape[0]) for name in normal_arrays if name in parsed.normals)
            textured = sum(1 for mesh in parsed.meshes.values() if mesh.texture)
            self._set_mesh_info(
                {
                    "type": "Mesh3D",
                    "vertices": vertices,
                    "vertices_count": vertices,
                    "triangles": triangles,
                    "triangles_count": triangles,
                    "uv": texcoords,
                    "uv_count": texcoords,
                    "normals": normals,
                    "normals_count": normals,
                    "materials": f"{len(parsed.meshes)} ({textured} tex)",
                }
            )
        except Exception as exc:
            self._set_mesh_info({"type": "unreadable", "triangles": exc})

    def reload_textures(self):
        self.material_rows.clear()
        self.material_iid_to_key.clear()
        self.material_key_to_iid.clear()
        self.texture_assets.clear()
        self.texture_iid_to_id.clear()
        self.texture_id_to_iid.clear()
        self.texture_key_to_id.clear()
        self.next_texture_id = 1
        self.material_table.delete(*self.material_table.get_children())
        self.texture_table.delete(*self.texture_table.get_children())
        path = Path(self.input_path.get())
        self._update_mesh_info(path)
        if not path.exists():
            return
        if path.suffix.lower() != ".obj":
            self._reload_tgx_textures(path)
            return
        try:
            mesh = load_obj(path)
            choices = resolve_mesh_textures(mesh)
            emissive_choices = resolve_mesh_textures(mesh, role="map_ke", guess_when_missing=False)
        except Exception as exc:
            messagebox.showerror("Texture scan failed", str(exc))
            return
        material_counts = self._obj_material_triangle_counts(mesh)
        materials = sorted(material_counts) or [""]
        for material in materials:
            desc = mesh.materials.get(material)
            diffuse_texture = ""
            emissive_texture = ""
            emissive_selected = ""
            emissive_requested = ""
            emissive_refs = ""
            choice = choices.get(material)
            if choice is not None:
                selected = str(choice.selected) if choice.selected else ""
                requested = str(choice.requested) if choice.requested else ""
                refs = self._display_texture_refs(choice.refs, DIFFUSE_ROLES)
                if self._texture_choice_has_data(selected, requested, refs):
                    diffuse_texture = self._add_texture_asset(selected or requested, name=Path(selected or requested).stem)
            emissive_choice = emissive_choices.get(material)
            if emissive_choice is not None:
                emissive_selected = str(emissive_choice.selected) if emissive_choice.selected else ""
                emissive_requested = str(emissive_choice.requested) if emissive_choice.requested else ""
                emissive_refs = self._display_texture_refs(emissive_choice.refs, EMISSIVE_ROLES)
                if self._texture_choice_has_data(emissive_selected, emissive_requested, emissive_refs):
                    emissive_texture = self._add_texture_asset(emissive_selected or emissive_requested, name=Path(emissive_selected or emissive_requested).stem)
            extended = self._material_has_emissive_data(desc, selected=emissive_selected, requested=emissive_requested, refs=emissive_refs)
            self._add_material_row(
                material,
                desc,
                diffuse_texture=diffuse_texture,
                emissive_texture=emissive_texture,
                extended=extended,
                triangles=material_counts.get(material, 0),
            )
        nearby_images = 0
        if not self.texture_assets and mesh.has_texcoords():
            nearby_images = self._add_nearby_image_assets(path.parent)
        self._refresh_all_textures()
        self._append_log(f"Found {len(self.material_rows)} material(s), {len(self.texture_assets)} texture(s).")
        if nearby_images:
            self._append_log(f"No texture map was declared by the used materials; added {nearby_images} unlinked image(s) from the OBJ folder.")

    def _diffuse_color(self, desc: object | None) -> tuple[float, float, float]:
        if desc is not None and hasattr(desc, "color"):
            return self._material_tuple(desc, "color", DEFAULT_MATERIAL_COLOR)
        return self._material_tuple(desc, "diffuse", DEFAULT_MATERIAL_COLOR)

    def _emissive_color(self, desc: object | None) -> tuple[float, float, float]:
        if desc is not None and hasattr(desc, "emissive_color"):
            return self._material_tuple(desc, "emissive_color", DEFAULT_EMISSIVE_COLOR)
        return self._material_tuple(desc, "emissive", DEFAULT_EMISSIVE_COLOR)

    def _ambient_strength(self, desc: object | None) -> float:
        if desc is not None and hasattr(desc, "ambiant"):
            return self._material_float(desc, "ambiant", DEFAULT_AMBIANT_STRENGTH)
        return self._material_float(desc, "ambiant_strength", DEFAULT_AMBIANT_STRENGTH)

    def _diffuse_strength_value(self, desc: object | None) -> float:
        if desc is not None and hasattr(desc, "diffuse_strength"):
            return self._material_float(desc, "diffuse_strength", DEFAULT_DIFFUSE_STRENGTH)
        if desc is not None and hasattr(desc, "color"):
            return self._material_float(desc, "diffuse", DEFAULT_DIFFUSE_STRENGTH)
        return DEFAULT_DIFFUSE_STRENGTH

    def _specular_strength_value(self, desc: object | None) -> float:
        if desc is not None and hasattr(desc, "specular"):
            return self._material_float(desc, "specular", DEFAULT_SPECULAR_STRENGTH)
        return self._material_float(desc, "specular_strength", DEFAULT_SPECULAR_STRENGTH)

    def _specular_exponent_value(self, desc: object | None) -> int:
        if desc is not None and hasattr(desc, "exponent"):
            return self._material_int(desc, "exponent", DEFAULT_SPECULAR_EXPONENT)
        return self._material_int(desc, "specular_exponent", DEFAULT_SPECULAR_EXPONENT)

    def _add_material_row(
        self,
        source: str,
        desc: object | None,
        *,
        diffuse_texture: str = "",
        emissive_texture: str = "",
        extended: bool = False,
        triangles: int = 0,
    ) -> None:
        key = source
        color = self._diffuse_color(desc)
        emissive = self._emissive_color(desc)
        name = self._unique_material_name(source or "default", exclude_key=key)
        self.material_rows[key] = {
            "source": source,
            "name": name,
            "extended": "1" if extended else "0",
            "triangles": str(max(0, int(triangles))),
            "color": self._format_material_color(color),
            "color_r": self._format_float(color[0]),
            "color_g": self._format_float(color[1]),
            "color_b": self._format_float(color[2]),
            "ambiant_strength": self._format_float(self._ambient_strength(desc)),
            "diffuse_strength": self._format_float(self._diffuse_strength_value(desc)),
            "specular_strength": self._format_float(self._specular_strength_value(desc)),
            "specular_exponent": str(self._specular_exponent_value(desc)),
            "diffuse_texture": diffuse_texture,
            "emissive_color": self._format_material_color(emissive),
            "emissive_r": self._format_float(emissive[0]),
            "emissive_g": self._format_float(emissive[1]),
            "emissive_b": self._format_float(emissive[2]),
            "emissive_strength": self._format_float(self._material_float(desc, "emissive_strength", DEFAULT_EMISSIVE_STRENGTH)),
            "emissive_texture": emissive_texture,
        }
        self._insert_material_row(key)

    def _reload_tgx_textures(self, path: Path) -> None:
        try:
            mesh_type = detect_mesh_type(path)
            if mesh_type == "Mesh3Dv2":
                mesh = parse_mesh3d2_header(path)
                if not mesh.materials:
                    self._add_material_row("", None)
                material_counts = self._decoded_mesh3d2_material_triangle_counts(mesh)
                for index, material in enumerate(mesh.materials):
                    name = material.name or f"mat{index}"
                    symbol = material.texture_symbol
                    selected = str(mesh.texture_headers.get(symbol, "")) if symbol else ""
                    diffuse_texture = ""
                    if self._texture_choice_has_data(selected, symbol, symbol):
                        diffuse_texture = self._add_texture_asset(selected, name=Path(selected).stem if selected else symbol, symbol=symbol)
                    emissive_symbol = material.emissive_texture_symbol
                    emissive_selected = str(mesh.texture_headers.get(emissive_symbol, "")) if emissive_symbol else ""
                    has_emissive = self._material_has_emissive_data(
                        material,
                        selected=emissive_selected,
                        requested=emissive_symbol,
                        refs=emissive_symbol,
                        symbol=emissive_symbol,
                    )
                    emissive_texture = ""
                    if self._texture_choice_has_data(emissive_selected, emissive_symbol, emissive_symbol):
                        emissive_texture = self._add_texture_asset(emissive_selected, name=Path(emissive_selected).stem if emissive_selected else emissive_symbol, symbol=emissive_symbol)
                    self._add_material_row(
                        name,
                        material,
                        diffuse_texture=diffuse_texture,
                        emissive_texture=emissive_texture,
                        extended=has_emissive,
                        triangles=material_counts.get(name, 0),
                    )
            else:
                parsed = parse_legacy_header(path)
                _obj, _texture_symbols, chain = legacy_to_objmesh(parsed)
                raw = path.read_text(encoding="utf-8", errors="replace")
                texture_headers = _parse_texture_headers(raw, path.resolve().parent)
                for index, mesh_decl in enumerate(chain):
                    material = mesh_decl.name or f"mesh_{index}"
                    desc = Material(
                        name=material,
                        diffuse=mesh_decl.color,
                        ambiant_strength=mesh_decl.ambiant_strength,
                        diffuse_strength=mesh_decl.diffuse_strength,
                        specular_strength=mesh_decl.specular_strength,
                        specular_exponent=mesh_decl.specular_exponent,
                    )
                    symbol = mesh_decl.texture
                    selected = str(texture_headers.get(symbol, "")) if symbol else ""
                    diffuse_texture = ""
                    if self._texture_choice_has_data(selected, symbol, symbol):
                        diffuse_texture = self._add_texture_asset(selected, name=Path(selected).stem if selected else symbol, symbol=symbol)
                    self._add_material_row(
                        material,
                        desc,
                        diffuse_texture=diffuse_texture,
                        extended=False,
                        triangles=mesh_decl.nb_faces,
                    )
        except Exception as exc:
            messagebox.showerror("Texture scan failed", str(exc))
            return
        self._refresh_all_textures()
        if self.material_rows:
            self._append_log(f"Found {len(self.material_rows)} TGX material(s), {len(self.texture_assets)} texture(s).")
        else:
            self._append_log("No TGX material found in this mesh.")

    def _selected_material_row(self) -> tuple[str, dict[str, str], str] | None:
        selection = self.material_table.selection()
        if not selection:
            return None
        iid = selection[0]
        key = self.material_iid_to_key.get(iid)
        if key is None:
            return None
        row = self.material_rows.get(key)
        if row is None:
            return None
        return key, row, iid

    def _selected_texture_row(self) -> tuple[str, dict[str, str], str] | None:
        selection = self.texture_table.selection()
        if not selection:
            return None
        iid = selection[0]
        texture_id = self.texture_iid_to_id.get(iid)
        if texture_id is None:
            return None
        row = self.texture_assets.get(texture_id)
        if row is None:
            return None
        return texture_id, row, iid

    def edit_selected_material(self, _event=None):
        selected = self._selected_material_row()
        if selected is not None:
            self._open_material_editor(*selected)

    def edit_selected_texture(self, _event=None):
        selected = self._selected_texture_row()
        if selected is not None:
            self._open_texture_editor(*selected)

    def _parse_unit_float(self, text: str, label: str) -> float:
        try:
            value = float(text)
        except Exception:
            raise ValueError(f"{label} must be a number.")
        if value < 0.0 or value > 1.0:
            raise ValueError(f"{label} must be between 0 and 1.")
        return value

    def _parse_nonnegative_float(self, text: str, label: str) -> float:
        try:
            value = float(text)
        except Exception:
            raise ValueError(f"{label} must be a number.")
        if value < 0.0:
            raise ValueError(f"{label} must be non-negative.")
        return value

    def _parse_exponent(self, text: str) -> int:
        try:
            value = int(round(float(text)))
        except Exception:
            raise ValueError("Specular exponent must be a number.")
        if value < 1 or value > 128:
            raise ValueError("Specular exponent must be between 1 and 128.")
        return value

    def _color_hex(self, rgb: tuple[float, float, float]) -> str:
        parts = [max(0, min(255, int(round(value * 255.0)))) for value in rgb]
        return "#{:02x}{:02x}{:02x}".format(*parts)

    def _texture_choices(self) -> tuple[list[str], dict[str, str]]:
        names = ["none"]
        lookup = {"none": ""}
        for texture_id, texture in sorted(self.texture_assets.items(), key=lambda item: item[1]["name"].lower()):
            names.append(texture["name"])
            lookup[texture["name"]] = texture_id
        return names, lookup

    def _open_material_editor(self, key: str, row: dict[str, str], _iid: str) -> None:
        dialog = tk.Toplevel(self)
        dialog.title(f"Edit material - {row['name'] or '[default]'}")
        dialog.transient(self)
        dialog.grab_set()
        dialog.columnconfigure(1, weight=1)

        name_var = tk.StringVar(value=row["name"])
        color_vars = [tk.StringVar(value=row["color_r"]), tk.StringVar(value=row["color_g"]), tk.StringVar(value=row["color_b"])]
        emissive_vars = [tk.StringVar(value=row["emissive_r"]), tk.StringVar(value=row["emissive_g"]), tk.StringVar(value=row["emissive_b"])]
        ambiant_var = tk.StringVar(value=row["ambiant_strength"])
        diffuse_var = tk.StringVar(value=row["diffuse_strength"])
        specular_var = tk.StringVar(value=row["specular_strength"])
        exponent_var = tk.StringVar(value=row["specular_exponent"])
        emissive_strength_var = tk.StringVar(value=row["emissive_strength"])
        extended_var = tk.BooleanVar(value=row["extended"] == "1")
        texture_names, texture_lookup = self._texture_choices()
        diffuse_texture_var = tk.StringVar(value=self._texture_name(row["diffuse_texture"]))
        emissive_texture_var = tk.StringVar(value=self._texture_name(row["emissive_texture"]))

        ttk.Label(dialog, text="Name").grid(row=0, column=0, sticky="w", padx=8, pady=(10, 3))
        ttk.Entry(dialog, textvariable=name_var).grid(row=0, column=1, columnspan=3, sticky="ew", padx=8, pady=(10, 3))

        diffuse_swatch = tk.Canvas(dialog, width=28, height=20, highlightthickness=1, highlightbackground="#666666")
        diffuse_swatch.grid(row=1, column=0, sticky="w", padx=8, pady=(8, 3))

        def update_swatch(canvas: tk.Canvas, vars_: list[tk.StringVar]) -> None:
            try:
                rgb = tuple(self._parse_unit_float(var.get(), "color") for var in vars_)
            except Exception:
                rgb = (0.0, 0.0, 0.0)
            canvas.delete("all")
            canvas.create_rectangle(0, 0, 28, 20, fill=self._color_hex(rgb), outline="")

        def pick_color(vars_: list[tk.StringVar], canvas: tk.Canvas) -> None:
            try:
                initial = self._color_hex(tuple(self._parse_unit_float(var.get(), "color") for var in vars_))
            except Exception:
                initial = "#000000"
            _rgb, hex_value = colorchooser.askcolor(color=initial, parent=dialog, title="Choose color")
            if not hex_value:
                return
            hex_value = hex_value.lstrip("#")
            values = [int(hex_value[i : i + 2], 16) / 255.0 for i in (0, 2, 4)]
            for var, value in zip(vars_, values):
                var.set(self._format_float(value))
            update_swatch(canvas, vars_)

        ttk.Label(dialog, text="Diffuse color").grid(row=1, column=1, sticky="w", pady=(8, 3))
        ttk.Button(dialog, text="Pick...", command=lambda: pick_color(color_vars, diffuse_swatch)).grid(row=1, column=3, sticky="e", padx=8, pady=(8, 3))
        for index, label in enumerate(("R", "G", "B"), start=2):
            ttk.Label(dialog, text=label).grid(row=index, column=0, sticky="w", padx=8, pady=3)
            ttk.Entry(dialog, textvariable=color_vars[index - 2], width=10).grid(row=index, column=1, sticky="ew", padx=8, pady=3)
            color_vars[index - 2].trace_add("write", lambda *_args: update_swatch(diffuse_swatch, color_vars))

        row_index = 5
        for label, var in (
            ("Ambient strength", ambiant_var),
            ("Diffuse strength", diffuse_var),
            ("Specular strength", specular_var),
            ("Specular exponent", exponent_var),
        ):
            ttk.Label(dialog, text=label).grid(row=row_index, column=0, sticky="w", padx=8, pady=3)
            ttk.Entry(dialog, textvariable=var, width=12).grid(row=row_index, column=1, columnspan=3, sticky="ew", padx=8, pady=3)
            row_index += 1

        ttk.Label(dialog, text="Diffuse texture").grid(row=row_index, column=0, sticky="w", padx=8, pady=(8, 3))
        ttk.Combobox(dialog, textvariable=diffuse_texture_var, values=texture_names, state="readonly").grid(row=row_index, column=1, columnspan=3, sticky="ew", padx=8, pady=(8, 3))
        row_index += 1

        extended_check = ttk.Checkbutton(dialog, text="Extended material", variable=extended_var)
        extended_check.grid(row=row_index, column=0, columnspan=4, sticky="w", padx=8, pady=(10, 3))
        row_index += 1

        emissive_widgets: list[tk.Widget] = []
        emissive_swatch = tk.Canvas(dialog, width=28, height=20, highlightthickness=1, highlightbackground="#666666")
        emissive_swatch.grid(row=row_index, column=0, sticky="w", padx=8, pady=(8, 3))
        emissive_widgets.append(emissive_swatch)
        label = ttk.Label(dialog, text="Emissive color")
        label.grid(row=row_index, column=1, sticky="w", pady=(8, 3))
        emissive_widgets.append(label)
        pick = ttk.Button(dialog, text="Pick...", command=lambda: pick_color(emissive_vars, emissive_swatch))
        pick.grid(row=row_index, column=3, sticky="e", padx=8, pady=(8, 3))
        emissive_widgets.append(pick)
        row_index += 1
        for index, label_text in enumerate(("R", "G", "B"), start=row_index):
            label = ttk.Label(dialog, text=label_text)
            label.grid(row=index, column=0, sticky="w", padx=8, pady=3)
            entry = ttk.Entry(dialog, textvariable=emissive_vars[index - row_index], width=10)
            entry.grid(row=index, column=1, sticky="ew", padx=8, pady=3)
            emissive_widgets.extend([label, entry])
            emissive_vars[index - row_index].trace_add("write", lambda *_args: update_swatch(emissive_swatch, emissive_vars))
        row_index += 3
        label = ttk.Label(dialog, text="Emissive strength")
        label.grid(row=row_index, column=0, sticky="w", padx=8, pady=3)
        entry = ttk.Entry(dialog, textvariable=emissive_strength_var, width=12)
        entry.grid(row=row_index, column=1, columnspan=3, sticky="ew", padx=8, pady=3)
        emissive_widgets.extend([label, entry])
        row_index += 1
        label = ttk.Label(dialog, text="Emissive texture")
        label.grid(row=row_index, column=0, sticky="w", padx=8, pady=(8, 3))
        combo = ttk.Combobox(dialog, textvariable=emissive_texture_var, values=texture_names, state="readonly")
        combo.grid(row=row_index, column=1, columnspan=3, sticky="ew", padx=8, pady=(8, 3))
        emissive_widgets.extend([label, combo])
        row_index += 1

        def update_extended_state(*_args) -> None:
            state = "normal" if extended_var.get() else "disabled"
            for widget in emissive_widgets:
                try:
                    widget.configure(state=state)
                except tk.TclError:
                    pass

        extended_var.trace_add("write", update_extended_state)

        buttons = ttk.Frame(dialog)
        buttons.grid(row=row_index, column=0, columnspan=4, sticky="e", padx=8, pady=(10, 8))

        def apply() -> None:
            try:
                material_name = self._unique_material_name(name_var.get().strip() or "material", exclude_key=key)
                color = tuple(self._parse_unit_float(var.get(), f"color {name}") for var, name in zip(color_vars, ("R", "G", "B")))
                ambiant = self._parse_nonnegative_float(ambiant_var.get(), "Ambient strength")
                diffuse = self._parse_nonnegative_float(diffuse_var.get(), "Diffuse strength")
                specular = self._parse_nonnegative_float(specular_var.get(), "Specular strength")
                exponent = self._parse_exponent(exponent_var.get())
                emissive = tuple(self._parse_unit_float(var.get(), f"emissive {name}") for var, name in zip(emissive_vars, ("R", "G", "B")))
                emissive_strength = self._parse_nonnegative_float(emissive_strength_var.get(), "Emissive strength")
            except Exception as exc:
                messagebox.showerror("Invalid material values", str(exc), parent=dialog)
                return
            row["name"] = material_name
            row["color_r"], row["color_g"], row["color_b"] = (self._format_float(value) for value in color)
            row["color"] = self._format_material_color(color)
            row["ambiant_strength"] = self._format_float(ambiant)
            row["diffuse_strength"] = self._format_float(diffuse)
            row["specular_strength"] = self._format_float(specular)
            row["specular_exponent"] = str(exponent)
            row["diffuse_texture"] = texture_lookup.get(diffuse_texture_var.get(), "")
            row["extended"] = "1" if extended_var.get() else "0"
            row["emissive_r"], row["emissive_g"], row["emissive_b"] = (self._format_float(value) for value in emissive)
            row["emissive_color"] = self._format_material_color(emissive)
            row["emissive_strength"] = self._format_float(emissive_strength)
            row["emissive_texture"] = texture_lookup.get(emissive_texture_var.get(), "") if extended_var.get() else ""
            self._refresh_material_row(key)
            self._refresh_all_textures()
            dialog.destroy()

        ttk.Button(buttons, text="Cancel", command=dialog.destroy).pack(side="right")
        ttk.Button(buttons, text="Apply", command=apply).pack(side="right", padx=(0, 8))
        update_swatch(diffuse_swatch, color_vars)
        update_swatch(emissive_swatch, emissive_vars)
        update_extended_state()

    def _open_texture_editor(self, texture_id: str, row: dict[str, str], _iid: str) -> None:
        dialog = tk.Toplevel(self)
        dialog.title(f"Edit texture - {row['name']}")
        dialog.transient(self)
        dialog.grab_set()
        dialog.columnconfigure(1, weight=1)
        dialog.rowconfigure(4, weight=1)
        dialog.minsize(760, 520)

        name_var = tk.StringVar(value=row["name"])
        path_var = tk.StringVar(value=row["path"])
        width_var = tk.StringVar(value=row["resize_w"])
        height_var = tk.StringVar(value=row["resize_h"])
        filter_var = tk.StringVar(value=row["filter"])
        preview_images: dict[str, ImageTk.PhotoImage | None] = {"original": None, "resized": None}

        ttk.Label(dialog, text="Name").grid(row=0, column=0, sticky="w", padx=8, pady=(10, 3))
        ttk.Entry(dialog, textvariable=name_var).grid(row=0, column=1, columnspan=2, sticky="ew", padx=8, pady=(10, 3))
        ttk.Label(dialog, text="Texture").grid(row=1, column=0, sticky="w", padx=8, pady=3)
        ttk.Entry(dialog, textvariable=path_var).grid(row=1, column=1, sticky="ew", padx=8, pady=3)

        def browse() -> None:
            path = filedialog.askopenfilename(
                title="Choose texture",
                filetypes=[("Images and TGX headers", "*.png *.jpg *.jpeg *.bmp *.tga *.tif *.tiff *.gif *.h *.hpp"), ("All files", "*.*")],
                parent=dialog,
            )
            if path:
                path_var.set(path)
                update_preview()

        ttk.Button(dialog, text="Browse...", command=browse).grid(row=1, column=2, sticky="e", padx=8, pady=3)

        ttk.Label(dialog, text="Resize").grid(row=2, column=0, sticky="w", padx=8, pady=3)
        resize_box = ttk.Frame(dialog)
        resize_box.grid(row=2, column=1, columnspan=2, sticky="w", padx=8, pady=3)
        ttk.Combobox(resize_box, textvariable=width_var, values=RESIZE_VALUES, state="readonly", width=6).pack(side="left")
        ttk.Label(resize_box, text="x").pack(side="left", padx=4)
        ttk.Combobox(resize_box, textvariable=height_var, values=RESIZE_VALUES, state="readonly", width=6).pack(side="left")

        ttk.Label(dialog, text="Filter").grid(row=3, column=0, sticky="w", padx=8, pady=3)
        ttk.Combobox(dialog, textvariable=filter_var, values=SUPPORTED_RESAMPLING, state="readonly", width=12).grid(row=3, column=1, columnspan=2, sticky="w", padx=8, pady=3)

        preview_box = ttk.Frame(dialog)
        preview_box.grid(row=4, column=0, columnspan=3, sticky="nsew", padx=8, pady=(8, 3))
        preview_box.columnconfigure(0, weight=1)
        preview_box.columnconfigure(1, weight=1)
        original_preview = ttk.Label(preview_box, text="Original\nNo texture selected", anchor="center", relief="sunken", compound="top")
        original_preview.grid(row=0, column=0, sticky="nsew", padx=(0, 4))
        resized_preview = ttk.Label(preview_box, text="Export size\nNo texture selected", anchor="center", relief="sunken", compound="top")
        resized_preview.grid(row=0, column=1, sticky="nsew", padx=(4, 0))

        def preview_resample(name: str):
            resampling = getattr(Image, "Resampling", Image)
            return {
                "nearest": resampling.NEAREST,
                "bilinear": resampling.BILINEAR,
                "bicubic": resampling.BICUBIC,
                "lanczos": resampling.LANCZOS,
            }.get(name.strip().lower(), resampling.LANCZOS)

        def load_preview_image(path: Path) -> tuple[Image.Image, tuple[int, int]]:
            if path.suffix.lower() in TGX_SOURCE_EXTENSIONS:
                symbol = row.get("symbol", "") or _first_tgx_image_symbol(path)
                texture = _parse_texture_image(path, symbol) if symbol else None
                if texture is None:
                    fallback_symbol = _first_tgx_image_symbol(path)
                    texture = _parse_texture_image(path, fallback_symbol) if fallback_symbol else None
                if texture is None:
                    raise ValueError("could not parse TGX texture header")
                image = Image.fromarray(texture.rgb, "RGB").convert("RGBA")
                return image, (texture.width, texture.height)
            with Image.open(path) as img:
                image = img.convert("RGBA")
                return image, img.size

        def make_preview_photo(image: Image.Image, *, upsample_small: bool = False) -> ImageTk.PhotoImage:
            max_size = (340, 240)
            preview = image.copy()
            if upsample_small and preview.width > 0 and preview.height > 0:
                scale = min(max_size[0] // preview.width, max_size[1] // preview.height, 16)
                if scale > 1:
                    preview = preview.resize((preview.width * scale, preview.height * scale), getattr(Image, "Resampling", Image).NEAREST)
            preview.thumbnail(max_size, getattr(Image, "Resampling", Image).LANCZOS)
            return ImageTk.PhotoImage(preview)

        def update_preview() -> None:
            preview_images["original"] = None
            preview_images["resized"] = None
            path_text = path_var.get().strip()
            if not path_text:
                original_preview.configure(image="", text="Original\nNo texture selected")
                resized_preview.configure(image="", text="Export size\nNo texture selected")
                return
            path = Path(path_text)
            if not path.exists():
                original_preview.configure(image="", text=f"Original\n{path.name}\nmissing file")
                resized_preview.configure(image="", text=f"Export size\n{path.name}\nmissing file")
                return
            try:
                image, (width, height) = load_preview_image(path)
                preview_images["original"] = make_preview_photo(image)
                width_text = width_var.get().strip()
                height_text = height_var.get().strip()
                if bool(width_text) != bool(height_text):
                    resized_preview.configure(image="", text="Export size\nchoose width and height")
                    original_preview.configure(image=preview_images["original"], text=f"Original\n{path.name}\n{width}x{height}")
                    return
                if width_text and height_text:
                    target_width, target_height = parse_resize(f"{width_text}x{height_text}")
                    resized_image = image.resize((target_width, target_height), preview_resample(filter_var.get()))
                    resized_text = f"Resized export\n{target_width}x{target_height}\n{filter_var.get().strip() or 'lanczos'}"
                else:
                    target_width, target_height = width, height
                    resized_image = image
                    resized_text = f"Export size\n{target_width}x{target_height}\noriginal"
                preview_images["resized"] = make_preview_photo(resized_image, upsample_small=True)
            except Exception as exc:
                original_preview.configure(image="", text=f"Original\n{path.name}\npreview failed")
                resized_preview.configure(image="", text=f"Export size\n{exc}")
                return
            original_preview.configure(image=preview_images["original"], text=f"Original\n{path.name}\n{width}x{height}")
            resized_preview.configure(image=preview_images["resized"], text=resized_text)

        def clear() -> None:
            path_var.set("")
            width_var.set("")
            height_var.set("")
            update_preview()

        def apply() -> None:
            width = width_var.get().strip()
            height = height_var.get().strip()
            if bool(width) != bool(height):
                messagebox.showerror("Invalid texture options", "Choose both resize width and height, or leave both empty.", parent=dialog)
                return
            resize = f"{width}x{height}" if width and height else ""
            try:
                texture_name = self._unique_texture_name(name_var.get().strip() or Path(path_var.get()).stem or "texture", exclude_id=texture_id)
                if resize:
                    parse_resize(resize)
                resample = filter_var.get().strip().lower() or "lanczos"
                if resample not in SUPPORTED_RESAMPLING:
                    raise ValueError("Unsupported texture filter '{}'. Choose one of: {}".format(resample, ", ".join(SUPPORTED_RESAMPLING)))
            except Exception as exc:
                messagebox.showerror("Invalid texture options", str(exc), parent=dialog)
                return
            new_path = path_var.get().strip()
            new_symbol = row.get("symbol", "")
            if new_path:
                new_path_obj = Path(new_path)
                if new_path_obj.suffix.lower() in TGX_SOURCE_EXTENSIONS:
                    new_symbol = _first_tgx_image_symbol(new_path_obj)
                else:
                    new_symbol = ""
            else:
                new_symbol = ""
            old_key = self._texture_key(row["path"], row.get("symbol", ""))
            new_key = self._texture_key(new_path, new_symbol)
            existing = self.texture_key_to_id.get(new_key)
            if existing and existing != texture_id:
                messagebox.showerror("Duplicate texture", "Another texture already uses this path.", parent=dialog)
                return
            if old_key in self.texture_key_to_id:
                del self.texture_key_to_id[old_key]
            self.texture_key_to_id[new_key] = texture_id
            row["name"] = texture_name
            row["path"] = new_path
            row["symbol"] = new_symbol
            row["resize"] = resize
            row["resize_w"] = width
            row["resize_h"] = height
            row["filter"] = resample
            row["size"] = self._texture_size_text(row["path"], row.get("symbol", ""))
            self._refresh_texture_row(texture_id)
            self._refresh_all_materials()
            dialog.destroy()

        buttons = ttk.Frame(dialog)
        buttons.grid(row=5, column=0, columnspan=3, sticky="e", padx=8, pady=(8, 8))
        ttk.Button(buttons, text="Clear texture", command=clear).pack(side="left", padx=(0, 16))
        ttk.Button(buttons, text="Cancel", command=dialog.destroy).pack(side="right")
        ttk.Button(buttons, text="Apply", command=apply).pack(side="right", padx=(0, 8))
        for variable in (path_var, width_var, height_var, filter_var):
            variable.trace_add("write", lambda *_args: update_preview())
        update_preview()

    def add_texture_asset(self) -> None:
        path = filedialog.askopenfilename(
            title="Add texture",
            filetypes=[("Images and TGX headers", "*.png *.jpg *.jpeg *.bmp *.tga *.tif *.tiff *.gif *.h *.hpp"), ("All files", "*.*")],
        )
        if not path:
            return
        texture_id = self._add_texture_asset(path, name=Path(path).stem)
        iid = self.texture_id_to_iid.get(texture_id)
        if iid:
            self.texture_table.selection_set(iid)
            self.texture_table.see(iid)

    def remove_selected_texture(self) -> None:
        selected = self._selected_texture_row()
        if selected is None:
            return
        texture_id, texture, iid = selected
        users = self._texture_users(texture_id)
        if users and not messagebox.askyesno(
            "Remove texture",
            "This texture is used by {} material link(s):\n\n{}\n\nRemove it anyway?".format(len(users), "\n".join(users)),
        ):
            return
        for material in self.material_rows.values():
            if material.get("diffuse_texture") == texture_id:
                material["diffuse_texture"] = ""
            if material.get("emissive_texture") == texture_id:
                material["emissive_texture"] = ""
        key = self._texture_key(texture["path"], texture.get("symbol", ""))
        self.texture_key_to_id.pop(key, None)
        self.texture_table.delete(iid)
        self.texture_iid_to_id.pop(iid, None)
        self.texture_id_to_iid.pop(texture_id, None)
        self.texture_assets.pop(texture_id, None)
        self._refresh_all_materials()

    def _sync_selected_texture_editor(self):
        return

    def preview_mesh(self):
        path = Path(self.input_path.get())
        if not path.exists():
            messagebox.showerror("Preview failed", "Input mesh does not exist.")
            return
        try:
            self._sync_selected_texture_editor()
        except Exception as exc:
            messagebox.showerror("Invalid texture options", str(exc))
            return
        texture_overrides: dict[str, str] = {}
        emissive_texture_overrides: dict[str, str] = {}
        texture_options: dict[str, tuple[tuple[int, int] | None, str]] = {}
        emissive_texture_options: dict[str, tuple[tuple[int, int] | None, str]] = {}
        texture_symbols: dict[str, str] = {}
        emissive_texture_symbols: dict[str, str] = {}
        for row in self.material_rows.values():
            texture = self.texture_assets.get(row.get("diffuse_texture", ""))
            if texture is not None:
                texture_overrides[row["source"]] = texture["path"]
                texture_options[row["source"]] = (parse_resize(texture["resize"]) if texture["resize"].strip() else None, texture["filter"].strip() or "lanczos")
                texture_symbols[row["source"]] = texture.get("symbol", "")
            if row["extended"] == "1":
                emissive_texture = self.texture_assets.get(row.get("emissive_texture", ""))
                if emissive_texture is not None:
                    emissive_texture_overrides[row["source"]] = emissive_texture["path"]
                    emissive_texture_options[row["source"]] = (
                        parse_resize(emissive_texture["resize"]) if emissive_texture["resize"].strip() else None,
                        emissive_texture["filter"].strip() or "lanczos",
                    )
                    emissive_texture_symbols[row["source"]] = emissive_texture.get("symbol", "")
        material_rows = {key: row.copy() for key, row in self.material_rows.items()}
        self._append_log(f"Opening PyVista preview for {path.name}...")
        threading.Thread(
            target=self._preview_mesh_worker,
            args=(
                path,
                texture_overrides,
                emissive_texture_overrides,
                texture_options,
                emissive_texture_options,
                texture_symbols,
                emissive_texture_symbols,
                material_rows,
            ),
            daemon=True,
        ).start()

    def _preview_mesh_worker(
        self,
        path: Path,
        texture_overrides: dict[str, str],
        emissive_texture_overrides: dict[str, str],
        texture_options: dict[str, tuple[tuple[int, int] | None, str]],
        emissive_texture_options: dict[str, tuple[tuple[int, int] | None, str]],
        texture_symbols: dict[str, str],
        emissive_texture_symbols: dict[str, str],
        material_rows: dict[str, dict[str, str]],
    ):
        try:
            if path.suffix.lower() == ".obj":
                mesh = load_obj(path)
                self._apply_preview_material_rows(mesh, material_rows)
                if not texture_overrides and not material_rows:
                    choices = resolve_mesh_textures(mesh)
                    texture_overrides = {material: str(choice.selected) for material, choice in choices.items() if choice.selected}
                    texture_options = {material: (None, "lanczos") for material in texture_overrides}
                if not emissive_texture_overrides and not material_rows:
                    emissive_choices = resolve_mesh_textures(mesh, role="map_ke", guess_when_missing=False)
                    emissive_texture_overrides = {material: str(choice.selected) for material, choice in emissive_choices.items() if choice.selected}
                    emissive_texture_options = {material: (None, "lanczos") for material in emissive_texture_overrides}
                for material, selected in texture_overrides.items():
                    if material in mesh.materials:
                        mesh.materials[material].texture_path = Path(selected) if selected else None
                        setattr(mesh.materials[material], "texture_symbol_hint", texture_symbols.get(material, ""))
                for material, selected in emissive_texture_overrides.items():
                    if material in mesh.materials:
                        mesh.materials[material].emissive_texture_path = Path(selected) if selected else None
                        setattr(mesh.materials[material], "emissive_texture_symbol_hint", emissive_texture_symbols.get(material, ""))
                from tools._internal.modules.mesh_pipeline.viewer import show_meshlets_pyvista

                show_meshlets_pyvista(
                    mesh,
                    texture=True,
                    show_edges=False,
                    texture_options=texture_options,
                    emissive_texture_options=emissive_texture_options,
                )
            else:
                from tools._internal.modules.mesh_pipeline.mesh_inspect import detect_mesh_type, parse_mesh3d2_header, show_legacy_pyvista, show_mesh3d2_pyvista

                mesh_type = detect_mesh_type(path)
                if mesh_type == "Mesh3D":
                    show_legacy_pyvista(path)
                else:
                    show_mesh3d2_pyvista(parse_mesh3d2_header(path), "tabs")
            self.after(0, self._append_log, "Preview window closed.")
        except Exception as exc:
            self.after(0, self._append_log, f"preview error: {exc}")
            self.after(0, messagebox.showerror, "Preview failed", str(exc))

    def _apply_preview_material_rows(self, mesh, material_rows: dict[str, dict[str, str]]) -> None:
        for row in material_rows.values():
            material = row["source"]
            desc = mesh.materials.get(material)
            if desc is None:
                continue
            desc.diffuse = (float(row["color_r"]), float(row["color_g"]), float(row["color_b"]))
            desc.ambiant_strength = float(row["ambiant_strength"])
            desc.diffuse_strength = float(row["diffuse_strength"])
            desc.specular_strength = float(row["specular_strength"])
            desc.specular_exponent = int(row["specular_exponent"])
            desc.texture_path = None
            desc.emissive_texture_path = None
            setattr(desc, "texture_symbol_hint", "")
            setattr(desc, "emissive_texture_symbol_hint", "")
            if row["extended"] == "1":
                desc.emissive = (float(row["emissive_r"]), float(row["emissive_g"]), float(row["emissive_b"]))
                desc.emissive_strength = float(row["emissive_strength"])
                desc.material_extra_present = True
            else:
                desc.emissive = (0.0, 0.0, 0.0)
                desc.emissive_strength = 0.0
                desc.material_extra_present = False

    def convert(self):
        try:
            self._sync_selected_texture_editor()
            if not self._confirm_pre_export_checks():
                return
            argv = self._build_command()
        except Exception as exc:
            messagebox.showerror("Invalid options", str(exc))
            return
        self._append_log("> " + subprocess.list2cmdline(argv))
        self.conversion_running = True
        self._update_mesh_actions()
        threading.Thread(target=self._run_command, args=(argv,), daemon=True).start()

    def _confirm_pre_export_checks(self) -> bool:
        texture_warnings = self._texture_power_of_two_warnings()
        if texture_warnings:
            message = (
                "The following referenced texture(s) will be exported with non power-of-two "
                "or unknown dimensions:\n\n"
                + "\n".join(texture_warnings)
                + "\n\nContinue export anyway?"
            )
            if not messagebox.askyesno("Texture size warning", message, parent=self):
                return False
        legacy_warnings = self._mesh3d_legacy_omission_warnings()
        if legacy_warnings:
            message = (
                "The Mesh3D legacy format does not store extended material emissive data.\n"
                "The emissive component of the following material(s) will be omitted:\n\n"
                + "\n".join(legacy_warnings)
                + "\n\nContinue export anyway?"
            )
            if not messagebox.askyesno("Mesh3D emissive warning", message, parent=self):
                return False
        large_warnings = self._large_export_warnings()
        if large_warnings:
            message = (
                "The following export parameter(s) are in the red size range:\n\n"
                + "\n".join(large_warnings)
                + "\n\nContinue export anyway?"
            )
            if not messagebox.askyesno("Large mesh warning", message, parent=self):
                return False
        return True

    def _mesh3d_legacy_omission_warnings(self) -> list[str]:
        if self.mesh_format.get() != "Mesh3D":
            return []
        warnings: list[str] = []
        for material in self.material_rows.values():
            if material.get("extended") != "1":
                continue
            texture_name = self._texture_name(material.get("emissive_texture", ""))
            warnings.append(
                "- {}: emissive color {}, strength {}, emissive texture {}".format(
                    material["name"] or "[default]",
                    material.get("emissive_color", "0, 0, 0"),
                    material.get("emissive_strength", "0"),
                    texture_name,
                )
            )
        return warnings

    def _referenced_texture_rows(self) -> list[tuple[str, dict[str, str], list[str]]]:
        referenced: dict[str, list[str]] = {}
        for material in self.material_rows.values():
            label = material["name"] or "[default]"
            diffuse_id = material.get("diffuse_texture", "")
            if diffuse_id:
                referenced.setdefault(diffuse_id, []).append(f"{label}:diffuse")
            if material.get("extended") == "1":
                emissive_id = material.get("emissive_texture", "")
                if emissive_id:
                    referenced.setdefault(emissive_id, []).append(f"{label}:emissive")
        rows: list[tuple[str, dict[str, str], list[str]]] = []
        for texture_id, users in sorted(referenced.items(), key=lambda item: self.texture_assets.get(item[0], {}).get("name", "").lower()):
            texture = self.texture_assets.get(texture_id)
            if texture is not None:
                rows.append((texture_id, texture, users))
        return rows

    def _texture_dimensions(self, texture: dict[str, str]) -> tuple[int, int] | None:
        size_text = self._texture_effective_size(texture)
        try:
            width_text, height_text = size_text.lower().split("x", 1)
            width = int(width_text)
            height = int(height_text)
        except Exception:
            return None
        if width <= 0 or height <= 0:
            return None
        return width, height

    @staticmethod
    def _is_power_of_two(value: int) -> bool:
        return value > 0 and (value & (value - 1)) == 0

    def _texture_power_of_two_warnings(self) -> list[str]:
        warnings: list[str] = []
        for _texture_id, texture, users in self._referenced_texture_rows():
            if not texture.get("path"):
                continue
            dims = self._texture_dimensions(texture)
            users_text = ", ".join(users)
            if dims is None:
                warnings.append(f"- {texture['name']} ({users_text}): unknown export size")
                continue
            width, height = dims
            if not (self._is_power_of_two(width) and self._is_power_of_two(height)):
                warnings.append(f"- {texture['name']} ({users_text}): {width}x{height}")
        return warnings

    def _mesh_export_counts(self) -> dict[str, int]:
        path = Path(self.input_path.get())
        if not path.exists():
            return {}
        if path.suffix.lower() == ".obj":
            mesh = load_obj(path)
            return {
                "vertices": len(mesh.vertices),
                "triangles": len(mesh.triangles),
                "uv": len(mesh.texcoords),
            }
        mesh_type = detect_mesh_type(path)
        if mesh_type == "Mesh3Dv2":
            mesh = parse_mesh3d2_header(path)
            return {
                "vertices": sum(m.nb_vertices for m in mesh.meshlets),
                "triangles": sum(len(m.triangles) for m in mesh.meshlets),
                "uv": sum(m.nb_texcoords for m in mesh.meshlets),
            }
        parsed = parse_legacy_header(path)
        vertex_arrays = {mesh.vertices for mesh in parsed.meshes.values() if mesh.vertices}
        texcoord_arrays = {mesh.texcoords for mesh in parsed.meshes.values() if mesh.texcoords}
        return {
            "vertices": sum(int(parsed.vertices[name].shape[0]) for name in vertex_arrays if name in parsed.vertices),
            "triangles": sum(mesh.nb_faces for mesh in parsed.meshes.values()),
            "uv": sum(int(parsed.texcoords[name].shape[0]) for name in texcoord_arrays if name in parsed.texcoords),
        }

    def _large_export_warnings(self) -> list[str]:
        warnings: list[str] = []
        red = "#d33333"
        for _texture_id, texture, users in self._referenced_texture_rows():
            if self._texture_size_class(self._texture_effective_size(texture)) == "tex_huge":
                users_text = ", ".join(users)
                warnings.append(f"- texture {texture['name']} ({users_text}): {self._texture_effective_size(texture)}")
        for label, count in self._mesh_export_counts().items():
            if self._mesh_count_color(count) == red:
                warnings.append(f"- {label}: {count}")
        return warnings

    def _build_command(self) -> list[str]:
        input_path = Path(self.input_path.get())
        output_path = Path(self.output_path.get())
        if not input_path.exists():
            raise ValueError("Input mesh does not exist.")
        if not output_path.name:
            raise ValueError("Choose an output header.")
        mesh_layout = LAYOUT_TO_CLI.get(self.layout.get())
        texture_layout = LAYOUT_TO_CLI.get(self.texture_layout.get())
        if mesh_layout not in SUPPORTED_LAYOUTS:
            raise ValueError("Choose a valid mesh file layout.")
        if texture_layout not in SUPPORTED_LAYOUTS:
            raise ValueError("Choose a valid texture file layout.")

        argv = [
            sys.executable,
            "-u",
            str(Path(__file__).resolve().parents[3] / "cli_tools" / "tgx_mesh_cli.py"),
            str(input_path),
            "-o",
            str(output_path),
            "--format",
            self.mesh_format.get(),
            "--layout",
            mesh_layout,
            "--color-type",
            self.color_type.get(),
            "--texture-format",
            self.color_type.get(),
            "--texture-layout",
            texture_layout,
        ]
        if self.object_name.get().strip():
            argv += ["--name", self.object_name.get().strip()]
        if self.normalize.get():
            argv.append("--normalize")
        if self.force_normals.get():
            argv.append("--force-normals")
        if self.remove_normals.get():
            argv.append("--remove-normals")
        if self.mesh_format.get() == "Mesh3Dv2":
            argv.append("--watertight-vertices")
            if self.meshlet_mode.get() == "compact":
                compression = int(round(float(self.compact_compression.get())))
                if compression < 0 or compression > 100:
                    raise ValueError("Non-adjacent packing must be between 0 and 100.")
                argv += ["--compact", "--compact-compression", str(compression)]
            else:
                target_vertices = self.target_vertices.get().strip()
                max_normal_angle = self.max_normal_angle.get().strip()
                if not target_vertices:
                    raise ValueError("Target vertices is required in visibility culling mode.")
                if not max_normal_angle:
                    raise ValueError("Max normal angle is required in visibility culling mode.")
                argv += ["--target-vertices", target_vertices]
                argv += ["--max-normal-angle", max_normal_angle]
        texture_owners: dict[str, tuple[str, str]] = {}
        for row in self.material_rows.values():
            material = row["source"]
            if row["name"] != (material or "default"):
                argv += ["--material-rename", f"{material}={row['name']}"]
            color = "{},{},{}".format(row["color_r"], row["color_g"], row["color_b"])
            argv += ["--material-color", f"{material}={color}"]
            argv += ["--material-ambiant", f"{material}={row['ambiant_strength']}"]
            argv += ["--material-diffuse", f"{material}={row['diffuse_strength']}"]
            argv += ["--material-specular", f"{material}={row['specular_strength']}"]
            argv += ["--material-exponent", f"{material}={row['specular_exponent']}"]
            if row["extended"] == "1":
                emissive = "{},{},{}".format(row["emissive_r"], row["emissive_g"], row["emissive_b"])
                argv += ["--emissive-color", f"{material}={emissive}"]
                argv += ["--emissive-strength", f"{material}={row['emissive_strength']}"]
            else:
                argv += ["--material-simple", material]

            texture_links = [("diffuse", row["diffuse_texture"])]
            if self.mesh_format.get() == "Mesh3Dv2":
                texture_links.append(("emissive", row["emissive_texture"] if row["extended"] == "1" else ""))
            for role, texture_id in texture_links:
                if not texture_id:
                    argv += ["--skip-emissive-texture" if role == "emissive" else "--skip-texture", material]
                    continue
                texture = self.texture_assets.get(texture_id)
                if texture is None:
                    argv += ["--skip-emissive-texture" if role == "emissive" else "--skip-texture", material]
                    continue
                if not texture["path"] and not texture["symbol"]:
                    argv += ["--skip-emissive-texture" if role == "emissive" else "--skip-texture", material]
                    continue
                symbol = sanitize_identifier(texture["name"]) if texture["path"] else (texture["symbol"] or sanitize_identifier(texture["name"]))
                owner = texture_owners.get(texture_id)
                if owner is None:
                    texture_owners[texture_id] = (material, role)
                    if texture["path"]:
                        if role == "emissive":
                            argv += ["--emissive-texture", f"{material}={texture['path']}"]
                            argv += ["--emissive-texture-name", f"{material}={symbol}"]
                            resize_arg = "--emissive-texture-resize"
                            filter_arg = "--emissive-texture-filter"
                        else:
                            argv += ["--texture", f"{material}={texture['path']}"]
                            argv += ["--texture-name", f"{material}={symbol}"]
                            resize_arg = "--texture-resize"
                            filter_arg = "--texture-filter"
                        if texture["resize"].strip():
                            parse_resize(texture["resize"].strip())
                            argv += [resize_arg, f"{material}={texture['resize'].strip()}"]
                        if texture["filter"].strip():
                            argv += [filter_arg, f"{material}={texture['filter'].strip()}"]
                    elif texture["symbol"]:
                        argv += ["--emissive-texture-symbol" if role == "emissive" else "--texture-symbol", f"{material}={texture['symbol']}"]
                    else:
                        argv += ["--skip-emissive-texture" if role == "emissive" else "--skip-texture", material]
                else:
                    argv += ["--emissive-texture-symbol" if role == "emissive" else "--texture-symbol", f"{material}={symbol}"]
        return argv

    def _run_command(self, argv: list[str]):
        output_lines: list[str] = []
        try:
            proc = subprocess.Popen(
                argv,
                cwd=str(Path(__file__).resolve().parents[4]),
                text=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                bufsize=1,
                encoding="utf-8",
                errors="replace",
            )
        except Exception as exc:
            self.after(0, self._append_log, f"error: {exc}")
            self.after(0, self._finish_conversion)
            return
        assert proc.stdout is not None
        for line in proc.stdout:
            text = line.rstrip()
            output_lines.append(text)
            self.after(0, self._append_log, text)
        returncode = proc.wait()
        self.after(0, self._finish_conversion)
        if returncode == 0:
            self.after(0, self._show_conversion_complete, output_lines)
        else:
            self.after(0, messagebox.showerror, "Conversion failed", f"Converter exited with code {returncode}.")

    def _finish_conversion(self):
        self.conversion_running = False
        self._update_mesh_actions()

    def _show_conversion_complete(self, output_lines: list[str]):
        files = self._generated_files_from_output(output_lines)
        if files:
            plural = "file" if len(files) == 1 else "files"
            message = "Conversion completed. {} {} created:\n\n{}".format(len(files), plural, "\n".join(files))
        else:
            message = "Conversion completed."
        messagebox.showinfo("TGX Mesh Converter", message)

    def _generated_files_from_output(self, output_lines: list[str]) -> list[str]:
        files: list[str] = []
        seen: set[str] = set()

        def add_file(path_text: str) -> None:
            path_text = path_text.strip()
            if not path_text:
                return
            key = str(Path(path_text).resolve()).lower()
            if key in seen:
                return
            seen.add(key)
            files.append(path_text)

        for line in output_lines:
            path_text = ""
            if line.startswith("wrote "):
                path_text = line[6:].strip()
            elif (line.startswith("texture: ") or line.startswith("emissive texture: ")) and " -> " in line:
                path_text = line.split(" -> ", 1)[1].split(" (", 1)[0].strip()
            if path_text:
                add_file(path_text)
                cpp_text = str(Path(path_text).with_suffix(".cpp"))
                if Path(cpp_text).exists():
                    add_file(cpp_text)

        output = Path(self.output_path.get())
        if output.name:
            for path in (output, output.with_suffix(".cpp")):
                if path.exists():
                    add_file(str(path))
        return files

    def _append_log(self, text: str):
        self.log.insert("end", text.rstrip() + "\n")
        self.log.see("end")


def main() -> int:
    app = TGXMeshConverterApp()
    app.mainloop()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
