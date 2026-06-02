#!/usr/bin/env python
from __future__ import annotations

import subprocess
import sys
import threading
import tkinter as tk
from pathlib import Path
from tkinter import filedialog, messagebox, ttk

from PIL import Image, ImageTk

REPO_ROOT = Path(__file__).resolve().parents[4]
TOOLS_ROOT = Path(__file__).resolve().parents[3]
if str(REPO_ROOT) not in sys.path:
    sys.path.insert(0, str(REPO_ROOT))
if str(TOOLS_ROOT) not in sys.path:
    sys.path.insert(0, str(TOOLS_ROOT))

from _internal.modules.image.core import SUPPORTED_LAYOUTS, SUPPORTED_RESAMPLING, parse_resize, sanitize_identifier
from _internal.modules.setup.checks import check_environment
from _internal.modules.mesh.textures import resolve_mesh_textures
from tools._internal.modules.mesh_pipeline.mesh3d_to_mesh3d2 import legacy_to_objmesh, parse_legacy_header
from tools._internal.modules.mesh_pipeline.mesh_inspect import _parse_texture_headers, _parse_texture_image, detect_mesh_type, parse_mesh3d2_header
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

        self.texture_rows: dict[str, dict[str, str]] = {}
        self.texture_iid_to_material: dict[str, str] = {}
        self.texture_material_to_iid: dict[str, str] = {}
        self.selected_material: str | None = None
        self.selected_iid: str | None = None
        self.material_texture_path = tk.StringVar()
        self.material_resize_width = tk.StringVar()
        self.material_resize_height = tk.StringVar()
        self.material_resample = tk.StringVar(value="lanczos")
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
        root.rowconfigure(8, weight=1)
        root.rowconfigure(12, weight=1)

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
        format_label = ttk.Label(format_box, text="Format")
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
        info_box.columnconfigure(2, weight=1)
        self.mesh_size_indicator = tk.Canvas(info_box, width=16, height=16, highlightthickness=0)
        self.mesh_size_indicator.grid(row=0, column=0, rowspan=6, sticky="n", padx=(0, 8), pady=(2, 0))
        for row_index, key in enumerate(("type", "vertices", "triangles", "uv", "normals", "materials")):
            self.mesh_info_vars[key] = tk.StringVar(value="-")
            ttk.Label(info_box, text=key).grid(row=row_index, column=1, sticky="w", padx=(0, 8))
            label = ttk.Label(info_box, textvariable=self.mesh_info_vars[key], anchor="e")
            label.grid(row=row_index, column=2, sticky="e")
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

        texture_header = ttk.Frame(root)
        texture_header.grid(row=7, column=0, columnspan=5, sticky="ew", pady=(10, 2))
        texture_header.columnconfigure(0, weight=1)
        ttk.Label(texture_header, text="Material texture slots").grid(row=0, column=0, sticky="w")
        self.reload_button = ttk.Button(texture_header, text="Reload textures", command=self.reload_textures)
        self.reload_button.grid(row=0, column=1, sticky="e")
        self._tip(self.reload_button, "Rescan materials and texture references from the selected input mesh.")

        columns = ("material", "slot", "size", "filter", "refs", "role", "requested", "selected")
        self.texture_table = ttk.Treeview(root, columns=columns, show=("tree", "headings"), height=8)
        self.texture_table.heading("#0", text="")
        self.texture_table.column("#0", width=42, minwidth=42, stretch=False, anchor="center")
        for column, width in (
            ("material", 120),
            ("slot", 80),
            ("size", 160),
            ("filter", 80),
            ("refs", 170),
            ("role", 80),
            ("requested", 190),
            ("selected", 220),
        ):
            self.texture_table.heading(column, text=column)
            self.texture_table.column(column, width=width, stretch=column in ("requested", "selected"))
        self.texture_table.grid(row=8, column=0, columnspan=5, sticky="nsew")
        self.texture_table.bind("<<TreeviewSelect>>", self._on_texture_select)

        edit_box = ttk.Frame(root)
        edit_box.grid(row=9, column=0, columnspan=5, sticky="ew", pady=(6, 8))
        edit_box.columnconfigure(1, weight=1)
        ttk.Label(edit_box, text="Selected texture").grid(row=0, column=0, sticky="w")
        ttk.Entry(edit_box, textvariable=self.material_texture_path).grid(row=0, column=1, sticky="ew", padx=6)
        ttk.Button(edit_box, text="Browse...", command=self.choose_material_texture).grid(row=0, column=2)
        ttk.Label(edit_box, text="Resize").grid(row=0, column=3, padx=(12, 4))
        ttk.Combobox(edit_box, textvariable=self.material_resize_width, values=RESIZE_VALUES, state="readonly", width=5).grid(row=0, column=4)
        ttk.Label(edit_box, text="x").grid(row=0, column=5, padx=2)
        ttk.Combobox(edit_box, textvariable=self.material_resize_height, values=RESIZE_VALUES, state="readonly", width=5).grid(row=0, column=6)
        ttk.Label(edit_box, text="Filter").grid(row=0, column=7, padx=(12, 4))
        ttk.Combobox(edit_box, textvariable=self.material_resample, values=SUPPORTED_RESAMPLING, state="readonly", width=10).grid(row=0, column=8)
        apply_texture = ttk.Button(edit_box, text="Apply", command=self.apply_material_texture_checked)
        apply_texture.grid(row=0, column=9, padx=(8, 0))
        self._tip(apply_texture, "Apply the selected texture, resize and filter settings to this material.")
        no_texture = ttk.Button(edit_box, text="No texture", command=self.clear_material_texture)
        no_texture.grid(row=0, column=10, padx=(8, 0))
        self._tip(no_texture, "Remove the texture from this material in the generated mesh.")
        self.texture_preview = ttk.Label(edit_box, text="No texture selected", anchor="center", relief="sunken", compound="top")
        self.texture_preview.grid(row=1, column=1, columnspan=10, sticky="ew", padx=6, pady=(6, 0))

        buttons = ttk.Frame(root)
        buttons.grid(row=10, column=0, columnspan=5, sticky="ew")
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

        ttk.Label(root, text="Log").grid(row=11, column=0, sticky="w", pady=(8, 0))
        self.log = tk.Text(root, height=12, wrap="word")
        self.log.grid(row=12, column=0, columnspan=5, sticky="nsew", pady=(2, 0))

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
        if path.suffix.lower() in (".h", ".hpp", ".cpp", ".cxx", ".cc"):
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

    def _texture_row_id(self, material: str, slot: str) -> str:
        return f"{slot}:{material}"

    def _texture_rows_for_slot(self, slot: str) -> dict[str, dict[str, str]]:
        return {row["material"]: row for row in self.texture_rows.values() if row.get("slot") == slot}

    def _update_texture_preview(self, path_text: str | None = None, symbol: str = ""):
        path_text = self.material_texture_path.get().strip() if path_text is None else path_text.strip()
        self.texture_preview_image = None
        if not path_text:
            self.texture_preview.configure(image="", text="No texture selected")
            return
        path = Path(path_text)
        if not path.exists():
            self.texture_preview.configure(image="", text=f"{path.name}\nmissing file")
            return
        try:
            if path.suffix.lower() in (".h", ".hpp", ".cpp", ".cxx", ".cc"):
                texture = _parse_texture_image(path, symbol)
                if texture is None:
                    raise ValueError("could not parse TGX texture header")
                width, height = texture.width, texture.height
                preview = Image.fromarray(texture.rgb, "RGB").convert("RGBA")
            else:
                with Image.open(path) as img:
                    width, height = img.size
                    preview = img.convert("RGBA")
            resample = getattr(Image, "Resampling", Image).LANCZOS
            preview.thumbnail((240, 160), resample)
            self.texture_preview_image = ImageTk.PhotoImage(preview)
        except Exception as exc:
            self.texture_preview.configure(image="", text=f"{path.name}\npreview failed: {exc}")
            return
        title = f"{symbol}\n" if symbol else ""
        self.texture_preview.configure(image=self.texture_preview_image, text=f"{title}{path.name}\n{width}x{height}")

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
                textured = sum(1 for mat in mesh.materials.values() if mat.texture_path)
                emissive_textured = sum(1 for mat in mesh.materials.values() if mat.emissive_texture_path)
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
                        "materials": f"{len(mesh.materials)} ({textured} tex, {emissive_textured} emit)",
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
        self.texture_rows.clear()
        self.texture_iid_to_material.clear()
        self.texture_material_to_iid.clear()
        self.selected_material = None
        self.selected_iid = None
        self.material_texture_path.set("")
        self.material_resize_width.set("")
        self.material_resize_height.set("")
        self.material_resample.set("lanczos")
        self.texture_table.delete(*self.texture_table.get_children())
        path = Path(self.input_path.get())
        self._update_mesh_info(path)
        if not path.exists():
            self._update_texture_preview("")
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
        materials = sorted(set(choices) | set(emissive_choices))
        for material in materials:
            choice = choices.get(material)
            if choice is not None:
                selected = str(choice.selected) if choice.selected else ""
                requested = str(choice.requested) if choice.requested else ""
                refs = ", ".join(f"{role}:{path.name}" for role, path in sorted(choice.refs.items()))
                self._add_texture_row(material, slot="diffuse", symbol="", selected=selected, refs=refs, role=choice.role, requested=requested)
            emissive_choice = emissive_choices.get(material)
            if emissive_choice is not None:
                selected = str(emissive_choice.selected) if emissive_choice.selected else ""
                requested = str(emissive_choice.requested) if emissive_choice.requested else ""
                refs = ", ".join(f"{role}:{path.name}" for role, path in sorted(emissive_choice.refs.items()))
                self._add_texture_row(material, slot="emissive", symbol="", selected=selected, refs=refs, role=emissive_choice.role, requested=requested)
        self._append_log(f"Found {len(self.texture_rows)} material texture slot(s).")
        self._update_texture_preview("")

    def _add_texture_row(self, material: str, *, slot: str, symbol: str, selected: str, refs: str, role: str = "TGX", requested: str = "") -> None:
        row_id = self._texture_row_id(material, slot)
        size = self._texture_size_text(selected, symbol)
        self.texture_rows[row_id] = {
            "material": material,
            "slot": slot,
            "size": size,
            "symbol": symbol,
            "refs": refs,
            "role": role,
            "requested": requested,
            "selected": selected,
            "resize": "",
            "resize_w": "",
            "resize_h": "",
            "filter": "lanczos",
        }
        iid = f"mat_{len(self.texture_iid_to_material)}"
        self.texture_iid_to_material[iid] = row_id
        self.texture_material_to_iid[row_id] = iid
        self.texture_table.insert(
            "",
            "end",
            iid=iid,
            image=self._texture_size_icon(self.texture_rows[row_id]),
            values=(material or "[default]", slot, self._texture_size_display(self.texture_rows[row_id]), "lanczos", refs, role, requested, selected),
        )

    def _reload_tgx_textures(self, path: Path) -> None:
        try:
            mesh_type = detect_mesh_type(path)
            if mesh_type == "Mesh3Dv2":
                mesh = parse_mesh3d2_header(path)
                for index, material in enumerate(mesh.materials):
                    symbol = material.texture_symbol
                    selected = str(mesh.texture_headers.get(symbol, "")) if symbol else ""
                    self._add_texture_row(f"mat{index}", slot="diffuse", symbol=symbol, selected=selected, refs=symbol, requested=symbol)
                    emissive_symbol = material.emissive_texture_symbol
                    emissive_selected = str(mesh.texture_headers.get(emissive_symbol, "")) if emissive_symbol else ""
                    self._add_texture_row(f"mat{index}", slot="emissive", symbol=emissive_symbol, selected=emissive_selected, refs=emissive_symbol, requested=emissive_symbol)
            else:
                parsed = parse_legacy_header(path)
                _obj, texture_symbols, _chain = legacy_to_objmesh(parsed)
                raw = path.read_text(encoding="utf-8", errors="replace")
                texture_headers = _parse_texture_headers(raw, path.resolve().parent)
                for material in sorted(_obj.materials):
                    symbol = texture_symbols.get(material, "")
                    selected = str(texture_headers.get(symbol, "")) if symbol else ""
                    self._add_texture_row(material, slot="diffuse", symbol=symbol, selected=selected, refs=symbol, requested=symbol)
                    self._add_texture_row(material, slot="emissive", symbol="", selected="", refs="", requested="")
        except Exception as exc:
            messagebox.showerror("Texture scan failed", str(exc))
            return
        if self.texture_rows:
            self._append_log(f"Found {len(self.texture_rows)} TGX texture reference(s).")
        else:
            self._append_log("No TGX texture reference found in this mesh.")
        self._update_texture_preview("")

    def _on_texture_select(self, _event=None):
        selection = self.texture_table.selection()
        if not selection:
            return
        iid = selection[0]
        row_id = self.texture_iid_to_material.get(iid)
        if row_id is None:
            return
        row = self.texture_rows.get(row_id)
        if row is None:
            return
        self.selected_material = row_id
        self.selected_iid = iid
        self.material_texture_path.set(row["selected"])
        self.material_resize_width.set(row["resize_w"])
        self.material_resize_height.set(row["resize_h"])
        self.material_resample.set(row["filter"])
        self._update_texture_preview(row["selected"], row.get("symbol", ""))

    def choose_material_texture(self):
        path = filedialog.askopenfilename(
            title="Choose texture",
            filetypes=[("Images", "*.png *.jpg *.jpeg *.bmp *.tga *.tif *.tiff *.gif"), ("All files", "*.*")],
        )
        if path:
            self.material_texture_path.set(path)
            if self.selected_material is not None:
                self.apply_material_texture()
            else:
                self._update_texture_preview(path)

    def apply_material_texture(self):
        if self.selected_material is None or self.selected_iid is None:
            return
        width = self.material_resize_width.get().strip()
        height = self.material_resize_height.get().strip()
        if bool(width) != bool(height):
            raise ValueError("Choose both resize width and height, or leave both empty.")
        resize = f"{width}x{height}" if width and height else ""
        if resize:
            parse_resize(resize)
        resample = self.material_resample.get().strip().lower() or "lanczos"
        if resample not in SUPPORTED_RESAMPLING:
            raise ValueError("Unsupported texture filter '{}'. Choose one of: {}".format(resample, ", ".join(SUPPORTED_RESAMPLING)))
        row = self.texture_rows[self.selected_material]
        row["selected"] = self.material_texture_path.get().strip()
        row["resize"] = resize
        row["resize_w"] = width
        row["resize_h"] = height
        row["filter"] = resample
        row["size"] = self._texture_size_text(row["selected"], row.get("symbol", ""))
        self.texture_table.item(
            self.selected_iid,
            image=self._texture_size_icon(row),
            values=(row["material"] or "[default]", row["slot"], self._texture_size_display(row), row["filter"], row["refs"], row["role"], row["requested"], row["selected"]),
        )
        self._update_texture_preview(row["selected"], row.get("symbol", ""))

    def apply_material_texture_checked(self):
        try:
            self.apply_material_texture()
        except Exception as exc:
            messagebox.showerror("Invalid texture options", str(exc))

    def clear_material_texture(self):
        self.material_texture_path.set("")
        self.material_resize_width.set("")
        self.material_resize_height.set("")
        self.material_resample.set("lanczos")
        self.apply_material_texture()

    def _sync_selected_texture_editor(self):
        if self.selected_material is not None:
            self.apply_material_texture()

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
        diffuse_rows = self._texture_rows_for_slot("diffuse")
        texture_overrides = {material: row["selected"].strip() for material, row in diffuse_rows.items()}
        texture_options = {material: (parse_resize(row["resize"]) if row["resize"].strip() else None, row["filter"].strip() or "lanczos") for material, row in diffuse_rows.items()}
        self._append_log(f"Opening PyVista preview for {path.name}...")
        threading.Thread(target=self._preview_mesh_worker, args=(path, texture_overrides, texture_options), daemon=True).start()

    def _preview_mesh_worker(self, path: Path, texture_overrides: dict[str, str], texture_options: dict[str, tuple[tuple[int, int] | None, str]]):
        try:
            if path.suffix.lower() == ".obj":
                mesh = load_obj(path)
                if not texture_overrides:
                    choices = resolve_mesh_textures(mesh)
                    texture_overrides = {material: str(choice.selected) for material, choice in choices.items() if choice.selected}
                    texture_options = {material: (None, "lanczos") for material in texture_overrides}
                for material, selected in texture_overrides.items():
                    if material in mesh.materials:
                        mesh.materials[material].texture_path = Path(selected) if selected else None
                from tools._internal.modules.mesh_pipeline.viewer import show_meshlets_pyvista

                show_meshlets_pyvista(mesh, texture=True, show_edges=False, texture_options=texture_options)
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

    def convert(self):
        try:
            self._sync_selected_texture_editor()
            argv = self._build_command()
        except Exception as exc:
            messagebox.showerror("Invalid options", str(exc))
            return
        self._append_log("> " + subprocess.list2cmdline(argv))
        self.conversion_running = True
        self._update_mesh_actions()
        threading.Thread(target=self._run_command, args=(argv,), daemon=True).start()

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
        for _row_id, row in sorted(self.texture_rows.items()):
            material = row["material"]
            selected = row["selected"].strip()
            slot = row["slot"]
            if slot == "emissive":
                texture_arg = "--emissive-texture"
                skip_arg = "--skip-emissive-texture"
                resize_arg = "--emissive-texture-resize"
                filter_arg = "--emissive-texture-filter"
            else:
                texture_arg = "--texture"
                skip_arg = "--skip-texture"
                resize_arg = "--texture-resize"
                filter_arg = "--texture-filter"
            if selected:
                argv += [texture_arg, f"{material}={selected}"]
            else:
                argv += [skip_arg, material]
            if row["resize"].strip():
                parse_resize(row["resize"].strip())
                argv += [resize_arg, f"{material}={row['resize'].strip()}"]
            if row["filter"].strip():
                argv += [filter_arg, f"{material}={row['filter'].strip()}"]
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
            elif line.startswith("texture: ") and " -> " in line:
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
