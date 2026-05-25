#!/usr/bin/env python
from __future__ import annotations

import subprocess
import sys
import threading
import tkinter as tk
from pathlib import Path
from tkinter import filedialog, messagebox, ttk

from PIL import Image, ImageTk

REPO_ROOT = Path(__file__).resolve().parents[3]
TOOLS_ROOT = Path(__file__).resolve().parents[2]
if str(REPO_ROOT) not in sys.path:
    sys.path.insert(0, str(REPO_ROOT))
if str(TOOLS_ROOT) not in sys.path:
    sys.path.insert(0, str(TOOLS_ROOT))

from modules.image.core import SUPPORTED_LAYOUTS, SUPPORTED_RESAMPLING, parse_resize, sanitize_identifier
from modules.mesh.textures import resolve_mesh_textures
from tools.mesh3d2_converter.obj_loader import load_obj
from tools.mesh3d2_converter.profiles import DEFAULT_MAX_NORMAL_ANGLE_DEG, DEFAULT_TARGET_VERTICES


COLOR_TYPES = ("RGB565", "RGB24", "RGB32")
RESIZE_VALUES = ("", "4", "8", "16", "32", "64", "128", "256", "512", "1024", "2048")


class TGXMeshConverterApp(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("TGX Mesh Converter")
        self.minsize(1120, 720)

        self.input_path = tk.StringVar()
        self.output_path = tk.StringVar()
        self.object_name = tk.StringVar()
        self.mesh_format = tk.StringVar(value="Mesh3Dv2")
        self.layout = tk.StringVar(value="header")
        self.color_type = tk.StringVar(value="RGB565")
        self.normalize = tk.BooleanVar(value=True)
        self.target_vertices = tk.StringVar(value=str(DEFAULT_TARGET_VERTICES))
        self.max_normal_angle = tk.StringVar(value=str(DEFAULT_MAX_NORMAL_ANGLE_DEG))
        self.texture_layout = tk.StringVar(value="header")

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

        self._build_ui()

    def _build_ui(self):
        root = ttk.Frame(self, padding=12)
        root.grid(row=0, column=0, sticky="nsew")
        self.columnconfigure(0, weight=1)
        self.rowconfigure(0, weight=1)
        root.columnconfigure(1, weight=1)
        root.rowconfigure(7, weight=1)
        root.rowconfigure(11, weight=1)

        ttk.Label(root, text="Input OBJ / TGX mesh").grid(row=0, column=0, sticky="w")
        ttk.Entry(root, textvariable=self.input_path).grid(row=0, column=1, columnspan=3, sticky="ew", padx=6)
        ttk.Button(root, text="Browse...", command=self.choose_input).grid(row=0, column=4)

        ttk.Label(root, text="Output mesh .h").grid(row=1, column=0, sticky="w")
        ttk.Entry(root, textvariable=self.output_path).grid(row=1, column=1, columnspan=3, sticky="ew", padx=6)
        ttk.Button(root, text="Browse...", command=self.choose_output).grid(row=1, column=4)

        ttk.Label(root, text="Object name").grid(row=2, column=0, sticky="w")
        ttk.Entry(root, textvariable=self.object_name).grid(row=2, column=1, columnspan=3, sticky="ew", padx=6)

        format_box = ttk.Frame(root)
        format_box.grid(row=3, column=0, columnspan=5, sticky="w")
        format_box.columnconfigure(1, minsize=140)
        format_box.columnconfigure(3, minsize=140)
        ttk.Label(format_box, text="Format").grid(row=0, column=0, sticky="w")
        ttk.Combobox(format_box, textvariable=self.mesh_format, values=("Mesh3Dv2", "Mesh3D"), state="readonly", width=12).grid(row=0, column=1, sticky="w", padx=(6, 20))
        ttk.Label(format_box, text="Color").grid(row=0, column=2, sticky="w")
        ttk.Combobox(format_box, textvariable=self.color_type, values=COLOR_TYPES, state="readonly", width=12).grid(row=0, column=3, sticky="w", padx=(6, 0))

        layout_box = ttk.Frame(root)
        layout_box.grid(row=4, column=0, columnspan=5, sticky="w")
        layout_box.columnconfigure(1, minsize=140)
        ttk.Label(layout_box, text="Mesh files layout").grid(row=0, column=0, sticky="w")
        ttk.Combobox(layout_box, textvariable=self.layout, values=SUPPORTED_LAYOUTS, state="readonly", width=12).grid(row=0, column=1, sticky="w", padx=(6, 0))
        ttk.Label(layout_box, text="Texture files layout").grid(row=1, column=0, sticky="w", pady=(4, 0))
        ttk.Combobox(layout_box, textvariable=self.texture_layout, values=SUPPORTED_LAYOUTS, state="readonly", width=12).grid(row=1, column=1, sticky="w", padx=(6, 0), pady=(4, 0))

        meshlet_box = ttk.Frame(root)
        meshlet_box.grid(row=5, column=1, columnspan=4, sticky="w", padx=6)
        ttk.Checkbutton(root, text="Normalize OBJ", variable=self.normalize).grid(row=5, column=0, sticky="w")
        ttk.Label(meshlet_box, text="target vertices").pack(side="left")
        ttk.Entry(meshlet_box, textvariable=self.target_vertices, width=6).pack(side="left", padx=(4, 14))
        ttk.Label(meshlet_box, text="max normal angle").pack(side="left")
        ttk.Entry(meshlet_box, textvariable=self.max_normal_angle, width=6).pack(side="left", padx=(4, 0))

        ttk.Label(root, text="OBJ material textures").grid(row=6, column=0, sticky="w", pady=(10, 2))
        columns = ("material", "size", "resize", "filter", "refs", "role", "requested", "selected")
        self.texture_table = ttk.Treeview(root, columns=columns, show="headings", height=8)
        for column, width in (
            ("material", 120),
            ("size", 80),
            ("resize", 80),
            ("filter", 80),
            ("refs", 170),
            ("role", 80),
            ("requested", 190),
            ("selected", 220),
        ):
            self.texture_table.heading(column, text=column)
            self.texture_table.column(column, width=width, stretch=column in ("requested", "selected"))
        self.texture_table.grid(row=7, column=0, columnspan=5, sticky="nsew")
        self.texture_table.bind("<<TreeviewSelect>>", self._on_texture_select)

        edit_box = ttk.Frame(root)
        edit_box.grid(row=8, column=0, columnspan=5, sticky="ew", pady=(6, 8))
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
        ttk.Button(edit_box, text="Apply", command=self.apply_material_texture_checked).grid(row=0, column=9, padx=(8, 0))
        ttk.Button(edit_box, text="No texture", command=self.clear_material_texture).grid(row=0, column=10, padx=(8, 0))
        self.texture_preview = ttk.Label(edit_box, text="No texture selected", anchor="center", relief="sunken", compound="top")
        self.texture_preview.grid(row=1, column=1, columnspan=10, sticky="ew", padx=6, pady=(6, 0))

        buttons = ttk.Frame(root)
        buttons.grid(row=9, column=0, columnspan=5, sticky="e")
        ttk.Button(buttons, text="Reload textures", command=self.reload_textures).pack(side="left", padx=(0, 8))
        ttk.Button(buttons, text="Preview mesh", command=self.preview_mesh).pack(side="left", padx=(0, 8))
        self.convert_button = ttk.Button(buttons, text="Convert", command=self.convert)
        self.convert_button.pack(side="left")

        ttk.Label(root, text="Log").grid(row=10, column=0, sticky="w", pady=(8, 0))
        self.log = tk.Text(root, height=12, wrap="word")
        self.log.grid(row=11, column=0, columnspan=5, sticky="nsew", pady=(2, 0))

    def _texture_size_text(self, path_text: str) -> str:
        if not path_text:
            return ""
        path = Path(path_text)
        if not path.exists():
            return "missing"
        try:
            with Image.open(path) as img:
                return f"{img.width}x{img.height}"
        except Exception:
            return "?"

    def _update_texture_preview(self, path_text: str | None = None):
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
            with Image.open(path) as img:
                width, height = img.size
                preview = img.convert("RGBA")
                resample = getattr(Image, "Resampling", Image).LANCZOS
                preview.thumbnail((240, 160), resample)
                self.texture_preview_image = ImageTk.PhotoImage(preview)
        except Exception as exc:
            self.texture_preview.configure(image="", text=f"{path.name}\npreview failed: {exc}")
            return
        self.texture_preview.configure(image=self.texture_preview_image, text=f"{path.name}\n{width}x{height}")

    def choose_input(self):
        path = filedialog.askopenfilename(
            title="Choose mesh",
            filetypes=[("Mesh files", "*.obj *.h *.hpp"), ("All files", "*.*")],
        )
        if not path:
            return
        self.input_path.set(path)
        stem = sanitize_identifier(Path(path).stem)
        if not self.object_name.get().strip():
            self.object_name.set(stem)
        if not self.output_path.get().strip():
            self.output_path.set(str(Path(path).with_name(stem + "_v2.h")))
        self.reload_textures()

    def choose_output(self):
        path = filedialog.asksaveasfilename(title="Output mesh header", defaultextension=".h", filetypes=[("Header", "*.h"), ("All files", "*.*")])
        if path:
            self.output_path.set(path)

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
        if path.suffix.lower() != ".obj" or not path.exists():
            self._append_log("Texture list is available for OBJ inputs.")
            self._update_texture_preview("")
            return
        try:
            mesh = load_obj(path)
            choices = resolve_mesh_textures(mesh)
        except Exception as exc:
            messagebox.showerror("Texture scan failed", str(exc))
            return
        for material, choice in choices.items():
            selected = str(choice.selected) if choice.selected else ""
            requested = str(choice.requested) if choice.requested else ""
            refs = ", ".join(f"{role}:{path.name}" for role, path in sorted(choice.refs.items()))
            size = self._texture_size_text(selected)
            self.texture_rows[material] = {
                "size": size,
                "refs": refs,
                "role": choice.role,
                "requested": requested,
                "selected": selected,
                "resize": "",
                "resize_w": "",
                "resize_h": "",
                "filter": "lanczos",
            }
            iid = f"mat_{len(self.texture_iid_to_material)}"
            self.texture_iid_to_material[iid] = material
            self.texture_material_to_iid[material] = iid
            self.texture_table.insert("", "end", iid=iid, values=(material or "[default]", size, "", "lanczos", refs, choice.role, requested, selected))
        self._append_log(f"Found {len(choices)} material texture slot(s).")
        self._update_texture_preview("")

    def _on_texture_select(self, _event=None):
        selection = self.texture_table.selection()
        if not selection:
            return
        iid = selection[0]
        material = self.texture_iid_to_material.get(iid)
        if material is None:
            return
        row = self.texture_rows.get(material)
        if row is None:
            return
        self.selected_material = material
        self.selected_iid = iid
        self.material_texture_path.set(row["selected"])
        self.material_resize_width.set(row["resize_w"])
        self.material_resize_height.set(row["resize_h"])
        self.material_resample.set(row["filter"])
        self._update_texture_preview(row["selected"])

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
        row["size"] = self._texture_size_text(row["selected"])
        self.texture_table.item(
            self.selected_iid,
            values=(self.selected_material or "[default]", row["size"], row["resize"], row["filter"], row["refs"], row["role"], row["requested"], row["selected"]),
        )
        self._update_texture_preview(row["selected"])

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
        texture_overrides = {material: row["selected"].strip() for material, row in self.texture_rows.items()}
        texture_options = {material: (parse_resize(row["resize"]) if row["resize"].strip() else None, row["filter"].strip() or "lanczos") for material, row in self.texture_rows.items()}
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
                from tools.mesh3d2_converter.viewer import show_meshlets_pyvista

                show_meshlets_pyvista(mesh, texture=True, show_edges=False, texture_options=texture_options)
            else:
                from tools.mesh3d2_converter.mesh_inspect import detect_mesh_type, parse_mesh3d2_header, show_legacy_pyvista, show_mesh3d2_pyvista

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
        self.convert_button.configure(state="disabled")
        threading.Thread(target=self._run_command, args=(argv,), daemon=True).start()

    def _build_command(self) -> list[str]:
        input_path = Path(self.input_path.get())
        output_path = Path(self.output_path.get())
        if not input_path.exists():
            raise ValueError("Input mesh does not exist.")
        if not output_path.name:
            raise ValueError("Choose an output header.")

        argv = [
            sys.executable,
            "-u",
            str(Path(__file__).resolve().parents[2] / "tgx_mesh_cli.py"),
            str(input_path),
            "-o",
            str(output_path),
            "--format",
            self.mesh_format.get(),
            "--layout",
            self.layout.get(),
            "--color-type",
            self.color_type.get(),
            "--texture-format",
            self.color_type.get(),
            "--texture-layout",
            self.texture_layout.get(),
        ]
        if self.object_name.get().strip():
            argv += ["--name", self.object_name.get().strip()]
        if self.normalize.get():
            argv.append("--normalize")
        if self.mesh_format.get() == "Mesh3Dv2":
            argv += ["--target-vertices", self.target_vertices.get().strip()]
            argv += ["--max-normal-angle", self.max_normal_angle.get().strip()]
        input_is_obj = input_path.suffix.lower() == ".obj"
        if input_is_obj:
            for material, row in sorted(self.texture_rows.items()):
                selected = row["selected"].strip()
                if selected:
                    argv += ["--texture", f"{material}={selected}"]
                else:
                    argv += ["--skip-texture", material]
                if row["resize"].strip():
                    parse_resize(row["resize"].strip())
                    argv += ["--texture-resize", f"{material}={row['resize'].strip()}"]
                if row["filter"].strip():
                    argv += ["--texture-filter", f"{material}={row['filter'].strip()}"]
        return argv

    def _run_command(self, argv: list[str]):
        try:
            proc = subprocess.Popen(
                argv,
                cwd=str(Path(__file__).resolve().parents[3]),
                text=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                bufsize=1,
                encoding="utf-8",
                errors="replace",
            )
        except Exception as exc:
            self.after(0, self._append_log, f"error: {exc}")
            self.after(0, self.convert_button.configure, {"state": "normal"})
            return
        assert proc.stdout is not None
        for line in proc.stdout:
            self.after(0, self._append_log, line.rstrip())
        returncode = proc.wait()
        self.after(0, self.convert_button.configure, {"state": "normal"})
        if returncode == 0:
            self.after(0, messagebox.showinfo, "TGX Mesh Converter", "Conversion completed.")
        else:
            self.after(0, messagebox.showerror, "Conversion failed", f"Converter exited with code {returncode}.")

    def _append_log(self, text: str):
        self.log.insert("end", text.rstrip() + "\n")
        self.log.see("end")


def main() -> int:
    app = TGXMeshConverterApp()
    app.mainloop()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
