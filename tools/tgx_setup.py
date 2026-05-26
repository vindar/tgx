#!/usr/bin/env python
from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
if str(REPO_ROOT) not in sys.path:
    sys.path.insert(0, str(REPO_ROOT))

from _internal.modules.setup.checks import (
    GA_EAX_EXE,
    GA_EAX_SRC,
    LKH_EXE,
    LKH_SRC,
    MIN_PYTHON,
    REQUIREMENTS,
    TOOLS_ROOT,
    check_environment,
    command_output,
    find_cmake,
    find_cpp_compiler,
    format_install_help,
    lkh_help,
    read_setup_config,
    visibility_helper_path,
    write_setup_config,
)


def _yes_no(prompt: str, default: bool = False) -> bool:
    suffix = " [Y/n] " if default else " [y/N] "
    try:
        answer = input(prompt + suffix).strip().lower()
    except EOFError:
        return default
    if not answer:
        return default
    return answer in ("y", "yes", "o", "oui")


def _run(cmd: list[str], *, cwd: Path | None = None) -> None:
    print("+", " ".join(str(c) for c in cmd))
    subprocess.run(cmd, cwd=cwd, check=True)


def _install_python_deps() -> None:
    if sys.version_info < MIN_PYTHON:
        print(
            "This Python is too old for the TGX tools. Use Python {}.{} or newer before installing dependencies.".format(
                MIN_PYTHON[0], MIN_PYTHON[1]
            ),
            file=sys.stderr,
        )
        raise SystemExit(1)
    _run([sys.executable, "-m", "pip", "install", "-r", str(REQUIREMENTS)])


def _print_status(*, require_config: bool) -> int:
    status = check_environment(tool="mesh", require_config=require_config)
    print("TGX setup status")
    print("================")
    print(f"Python          : {status.info.get('python', sys.executable)}")
    print(f"Python version  : {status.info.get('python_version', '')}")
    print(f"CMake           : {status.info.get('cmake', '') or 'missing'}")
    print(f"C++ compiler    : {status.info.get('cpp_compiler', '') or 'missing'}")
    print(f"TGX visibility  : {status.info.get('visibility_helper', '') or 'missing'}")
    print(f"GA-EAX          : {status.info.get('ga_eax', '') or 'missing'}")
    print(f"LKH             : {status.info.get('lkh', '') or 'missing (optional)'}")
    print()
    if status.errors:
        print("Required checks failed:")
        for error in status.errors:
            print(f"  - {error}")
    else:
        print("Required checks: OK")
    if status.warnings:
        print()
        print("Warnings:")
        for warning in status.warnings:
            print(f"  - {warning}")
    help_text = format_install_help(status, tool="mesh")
    if help_text:
        print()
        print(help_text)
    return 0 if status.ok else 1


def _check_python_deps(interactive: bool, assume_yes: bool) -> None:
    status = check_environment(tool="mesh", require_config=False)
    missing_python = bool(status.missing_modules or status.missing_tkinter or any("Python" in e for e in status.errors))
    if not missing_python:
        print("Python dependencies: OK")
        return

    print("Python dependency check failed.")
    for error in status.errors:
        if "Python" in error or "Missing Python" in error or "tkinter" in error:
            print(f"  - {error}")
    print()
    print(format_install_help(status, tool="mesh"))

    pip_installable = bool(status.missing_modules) and not any("Python" in e for e in status.errors)
    if pip_installable and (assume_yes or (interactive and _yes_no("Install missing Python packages now?", default=False))):
        _install_python_deps()
        print()
        print("Python packages were installed. Please rerun this setup script.")
        raise SystemExit(1)

    print()
    print("Fix the Python environment, then rerun this setup script.")
    raise SystemExit(1)


def _check_cpp_tools() -> None:
    cmake = find_cmake()
    compiler = find_cpp_compiler()
    if cmake and compiler:
        print(f"CMake: {cmake}")
        print(f"C++ compiler: {compiler}")
        cmake_version = command_output([cmake, "--version"]).splitlines()
        if cmake_version:
            print(cmake_version[0])
        return

    status = check_environment(tool="mesh", require_config=False)
    print("C++ toolchain check failed.")
    if cmake is None:
        print("  - CMake was not found.")
    if compiler is None:
        print("  - No supported C++ compiler was found.")
    print()
    print(format_install_help(status, tool="mesh"))
    raise SystemExit(1)


def _build_visibility_helper() -> Path:
    print()
    print("Building TGX visibility helper...")
    from tools._internal.modules.mesh_pipeline.visibility import build_tgx_visibility_helper

    helper = build_tgx_visibility_helper()
    print(f"TGX visibility helper: {helper}")
    return helper


def _build_ga_eax() -> Path:
    print()
    print("Building GA-EAX stripifier helper...")
    if not (GA_EAX_SRC / "main.cpp").exists():
        raise RuntimeError(
            "GA-EAX source files are missing from:\n"
            f"  {GA_EAX_SRC}\n"
            "GA-EAX is required by TGX mesh conversion. Restore the TGX external sources and rerun setup."
        )
    from tools._internal.modules.mesh_pipeline.setup_stripifiers import compile_ga_eax

    exe = compile_ga_eax()
    print(f"GA-EAX helper: {exe}")
    return exe


def _build_lkh(interactive: bool, assume_yes: bool, skip_lkh: bool) -> Path | None:
    print()
    print("Checking optional LKH stripifier...")
    if LKH_EXE.exists():
        print(f"LKH helper: {LKH_EXE}")
        return LKH_EXE
    if skip_lkh:
        print("LKH skipped by request.")
        return None

    from tools._internal.modules.mesh_pipeline.setup_stripifiers import find_lkh_source

    has_sources = find_lkh_source(LKH_SRC) is not None
    if not has_sources:
        print(lkh_help())
        if interactive and not assume_yes:
            _yes_no("Press Enter after reading the LKH instructions, or answer no to continue without LKH.", default=True)
        print("Continuing without LKH. You can add it later and rerun setup.")
        return None

    if not (assume_yes or not interactive or _yes_no("Compile optional LKH now?", default=True)):
        print("Continuing without LKH. You can compile it later by rerunning setup.")
        return None

    from tools._internal.modules.mesh_pipeline.setup_stripifiers import compile_lkh

    exe = compile_lkh()
    if exe:
        print(f"LKH helper: {exe}")
    return exe


def run_setup(args: argparse.Namespace) -> int:
    interactive = not args.yes and sys.stdin.isatty()
    if args.install_python_deps:
        _install_python_deps()
        return 0

    _check_python_deps(interactive=interactive, assume_yes=args.yes)
    _check_cpp_tools()
    visibility = _build_visibility_helper()
    ga_eax = _build_ga_eax()
    lkh = _build_lkh(interactive=interactive, assume_yes=args.yes, skip_lkh=args.skip_lkh)

    write_setup_config(
        {
            "visibility_helper": str(visibility),
            "ga_eax": str(ga_eax),
            "lkh": str(lkh) if lkh else None,
        }
    )
    print()
    print(f"Wrote {TOOLS_ROOT / '.tgx_setup.json'}")
    print()
    return _print_status(require_config=True)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Check and build the TGX Python/C++ tool environment.")
    parser.add_argument("--check", action="store_true", help="only print current setup status")
    parser.add_argument("--install-python-deps", action="store_true", help="install Python dependencies from tools/requirements.txt and exit")
    parser.add_argument("-y", "--yes", action="store_true", help="answer yes to setup prompts")
    parser.add_argument("--skip-lkh", action="store_true", help="do not try to build optional LKH")
    args = parser.parse_args(argv)

    if args.check:
        return _print_status(require_config=True)
    return run_setup(args)


if __name__ == "__main__":
    raise SystemExit(main())
