from __future__ import annotations

import threading
import tkinter as tk
from pathlib import Path
from tkinter import filedialog, messagebox, ttk

from PIL import Image, ImageTk

from .core import InfoError, InfoResult, inspect_path


class InfoApp(tk.Tk):
    def __init__(self) -> None:
        super().__init__()
        self.title("TGX Info")
        self.geometry("1080x760")
        self.minsize(900, 600)

        self.result: InfoResult | None = None
        self.preview_photo: ImageTk.PhotoImage | None = None
        self.preview_zoom = 1.0
        self.preview_fit = True

        self.path_var = tk.StringVar()
        self.type_var = tk.StringVar(value="No file loaded")

        self._build_ui()

    def _build_ui(self) -> None:
        top = ttk.Frame(self, padding=10)
        top.pack(fill="x")
        ttk.Label(top, text="TGX file").grid(row=0, column=0, sticky="w")
        entry = ttk.Entry(top, textvariable=self.path_var)
        entry.grid(row=0, column=1, sticky="ew", padx=8)
        ttk.Button(top, text="Browse...", command=self.open_file).grid(row=0, column=2)
        top.columnconfigure(1, weight=1)

        status = ttk.Frame(self, padding=(10, 0, 10, 6))
        status.pack(fill="x")
        ttk.Label(status, textvariable=self.type_var, font=("", 10, "bold")).pack(side="left")
        self.view_button = ttk.Button(status, text="Preview mesh", command=self.preview_mesh, state="disabled")
        self.view_button.pack(side="right")
        self.save_button = ttk.Button(status, text="Save preview PNG", command=self.save_preview, state="disabled")
        self.save_button.pack(side="right", padx=(0, 8))

        panes = ttk.PanedWindow(self, orient="horizontal")
        panes.pack(fill="both", expand=True, padx=10, pady=(0, 10))

        left = ttk.Frame(panes)
        right = ttk.Frame(panes)
        panes.add(left, weight=2)
        panes.add(right, weight=3)

        ttk.Label(left, text="Report").pack(anchor="w")
        text_frame = ttk.Frame(left)
        text_frame.pack(fill="both", expand=True)
        self.report = tk.Text(text_frame, wrap="word", height=20, font=("Consolas", 10))
        yscroll = ttk.Scrollbar(text_frame, orient="vertical", command=self.report.yview)
        self.report.configure(yscrollcommand=yscroll.set)
        self.report.pack(side="left", fill="both", expand=True)
        yscroll.pack(side="right", fill="y")

        preview_bar = ttk.Frame(right)
        preview_bar.pack(fill="x")
        ttk.Label(preview_bar, text="Preview").pack(side="left")
        self.zoom_label = ttk.Label(preview_bar, text="")
        self.zoom_label.pack(side="right")
        self.zoom_in_button = ttk.Button(preview_bar, text="+", width=3, command=lambda: self.set_zoom(self.preview_zoom * 1.25), state="disabled")
        self.zoom_in_button.pack(side="right", padx=(2, 0))
        self.zoom_out_button = ttk.Button(preview_bar, text="-", width=3, command=lambda: self.set_zoom(self.preview_zoom / 1.25), state="disabled")
        self.zoom_out_button.pack(side="right", padx=(2, 0))
        self.zoom_100_button = ttk.Button(preview_bar, text="100%", width=6, command=lambda: self.set_zoom(1.0), state="disabled")
        self.zoom_100_button.pack(side="right", padx=(8, 0))
        self.zoom_fit_button = ttk.Button(preview_bar, text="Fit", width=5, command=self.fit_preview, state="disabled")
        self.zoom_fit_button.pack(side="right", padx=(2, 0))

        canvas_frame = ttk.Frame(right)
        canvas_frame.pack(fill="both", expand=True)
        self.preview_canvas = tk.Canvas(canvas_frame, background="#20242c", highlightthickness=0)
        xscroll = ttk.Scrollbar(canvas_frame, orient="horizontal", command=self.preview_canvas.xview)
        yscroll = ttk.Scrollbar(canvas_frame, orient="vertical", command=self.preview_canvas.yview)
        self.preview_canvas.configure(xscrollcommand=xscroll.set, yscrollcommand=yscroll.set)
        self.preview_canvas.grid(row=0, column=0, sticky="nsew")
        yscroll.grid(row=0, column=1, sticky="ns")
        xscroll.grid(row=1, column=0, sticky="ew")
        canvas_frame.columnconfigure(0, weight=1)
        canvas_frame.rowconfigure(0, weight=1)
        self.preview_canvas.bind("<Configure>", lambda _event: self.refresh_preview())
        self.preview_canvas.bind("<MouseWheel>", self.on_mousewheel_zoom)
        self.preview_canvas.bind("<Button-4>", lambda event: self.on_mousewheel_zoom(event, 1))
        self.preview_canvas.bind("<Button-5>", lambda event: self.on_mousewheel_zoom(event, -1))

    def open_file(self) -> None:
        filename = filedialog.askopenfilename(
            title="Open TGX generated file",
            filetypes=[
                ("TGX generated files", "*.h *.hpp *.cpp *.cxx *.cc"),
                ("Headers", "*.h *.hpp"),
                ("C++ source", "*.cpp *.cxx *.cc"),
                ("All files", "*.*"),
            ],
        )
        if filename:
            self.load_path(Path(filename))

    def load_path(self, path: Path) -> None:
        self.path_var.set(str(path))
        self.report.delete("1.0", "end")
        self.report.insert("end", "Inspecting...\n")
        self.type_var.set("Inspecting...")
        self.view_button.configure(state="disabled")
        self.save_button.configure(state="disabled")
        self.set_preview_zoom_controls("disabled")
        self.zoom_label.configure(text="")
        self.preview_canvas.delete("all")
        self.result = None
        self.after(20, lambda: self._load_path_now(path))

    def _load_path_now(self, path: Path) -> None:
        try:
            result = inspect_path(path)
        except InfoError as exc:
            self.report.delete("1.0", "end")
            self.report.insert("end", str(exc))
            self.type_var.set("Could not inspect file")
            messagebox.showerror("TGX Info", str(exc))
            return
        except Exception as exc:
            self.report.delete("1.0", "end")
            self.report.insert("end", f"Unexpected error:\n{exc}")
            self.type_var.set("Could not inspect file")
            messagebox.showerror("TGX Info", f"Unexpected error:\n{exc}")
            return

        self.result = result
        self.report.delete("1.0", "end")
        self.report.insert("end", result.report)
        self.type_var.set(f"{result.kind.upper()} - {result.symbol}")
        self.view_button.configure(state="normal" if result.preview.view_callback else "disabled")
        self.save_button.configure(state="normal" if result.preview.image else "disabled")
        self.preview_zoom = 1.0
        self.preview_fit = True
        self.set_preview_zoom_controls("normal" if result.preview.image else "disabled")
        self.refresh_preview()

    def refresh_preview(self) -> None:
        self.preview_canvas.delete("all")
        if self.result is None:
            return
        image = self.result.preview.image
        if image is None:
            self.preview_canvas.configure(scrollregion=(0, 0, 0, 0))
            self.zoom_label.configure(text="")
            self.preview_canvas.create_text(
                20,
                20,
                anchor="nw",
                fill="#d0d8e8",
                text="Mesh preview opens in a PyVista window.",
                font=("", 12),
            )
            return
        w = max(20, self.preview_canvas.winfo_width())
        h = max(20, self.preview_canvas.winfo_height())
        if self.preview_fit:
            self.preview_zoom = min((w - 20) / image.width, (h - 20) / image.height)
            self.preview_zoom = max(0.01, min(8.0, self.preview_zoom))
        zw = max(1, int(round(image.width * self.preview_zoom)))
        zh = max(1, int(round(image.height * self.preview_zoom)))
        resample = Image.Resampling.NEAREST if self.preview_zoom >= 1.0 else Image.Resampling.LANCZOS
        preview = image.resize((zw, zh), resample)
        self.preview_photo = ImageTk.PhotoImage(preview)
        self.preview_canvas.create_image(0, 0, image=self.preview_photo, anchor="nw")
        self.preview_canvas.configure(scrollregion=(0, 0, zw, zh))
        self.zoom_label.configure(text=f"{self.preview_zoom * 100:.0f}%")

    def set_preview_zoom_controls(self, state: str) -> None:
        self.zoom_fit_button.configure(state=state)
        self.zoom_100_button.configure(state=state)
        self.zoom_out_button.configure(state=state)
        self.zoom_in_button.configure(state=state)

    def fit_preview(self) -> None:
        self.preview_fit = True
        self.refresh_preview()

    def set_zoom(self, zoom: float) -> None:
        self.preview_fit = False
        self.preview_zoom = max(0.05, min(16.0, zoom))
        self.refresh_preview()

    def on_mousewheel_zoom(self, event: tk.Event, direction: int | None = None) -> str:
        if self.result is None or self.result.preview.image is None:
            return "break"
        delta = direction if direction is not None else getattr(event, "delta", 0)
        factor = 1.1 if delta > 0 else 1.0 / 1.1
        self.set_zoom(self.preview_zoom * factor)
        return "break"

    def save_preview(self) -> None:
        if self.result is None or self.result.preview.image is None:
            return
        default = f"{self.result.symbol}_preview.png"
        filename = filedialog.asksaveasfilename(
            title="Save preview PNG",
            defaultextension=".png",
            initialfile=default,
            filetypes=[("PNG image", "*.png"), ("All files", "*.*")],
        )
        if not filename:
            return
        try:
            self.result.preview.image.save(filename)
        except Exception as exc:
            messagebox.showerror("TGX Info", f"Could not save preview:\n{exc}")

    def preview_mesh(self) -> None:
        if self.result is None or self.result.preview.view_callback is None:
            return
        callback = self.result.preview.view_callback
        thread = threading.Thread(target=callback, daemon=True)
        thread.start()


def main() -> int:
    app = InfoApp()
    app.mainloop()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
