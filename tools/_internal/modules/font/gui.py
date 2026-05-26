#!/usr/bin/env python
from __future__ import annotations

import tkinter as tk
from pathlib import Path
from tkinter import filedialog, messagebox, ttk

from .core import (
    CHARACTER_PRESETS,
    FontExportOptions,
    available_codepoints,
    export_font,
    font_family_name,
    normalize_format,
    parse_character_selection,
    parse_sizes,
    sanitize_identifier,
)

try:
    from PIL import Image, ImageDraw, ImageFont, ImageTk
except ImportError as exc:  # pragma: no cover - user-facing dependency error
    raise RuntimeError("Pillow is required. Install it with: python -m pip install pillow") from exc


class TGXFontConverterApp(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("TGX Font Converter")
        self.minsize(920, 720)

        self.input_path = tk.StringVar()
        self.output_path = tk.StringVar()
        self.object_name = tk.StringVar()
        self.format = tk.StringVar(value="ili9341-v23")
        self.antialias = tk.StringVar(value="4")
        self.sizes = tk.StringVar(value="12,16,24")
        self.layout = tk.StringVar(value="header")
        self.progmem = tk.BooleanVar(value=True)
        self.preset = tk.StringVar(value="printable")
        self.preview_png = tk.BooleanVar(value=False)

        self.available: set[int] = set()
        self.selected: set[int] = set()
        self.font_path: Path | None = None
        self.grid_image = None
        self.convert_button: ttk.Button | None = None
        self.preview_check: ttk.Checkbutton | None = None

        self._build_ui()
        self._update_controls()

    def _build_ui(self):
        root = ttk.Frame(self, padding=12)
        root.grid(row=0, column=0, sticky="nsew")
        self.columnconfigure(0, weight=1)
        self.rowconfigure(0, weight=1)
        root.columnconfigure(1, weight=1)
        root.rowconfigure(11, weight=1)

        ttk.Label(root, text="Input font").grid(row=0, column=0, sticky="w")
        ttk.Entry(root, textvariable=self.input_path).grid(row=0, column=1, sticky="ew", padx=6)
        ttk.Button(root, text="Browse...", command=self.choose_font).grid(row=0, column=2)

        ttk.Label(root, text="Output file").grid(row=1, column=0, sticky="w")
        ttk.Entry(root, textvariable=self.output_path).grid(row=1, column=1, sticky="ew", padx=6)
        ttk.Button(root, text="Browse...", command=self.choose_output).grid(row=1, column=2)

        ttk.Label(root, text="Object name").grid(row=2, column=0, sticky="w")
        ttk.Entry(root, textvariable=self.object_name).grid(row=2, column=1, sticky="ew", padx=6)

        fmt_row = ttk.Frame(root)
        fmt_row.grid(row=3, column=1, sticky="w", padx=6)
        ttk.Label(root, text="Format").grid(row=3, column=0, sticky="w")
        fmt_combo = ttk.Combobox(
            fmt_row,
            textvariable=self.format,
            values=("ili9341-v23", "ili9341-v1", "adafruit-gfx"),
            state="readonly",
            width=16,
        )
        fmt_combo.pack(side="left")
        fmt_combo.bind("<<ComboboxSelected>>", lambda _event: self._update_controls())
        ttk.Label(fmt_row, text="  antialias").pack(side="left", padx=(14, 4))
        self.aa_combo = ttk.Combobox(fmt_row, textvariable=self.antialias, values=("none", "2", "4", "8"), state="readonly", width=8)
        self.aa_combo.pack(side="left")

        ttk.Label(root, text="Sizes").grid(row=4, column=0, sticky="w")
        ttk.Entry(root, textvariable=self.sizes, width=24).grid(row=4, column=1, sticky="w", padx=6)

        ttk.Label(root, text="Output layout").grid(row=5, column=0, sticky="w")
        layout_row = ttk.Frame(root)
        layout_row.grid(row=5, column=1, sticky="w", padx=6)
        ttk.Radiobutton(layout_row, text="Single .h", variable=self.layout, value="header").pack(side="left")
        ttk.Radiobutton(layout_row, text=".h + .cpp", variable=self.layout, value="split").pack(side="left", padx=(16, 0))
        ttk.Checkbutton(layout_row, text="PROGMEM", variable=self.progmem).pack(side="left", padx=(16, 0))
        self.preview_check = ttk.Checkbutton(layout_row, text="Write preview PNG", variable=self.preview_png)
        self.preview_check.pack(side="left", padx=(16, 0))

        ttk.Label(root, text="Character preset").grid(row=6, column=0, sticky="w")
        preset_row = ttk.Frame(root)
        preset_row.grid(row=6, column=1, sticky="w", padx=6)
        preset_combo = ttk.Combobox(preset_row, textvariable=self.preset, values=CHARACTER_PRESETS, state="readonly", width=18)
        preset_combo.pack(side="left")
        preset_combo.bind("<<ComboboxSelected>>", lambda _event: self.apply_preset())
        ttk.Button(preset_row, text="Apply", command=self.apply_preset).pack(side="left", padx=(8, 0))
        ttk.Button(preset_row, text="Clear", command=lambda: self._set_selection(set())).pack(side="left", padx=(8, 0))
        ttk.Button(preset_row, text="All available", command=lambda: self._set_selection(set(self.available))).pack(side="left", padx=(8, 0))

        self.info = ttk.Label(root, text="No font selected", anchor="w")
        self.info.grid(row=7, column=0, columnspan=3, sticky="ew", pady=(8, 2))

        grid_frame = ttk.Frame(root)
        grid_frame.grid(row=8, column=0, columnspan=3, sticky="nsew")
        grid_frame.columnconfigure(0, weight=1)
        grid_frame.rowconfigure(0, weight=1)
        self.grid = tk.Canvas(grid_frame, width=760, height=330, background="#202226", highlightthickness=1, highlightbackground="#777")
        scroll = ttk.Scrollbar(grid_frame, orient="vertical", command=self.grid.yview)
        self.grid.configure(yscrollcommand=scroll.set)
        self.grid.grid(row=0, column=0, sticky="nsew")
        scroll.grid(row=0, column=1, sticky="ns")
        self.grid.bind("<Button-1>", self._grid_click)

        action_row = ttk.Frame(root)
        action_row.grid(row=9, column=0, columnspan=3, sticky="ew", pady=(10, 4))
        action_row.columnconfigure(0, weight=1)
        self.convert_button = ttk.Button(action_row, text="Convert", command=self.convert)
        self.convert_button.grid(row=0, column=1, sticky="e")

        ttk.Label(root, text="Log").grid(row=10, column=0, sticky="w", pady=(8, 0))
        self.log = tk.Text(root, height=8, wrap="word")
        self.log.grid(row=11, column=0, columnspan=3, sticky="nsew", pady=(2, 0))

    def choose_font(self):
        path = filedialog.askopenfilename(
            title="Choose font",
            filetypes=[
                ("Font files", "*.ttf *.otf *.ttc"),
                ("All files", "*.*"),
            ],
        )
        if path:
            self.load_font(Path(path))

    def choose_output(self):
        path = filedialog.asksaveasfilename(
            title="Choose output header",
            defaultextension=".h",
            filetypes=[("Header", "*.h"), ("All files", "*.*")],
            initialfile=Path(self.output_path.get()).name if self.output_path.get() else "",
        )
        if path:
            self.output_path.set(path)

    def load_font(self, path: Path):
        try:
            available = available_codepoints(path)
            family = font_family_name(path)
        except Exception as exc:
            messagebox.showerror("Font load failed", str(exc))
            return

        self.font_path = path
        self.available = available
        self.input_path.set(str(path))
        base = sanitize_identifier(path.stem, "font")
        self.object_name.set(base)
        self.output_path.set(str(path.with_name(base + ".h")))
        self._set_selection(parse_character_selection(self.preset.get(), self.available), redraw=False)
        self._append_log(f"Loaded {family}: {path}")
        self._redraw_grid()
        self._update_controls()

    def apply_preset(self):
        if not self.font_path:
            return
        self._set_selection(parse_character_selection(self.preset.get(), self.available))

    def _set_selection(self, selected: set[int], *, redraw: bool = True):
        self.selected = {cp for cp in selected if cp in self.available and 0 <= cp <= 255}
        if redraw:
            self._redraw_grid()
        self._update_controls()

    def _update_controls(self):
        has_font = self.font_path is not None
        if self.convert_button is not None:
            self.convert_button.configure(state="normal" if has_font and self.selected else "disabled")
        if normalize_format(self.format.get()) in ("ili9341-v1", "adafruit-gfx"):
            self.antialias.set("none")
            self.aa_combo.configure(state="disabled")
        else:
            self.aa_combo.configure(state="readonly")
        if has_font:
            self.info.configure(
                text=f"{Path(self.input_path.get()).name}: {len(self.available)} available codepoints, {len(self.selected)} selected"
            )
        else:
            self.info.configure(text="No font selected")

    def _grid_click(self, event):
        if not self.font_path:
            return
        x = self.grid.canvasx(event.x)
        y = self.grid.canvasy(event.y)
        col = int(x // 44)
        row = int(y // 34)
        cp = row * 16 + col
        if 0 <= cp <= 255 and cp in self.available:
            if cp in self.selected:
                self.selected.remove(cp)
            else:
                self.selected.add(cp)
            self._redraw_grid()
            self._update_controls()

    def _redraw_grid(self):
        self.grid.delete("all")
        if not self.font_path:
            self.grid.create_text(380, 160, text="No font selected", fill="#ddd")
            return

        cell_w, cell_h = 44, 34
        width, height = cell_w * 16, cell_h * 16
        image = Image.new("RGB", (width, height), (32, 34, 38))
        draw = ImageDraw.Draw(image)
        try:
            font = ImageFont.truetype(str(self.font_path), 18)
        except Exception:
            font = ImageFont.load_default()
        tiny = ImageFont.load_default()

        for cp in range(256):
            col, row = cp % 16, cp // 16
            x0, y0 = col * cell_w, row * cell_h
            if cp not in self.available:
                bg = (46, 46, 48)
                fg = (92, 92, 96)
            elif cp in self.selected:
                bg = (46, 82, 56)
                fg = (245, 245, 245)
            else:
                bg = (58, 58, 64)
                fg = (160, 160, 168)
            draw.rectangle((x0, y0, x0 + cell_w - 1, y0 + cell_h - 1), fill=bg, outline=(85, 85, 92))
            label = f"{cp:02X}"
            draw.text((x0 + 2, y0 + cell_h - 10), label, fill=(160, 160, 165), font=tiny)
            if cp >= 32:
                ch = chr(cp)
                bbox = draw.textbbox((0, 0), ch, font=font)
                tw = bbox[2] - bbox[0]
                th = bbox[3] - bbox[1]
                draw.text((x0 + (cell_w - tw) / 2 - bbox[0], y0 + 4 + (20 - th) / 2 - bbox[1]), ch, fill=fg, font=font)

        self.grid_image = ImageTk.PhotoImage(image)
        self.grid.create_image(0, 0, image=self.grid_image, anchor="nw")
        self.grid.configure(scrollregion=(0, 0, width, height))

    def _preview_path(self) -> Path | None:
        if not self.preview_png.get():
            return None
        output = Path(self.output_path.get())
        return output.with_name(output.stem + "_preview.png")

    def _append_log(self, text: str):
        self.log.insert("end", text + "\n")
        self.log.see("end")

    def convert(self):
        if self.font_path is None:
            return
        try:
            sizes = parse_sizes(self.sizes.get())
            result = export_font(
                FontExportOptions(
                    input_path=self.font_path,
                    output=Path(self.output_path.get()),
                    symbol_base=self.object_name.get().strip() or None,
                    sizes=sizes,
                    output_format=self.format.get(),
                    antialias=self.antialias.get(),
                    chars=",".join(str(cp) for cp in sorted(self.selected)),
                    layout=self.layout.get(),
                    progmem=self.progmem.get(),
                    preview_png=self._preview_path(),
                )
            )
        except Exception as exc:
            messagebox.showerror("Conversion failed", str(exc))
            self._append_log(f"error: {exc}")
            return

        lines = [
            "Generated TGX font:",
            f"  symbols: {', '.join(result.symbols)}",
            f"  glyphs: {result.glyph_count}",
            f"  memory: {result.data_bytes} bytes ({result.data_bytes / 1024.0:.1f} KiB)",
            f"  header: {result.header_path}",
        ]
        files = [str(result.header_path)]
        if result.cpp_path:
            lines.append(f"  source: {result.cpp_path}")
            files.append(str(result.cpp_path))
        if result.preview_path:
            lines.append(f"  preview: {result.preview_path}")
            files.append(str(result.preview_path))
        self._append_log("\n".join(lines))
        messagebox.showinfo(
            "TGX Font Converter",
            "Conversion complete. {} file{} created:\n\n{}".format(
                len(files),
                "" if len(files) == 1 else "s",
                "\n".join(files),
            ),
        )


def main() -> int:
    app = TGXFontConverterApp()
    app.mainloop()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
