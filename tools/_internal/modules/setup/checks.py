from __future__ import annotations

import importlib.util
import json
import os
import platform
import shutil
import subprocess
import sys
from dataclasses import dataclass, field
from pathlib import Path


TOOLS_ROOT = Path(__file__).resolve().parents[3]
REPO_ROOT = TOOLS_ROOT.parent
REQUIREMENTS = TOOLS_ROOT / "requirements.txt"
SETUP_CONFIG = TOOLS_ROOT / ".tgx_setup.json"

INTERNAL_ROOT = TOOLS_ROOT / "_internal"
VISIBILITY_CPP = INTERNAL_ROOT / "cpp" / "tgx_visibility"
VISIBILITY_BUILD = INTERNAL_ROOT / "build" / "tgx_visibility"
EXTERNAL_ROOT = TOOLS_ROOT / "external_lib"
EXTERNAL_BIN = INTERNAL_ROOT / "bin"
GA_EAX_SRC = EXTERNAL_ROOT / "GA_EAX"
LKH_SRC = EXTERNAL_ROOT / "LKH"

EXE_SUFFIX = ".exe" if os.name == "nt" else ""
GA_EAX_EXE = EXTERNAL_BIN / f"tgx_ga_eax_stripifier{EXE_SUFFIX}"
LKH_EXE = EXTERNAL_BIN / f"tgx_lkh_stripifier{EXE_SUFFIX}"

MIN_PYTHON = (3, 9)

PYTHON_MODULES = {
    "Pillow": "PIL",
    "fonttools": "fontTools",
    "NumPy": "numpy",
    "SciPy": "scipy",
    "Matplotlib": "matplotlib",
    "scikit-learn": "sklearn",
    "Numba": "numba",
    "Trimesh": "trimesh",
    "MeshIO": "meshio",
    "PyVista": "pyvista",
    "VTK": "vtk",
}
IMAGE_MODULES = {
    "Pillow": "PIL",
}
FONT_MODULES = {
    "Pillow": "PIL",
    "fonttools": "fontTools",
}


@dataclass
class SetupStatus:
    ok: bool = True
    warnings: list[str] = field(default_factory=list)
    errors: list[str] = field(default_factory=list)
    info: dict[str, str] = field(default_factory=dict)
    missing_modules: list[str] = field(default_factory=list)
    missing_tkinter: bool = False
    config_missing: bool = False
    lkh_missing: bool = False

    def add_error(self, message: str) -> None:
        self.ok = False
        self.errors.append(message)

    def add_warning(self, message: str) -> None:
        self.warnings.append(message)


def visibility_helper_path() -> Path:
    return EXTERNAL_BIN / f"tgx_mesh3d2_visibility{EXE_SUFFIX}"


def visibility_helper_is_current(path: Path | None = None) -> bool:
    path = visibility_helper_path() if path is None else path
    if not path.exists():
        return False
    sources = [VISIBILITY_CPP / "CMakeLists.txt", VISIBILITY_CPP / "tgx_mesh3d2_visibility.cpp"]
    return all(path.stat().st_mtime >= src.stat().st_mtime for src in sources if src.exists())


def find_cmake() -> str | None:
    return shutil.which("cmake")


def find_cpp_compiler() -> str | None:
    if os.name == "nt":
        cl = shutil.which("cl")
        if cl:
            return cl
        for candidate in (
            Path(r"C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat"),
            Path(r"C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat"),
            Path(r"C:\Program Files\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat"),
        ):
            if candidate.exists():
                return str(candidate)
    for name in ("c++", "g++", "clang++"):
        found = shutil.which(name)
        if found:
            return found
    return None


def config_exists() -> bool:
    return SETUP_CONFIG.exists()


def read_setup_config() -> dict[str, object]:
    if not SETUP_CONFIG.exists():
        return {}
    try:
        return json.loads(SETUP_CONFIG.read_text(encoding="utf-8"))
    except Exception:
        return {}


def write_setup_config(extra: dict[str, object] | None = None) -> None:
    payload: dict[str, object] = {
        "python": sys.executable,
        "python_version": platform.python_version(),
        "platform": platform.platform(),
        "cmake": find_cmake(),
        "cpp_compiler": find_cpp_compiler(),
        "visibility_helper": str(visibility_helper_path()) if visibility_helper_path().exists() else None,
        "ga_eax": str(GA_EAX_EXE) if GA_EAX_EXE.exists() else None,
        "lkh": str(LKH_EXE) if LKH_EXE.exists() else None,
    }
    if extra:
        payload.update(extra)
    SETUP_CONFIG.write_text(json.dumps(payload, indent=2), encoding="utf-8")


def check_python_modules(modules: dict[str, str]) -> tuple[list[str], bool]:
    missing = [name for name, import_name in modules.items() if importlib.util.find_spec(import_name) is None]
    missing_tk = importlib.util.find_spec("tkinter") is None
    return missing, missing_tk


def check_environment(*, tool: str = "mesh", require_config: bool = True) -> SetupStatus:
    status = SetupStatus()
    status.info["python"] = sys.executable
    status.info["python_version"] = platform.python_version()

    if sys.version_info < MIN_PYTHON:
        status.add_error(
            "Python {}.{} or newer is required; current Python is {}.".format(
                MIN_PYTHON[0], MIN_PYTHON[1], platform.python_version()
            )
        )

    if require_config and not SETUP_CONFIG.exists():
        status.config_missing = True
        status.add_error("TGX tools setup has not been completed in this checkout.")

    modules = PYTHON_MODULES if tool == "mesh" else (FONT_MODULES if tool == "font" else IMAGE_MODULES)
    missing, missing_tk = check_python_modules(modules)
    status.missing_modules = missing
    status.missing_tkinter = missing_tk
    if missing:
        status.add_error("Missing Python module(s): {}.".format(", ".join(missing)))
    if missing_tk:
        status.add_error("Missing tkinter, required by the graphical tools.")

    if tool == "mesh":
        cmake = find_cmake()
        compiler = find_cpp_compiler()
        status.info["cmake"] = cmake or ""
        status.info["cpp_compiler"] = compiler or ""
        if cmake is None:
            status.add_error("CMake was not found in PATH.")
        if compiler is None:
            status.add_error("No supported C++ compiler was found.")

        visibility = visibility_helper_path()
        status.info["visibility_helper"] = str(visibility) if visibility.exists() else ""
        if not visibility_helper_is_current(visibility):
            status.add_error("TGX visibility helper is missing or out of date.")

        status.info["ga_eax"] = str(GA_EAX_EXE) if GA_EAX_EXE.exists() else ""
        if not GA_EAX_EXE.exists():
            status.add_error("GA-EAX stripifier helper is missing.")

        status.info["lkh"] = str(LKH_EXE) if LKH_EXE.exists() else ""
        if not LKH_EXE.exists():
            status.lkh_missing = True
            status.add_warning("Optional LKH stripifier is not installed.")

    return status


def format_install_help(status: SetupStatus, *, tool: str = "mesh") -> str:
    lines: list[str] = []
    python_too_old = any(message.startswith("Python ") and "or newer is required" in message for message in status.errors)
    if python_too_old:
        lines.append("Use a recent Python environment before installing TGX Python dependencies.")
        lines.append("Recommended: Python 3.10 or newer, for example an Anaconda/Miniconda environment.")
        lines.append("")
    if status.config_missing:
        lines.append("Run the setup script from the TGX repository:")
        lines.append(f"  {sys.executable} {TOOLS_ROOT / 'tgx_setup.py'}")
        lines.append("")
    if status.missing_modules and not python_too_old:
        lines.append("Install the Python dependencies in this Python environment:")
        lines.append(f"  {sys.executable} -m pip install -r {REQUIREMENTS}")
        lines.append("")
    if status.missing_tkinter:
        lines.append(_tkinter_help())
        lines.append("")
    if tool == "mesh" and ("CMake was not found in PATH." in status.errors or "No supported C++ compiler was found." in status.errors):
        lines.append(_compiler_help())
        lines.append("")
    if tool == "mesh" and any("visibility helper" in e or "GA-EAX" in e for e in status.errors):
        lines.append("After installing CMake and a C++ compiler, build the TGX mesh helpers:")
        lines.append(f"  {sys.executable} {TOOLS_ROOT / 'tgx_setup.py'}")
        lines.append("")
    if status.lkh_missing:
        lines.append(lkh_help())
    return "\n".join(lines).rstrip()


def format_status(status: SetupStatus, *, tool: str = "mesh") -> str:
    lines: list[str] = []
    if status.errors:
        lines.append("TGX tool setup is incomplete:")
        lines.extend(f"  - {message}" for message in status.errors)
    if status.warnings:
        if lines:
            lines.append("")
        lines.append("Warning(s):")
        lines.extend(f"  - {message}" for message in status.warnings)
    help_text = format_install_help(status, tool=tool)
    if help_text:
        if lines:
            lines.append("")
        lines.append(help_text)
    return "\n".join(lines)


def ensure_tool_environment(tool: str, *, gui: bool = False) -> None:
    status = check_environment(tool=tool, require_config=True)
    if status.ok:
        if status.warnings:
            _emit_warning(format_status(status, tool=tool), gui=gui)
        return
    _emit_error(format_status(status, tool=tool), gui=gui)
    raise SystemExit(2)


def _emit_error(message: str, *, gui: bool) -> None:
    if gui:
        try:
            import tkinter as tk
            from tkinter import messagebox

            root = tk.Tk()
            root.withdraw()
            messagebox.showerror("TGX setup required", message)
            root.destroy()
            return
        except Exception:
            pass
    print(message, file=sys.stderr)


def _emit_warning(message: str, *, gui: bool) -> None:
    if gui:
        try:
            import tkinter as tk
            from tkinter import messagebox

            root = tk.Tk()
            root.withdraw()
            messagebox.showwarning("TGX setup warning", message)
            root.destroy()
            return
        except Exception:
            pass
    print(message, file=sys.stderr)


def lkh_help() -> str:
    return (
        "LKH is optional but recommended. TGX cannot bundle it directly because it has its own license.\n"
        f"Download LKH 2.x from http://webhotel4.ruc.dk/~keld/research/LKH/ and copy/extract the source tree into:\n"
        f"  {LKH_SRC}\n"
        "Then rerun:\n"
        f"  {sys.executable} {TOOLS_ROOT / 'tgx_setup.py'}"
    )


def _tkinter_help() -> str:
    system = platform.system().lower()
    if system == "linux":
        return "Install tkinter from your system packages, for example: sudo apt install python3-tk"
    if system == "darwin":
        return "Install a Python distribution that includes tkinter, or use python.org/Homebrew Python with Tcl/Tk support."
    return "Install or repair Python with Tcl/Tk support enabled."


def _compiler_help() -> str:
    system = platform.system().lower()
    if system == "windows":
        return (
            "Install CMake and Microsoft Visual Studio Build Tools.\n"
            "In the Visual Studio installer, enable 'Desktop development with C++'."
        )
    if system == "darwin":
        return "Install Xcode command line tools and CMake: xcode-select --install && brew install cmake"
    return "Install a C++ compiler and CMake, for example: sudo apt install build-essential cmake"


def command_output(cmd: list[str]) -> str:
    try:
        return subprocess.check_output(cmd, text=True, stderr=subprocess.STDOUT).strip()
    except Exception:
        return ""
