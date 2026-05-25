#!/usr/bin/env python
from __future__ import annotations

import tkinter as tk
from pathlib import Path
from tkinter import filedialog, messagebox, ttk

from .core import ImageExportOptions, SUPPORTED_COLOR_TYPES, SUPPORTED_RESAMPLING, export_image, parse_resize, sanitize_identifier

try:
    from PIL import Image, ImageTk
except ImportError as exc:  # pragma: no cover - user-facing dependency error
    raise RuntimeError("Pillow is required. Install it with: python -m pip install pillow") from exc


class TGXImageConverterApp(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("TGX Image Converter")
        self.minsize(720, 460)
        self.input_path = tk.StringVar()
        self.output_dir = tk.StringVar(value=str(Path.cwd()))
        self.object_name = tk.StringVar()
        self.output_base = tk.StringVar()
        self.color_type = tk.StringVar(value="RGB565")
        self.layout = tk.StringVar(value="header")
        self.resize_enabled = tk.BooleanVar(value=False)
        self.resize_w = tk.StringVar()
        self.resize_h = tk.StringVar()
        self.resample = tk.StringVar(value="lanczos")
        self.flip_y = tk.BooleanVar(value=False)
        self.progmem = tk.BooleanVar(value=True)
        self.premultiply_alpha = tk.BooleanVar(value=True)
        self.preview_image = None
        self.resize_width_entry = None
        self.resize_height_entry = None
        self.resample_combo = None
        self._build_ui()
        self._update_resize_state()

    def _build_ui(self):
        root = ttk.Frame(self, padding=12)
        root.grid(row=0, column=0, sticky="nsew")
        self.columnconfigure(0, weight=1)
        self.rowconfigure(0, weight=1)
        root.columnconfigure(1, weight=1)
        root.rowconfigure(12, weight=1)

        ttk.Label(root, text="Input image").grid(row=0, column=0, sticky="w")
        ttk.Entry(root, textvariable=self.input_path).grid(row=0, column=1, sticky="ew", padx=6)
        ttk.Button(root, text="Browse...", command=self.choose_input).grid(row=0, column=2)

        ttk.Label(root, text="Output folder").grid(row=1, column=0, sticky="w")
        ttk.Entry(root, textvariable=self.output_dir).grid(row=1, column=1, sticky="ew", padx=6)
        ttk.Button(root, text="Browse...", command=self.choose_output).grid(row=1, column=2)

        ttk.Label(root, text="Object name").grid(row=2, column=0, sticky="w")
        ttk.Entry(root, textvariable=self.object_name).grid(row=2, column=1, sticky="ew", padx=6)

        ttk.Label(root, text="File basename").grid(row=3, column=0, sticky="w")
        ttk.Entry(root, textvariable=self.output_base).grid(row=3, column=1, sticky="ew", padx=6)

        ttk.Label(root, text="Color type").grid(row=4, column=0, sticky="w")
        ttk.Combobox(root, textvariable=self.color_type, values=SUPPORTED_COLOR_TYPES, state="readonly", width=12).grid(row=4, column=1, sticky="w", padx=6)

        ttk.Label(root, text="Output").grid(row=5, column=0, sticky="w")
        layout_box = ttk.Frame(root)
        layout_box.grid(row=5, column=1, sticky="w", padx=6)
        ttk.Radiobutton(layout_box, text="Single .h", variable=self.layout, value="header").pack(side="left")
        ttk.Radiobutton(layout_box, text=".h + .cpp", variable=self.layout, value="split").pack(side="left", padx=(16, 0))

        resize_box = ttk.Frame(root)
        resize_box.grid(row=6, column=1, sticky="w", padx=6)
        ttk.Checkbutton(root, text="Resize", variable=self.resize_enabled, command=self._update_resize_state).grid(row=6, column=0, sticky="w")
        self.resize_width_entry = ttk.Entry(resize_box, textvariable=self.resize_w, width=8)
        self.resize_width_entry.pack(side="left")
        ttk.Label(resize_box, text=" x ").pack(side="left")
        self.resize_height_entry = ttk.Entry(resize_box, textvariable=self.resize_h, width=8)
        self.resize_height_entry.pack(side="left")
        ttk.Label(resize_box, text="  filter").pack(side="left", padx=(12, 4))
        self.resample_combo = ttk.Combobox(resize_box, textvariable=self.resample, values=SUPPORTED_RESAMPLING, state="readonly", width=10)
        self.resample_combo.pack(side="left")

        ttk.Checkbutton(root, text="Flip vertically for texture UVs", variable=self.flip_y).grid(row=7, column=1, sticky="w", padx=6)
        ttk.Checkbutton(root, text="Store pixel data in PROGMEM", variable=self.progmem).grid(row=8, column=1, sticky="w", padx=6)
        ttk.Checkbutton(root, text="Pre-multiply alpha for RGB32/RGB64", variable=self.premultiply_alpha).grid(row=9, column=1, sticky="w", padx=6)

        ttk.Button(root, text="Convert", command=self.convert).grid(row=10, column=1, sticky="e", padx=6, pady=(8, 4))

        self.preview = ttk.Label(root, text="No image selected", anchor="center", relief="sunken")
        self.preview.grid(row=0, column=3, rowspan=11, sticky="nsew", padx=(14, 0))
        root.columnconfigure(3, weight=1)

        ttk.Label(root, text="Log").grid(row=11, column=0, sticky="w", pady=(8, 0))
        self.log = tk.Text(root, height=8, wrap="word")
        self.log.grid(row=12, column=0, columnspan=4, sticky="nsew", pady=(2, 0))

    def choose_input(self):
        path = filedialog.askopenfilename(
            title="Choose image",
            filetypes=[
                ("Images", "*.png *.jpg *.jpeg *.bmp *.gif *.tif *.tiff"),
                ("All files", "*.*"),
            ],
        )
        if not path:
            return
        self.input_path.set(path)
        stem = sanitize_identifier(Path(path).stem)
        if not self.object_name.get().strip():
            self.object_name.set(stem)
        if not self.output_base.get().strip():
            self.output_base.set(stem)
        self._update_preview(path)

    def choose_output(self):
        path = filedialog.askdirectory(title="Choose output folder")
        if path:
            self.output_dir.set(path)

    def _update_preview(self, path):
        try:
            image = Image.open(path).convert("RGBA")
            original_width, original_height = image.size
            self.resize_w.set(str(original_width))
            self.resize_h.set(str(original_height))
            image.thumbnail((260, 220))
            self.preview_image = ImageTk.PhotoImage(image)
            self.preview.configure(image=self.preview_image, text="{} x {}".format(original_width, original_height), compound="top")
        except Exception as exc:
            self.preview.configure(image="", text="Preview failed:\n{}".format(exc))

    def _update_resize_state(self):
        state = "normal" if self.resize_enabled.get() else "disabled"
        combo_state = "readonly" if self.resize_enabled.get() else "disabled"
        if self.resize_width_entry is not None:
            self.resize_width_entry.configure(state=state)
        if self.resize_height_entry is not None:
            self.resize_height_entry.configure(state=state)
        if self.resample_combo is not None:
            self.resample_combo.configure(state=combo_state)

    def _resize_value(self):
        if not self.resize_enabled.get():
            return None
        return parse_resize("{}x{}".format(self.resize_w.get().strip(), self.resize_h.get().strip()))

    def _append_log(self, text):
        self.log.insert("end", text + "\n")
        self.log.see("end")

    def convert(self):
        try:
            result = export_image(
                ImageExportOptions(
                    input_path=Path(self.input_path.get()),
                    output_dir=Path(self.output_dir.get()),
                    color_type=self.color_type.get(),
                    object_name=self.object_name.get().strip() or None,
                    output_base=self.output_base.get().strip() or None,
                    layout=self.layout.get(),
                    resize=self._resize_value(),
                    resample=self.resample.get(),
                    flip_y=self.flip_y.get(),
                    progmem=self.progmem.get(),
                    premultiply_alpha=self.premultiply_alpha.get(),
                )
            )
        except Exception as exc:
            messagebox.showerror("Conversion failed", str(exc))
            self._append_log("error: {}".format(exc))
            return

        lines = [
            "Generated {}".format(result.object_name),
            "  format: tgx::{}".format(result.color_type),
            "  size: {}x{}".format(result.width, result.height),
            "  memory: {} bytes ({:.1f} KiB)".format(result.data_bytes, result.data_bytes / 1024.0),
            "  header: {}".format(result.header_path),
        ]
        if result.cpp_path:
            lines.append("  source: {}".format(result.cpp_path))
        self._append_log("\n".join(lines))
        messagebox.showinfo("TGX Image Converter", "Conversion completed.")


def main() -> int:
    app = TGXImageConverterApp()
    app.mainloop()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
