#!/usr/bin/env python3
"""Build, run and parse TGX benchmark3d modules."""

from __future__ import annotations

import argparse
import csv
import datetime as _dt
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import time
from typing import Any, Dict, Iterable, List, Tuple


ROOT = Path(__file__).resolve().parents[3]
BENCH_ROOT = ROOT / "validation" / "benchmark3d"
TMP_ROOT = ROOT / "tmp" / "benchmark3d"
CONFIG_ROOT = BENCH_ROOT / "tools" / "config"
CONNECTED_BOARD_IDS = ["teensy41", "cores3", "core2", "esp32p4", "picow", "pico2"]

ARDUINO_CLI = Path(
    r"C:\Users\Vindar\AppData\Local\Programs\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe"
)
ARDUINO_LIBRARIES = Path(r"D:\Programmation\arduino\libraries")
ESP_IDF_PATH = Path(r"C:\Espressif\frameworks\esp-idf-v5.5.4")
ESP_IDF_PYTHON = Path(r"D:\Espressif\python_env\idf5.5_py3.11_env\Scripts\python.exe")
ESP_IDF_TOOLS_PATH = Path(r"D:\Espressif")

if hasattr(sys.stdout, "reconfigure"):
    sys.stdout.reconfigure(errors="replace")
if hasattr(sys.stderr, "reconfigure"):
    sys.stderr.reconfigure(errors="replace")


def now_stamp() -> str:
    return _dt.datetime.now().strftime("%Y%m%d_%H%M%S")


def load_json(path: Path) -> Dict[str, Any]:
    return json.loads(path.read_text(encoding="utf-8"))


def bench_path(value: str | None) -> Path | None:
    if not value:
        return None
    path = Path(value)
    if path.is_absolute():
        return path
    return BENCH_ROOT / path


def load_modules() -> Dict[str, Dict[str, Any]]:
    data = load_json(CONFIG_ROOT / "modules.json")
    modules = data.get("modules", {})
    if not isinstance(modules, dict):
        raise RuntimeError("tools/config/modules.json must contain a 'modules' object")
    return modules


def load_boards() -> Dict[str, Dict[str, Any]]:
    boards: Dict[str, Dict[str, Any]] = {}
    for path in sorted((CONFIG_ROOT / "boards").glob("*.json")):
        board = load_json(path)
        board_id = str(board.get("id", path.stem))
        board["id"] = board_id
        boards[board_id] = board
    return boards


def git_info() -> Dict[str, Any]:
    def git(args: List[str]) -> str:
        proc = subprocess.run(
            ["git", *args],
            cwd=str(ROOT),
            text=True,
            encoding="utf-8",
            errors="replace",
            stdout=subprocess.PIPE,
            stderr=subprocess.DEVNULL,
        )
        return proc.stdout.strip() if proc.returncode == 0 else ""

    status = git(["status", "--short"])
    return {
        "commit": git(["rev-parse", "--short", "HEAD"]),
        "dirty": bool(status),
        "status_short": status,
    }


def run_command(command: List[str], cwd: Path, log_path: Path, timeout_s: int | None = None,
                env: Dict[str, str] | None = None) -> str:
    log_path.parent.mkdir(parents=True, exist_ok=True)
    print("+ " + " ".join(command))
    proc = subprocess.run(
        command,
        cwd=str(cwd),
        text=True,
        encoding="utf-8",
        errors="replace",
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        timeout=timeout_s,
        env=env,
    )
    log_path.write_text(proc.stdout, encoding="utf-8")
    if proc.returncode != 0:
        print(proc.stdout)
        raise RuntimeError(f"command failed with exit code {proc.returncode}: {' '.join(command)}")
    return proc.stdout


def module_define(module: Dict[str, Any]) -> str:
    define = str(module.get("define", "")).strip()
    if not define:
        raise RuntimeError(f"module {module.get('id', '?')} has no compile define")
    return define


def cpu_compile_command(exe: Path, module: Dict[str, Any]) -> List[str]:
    command = [
        "g++",
        "-std=c++17",
        "-O2",
        f"-D{module_define(module)}",
        "-I",
        ".",
        "-I",
        "src",
        "-I",
        r"examples\CPU\buddhaOnCPU",
        r"validation\benchmark3d\cpu\bench3d_cpu_main.cpp",
        r"src\Color.cpp",
        r"src\Renderer3D.cpp",
    ]
    if sys.platform.startswith("win"):
        command.append("-lgdi32")
    command += ["-o", str(exe)]
    return command


def arduino_compile_upload_command(board: Dict[str, Any], module: Dict[str, Any]) -> List[str]:
    sketch = bench_path(str(module.get("sketch", "")))
    if not sketch:
        raise RuntimeError(f"module {module.get('id', '?')} has no Arduino sketch")

    command = [
        str(ARDUINO_CLI),
        "compile",
        "--fqbn",
        str(board["fqbn"]),
        "--libraries",
        str(ARDUINO_LIBRARIES),
        "--libraries",
        str(BENCH_ROOT / "arduino"),
    ]

    build_properties = list(board.get("build_properties", []))
    build_properties.append(f"compiler.cpp.extra_flags=-D{module_define(module)}")
    for prop in build_properties:
        command += ["--build-property", str(prop)]

    command += [
        "--port",
        str(board["upload_port"]),
        "--upload",
        str(sketch),
    ]
    return command


def espidf_env() -> Dict[str, str]:
    env = os.environ.copy()
    env["IDF_PATH"] = str(ESP_IDF_PATH)
    env["IDF_TOOLS_PATH"] = str(ESP_IDF_TOOLS_PATH)
    env["IDF_PYTHON_ENV_PATH"] = str(ESP_IDF_PYTHON.parent.parent)
    tool_paths = [
        ESP_IDF_PYTHON.parent,
        ESP_IDF_TOOLS_PATH / "tools" / "cmake" / "3.30.2" / "bin",
        ESP_IDF_TOOLS_PATH / "tools" / "ninja" / "1.12.1",
        ESP_IDF_TOOLS_PATH / "tools" / "riscv32-esp-elf" / "esp-14.2.0_20260121" / "riscv32-esp-elf" / "bin",
    ]
    path_prefix = os.pathsep.join(str(path) for path in tool_paths if path.exists())
    env["PATH"] = path_prefix + os.pathsep + env.get("PATH", "")
    return env


def espidf_build_flash_command(board: Dict[str, Any], module_id: str) -> List[str]:
    project = bench_path(str(board.get("project", "")))
    if not project:
        raise RuntimeError(f"ESP-IDF board {board.get('id', '?')} has no project path")
    build_dir = TMP_ROOT / "idf_builds" / f"{board['id']}_{module_id}"
    sdkconfig = build_dir / "sdkconfig"
    sdkconfig_defaults = project / "sdkconfig.defaults"
    return [
        str(ESP_IDF_PYTHON),
        str(ESP_IDF_PATH / "tools" / "idf.py"),
        "-B",
        str(build_dir),
        f"-DSDKCONFIG={sdkconfig}",
        f"-DSDKCONFIG_DEFAULTS={sdkconfig_defaults}",
        f"-DTGX_BENCH_MODULE={module_id}",
        "-p",
        str(board["upload_port"]),
        "set-target",
        str(board["target"]),
        "build",
        "flash",
    ]


def parse_build_size(log_text: str) -> Dict[str, Any]:
    size: Dict[str, Any] = {}

    sketch = re.search(
        r"(?:Sketch uses|Le croquis utilise)\s+(\d+)\s+(?:bytes|octets).*?(?:Maximum is|Le maximum est de)\s+(\d+)\s+(?:bytes|octets)",
        log_text,
        re.IGNORECASE | re.DOTALL,
    )
    if sketch:
        size["sketch_bytes"] = int(sketch.group(1))
        size["sketch_max_bytes"] = int(sketch.group(2))

    globals_match = re.search(
        r"(?:Global variables use|Les variables globales utilisent)\s+(\d+)\s+(?:bytes|octets).*?(?:Maximum is|Le maximum est de)\s+(\d+)\s+(?:bytes|octets)",
        log_text,
        re.IGNORECASE | re.DOTALL,
    )
    if globals_match:
        size["global_bytes"] = int(globals_match.group(1))
        size["global_max_bytes"] = int(globals_match.group(2))

    flash = re.search(r"FLASH:\s+code:(\d+),\s+data:(\d+),\s+headers:(\d+)", log_text)
    if flash:
        size["teensy_flash"] = {
            "code": int(flash.group(1)),
            "data": int(flash.group(2)),
            "headers": int(flash.group(3)),
        }

    ram1 = re.search(r"RAM1:\s+variables:(\d+),\s+code:(\d+),\s+padding:(\d+).*?free for local variables:(\d+)", log_text)
    if ram1:
        size["teensy_ram1"] = {
            "variables": int(ram1.group(1)),
            "code": int(ram1.group(2)),
            "padding": int(ram1.group(3)),
            "free": int(ram1.group(4)),
        }

    ram2 = re.search(r"RAM2:\s+variables:(\d+).*?free for malloc/new:(\d+)", log_text)
    if ram2:
        size["teensy_ram2"] = {
            "variables": int(ram2.group(1)),
            "free": int(ram2.group(2)),
        }

    idf_app = re.search(
        r"([A-Za-z0-9_.-]+\.bin) binary size 0x([0-9a-fA-F]+) bytes\..*?0x([0-9a-fA-F]+) bytes .*? free",
        log_text,
    )
    if idf_app:
        size["idf_app"] = {
            "image": idf_app.group(1),
            "bytes": int(idf_app.group(2), 16),
            "free": int(idf_app.group(3), 16),
        }

    idf_bootloader = re.search(r"Bootloader binary size 0x([0-9a-fA-F]+) bytes\. 0x([0-9a-fA-F]+) bytes .*? free", log_text)
    if idf_bootloader:
        size["idf_bootloader"] = {
            "bytes": int(idf_bootloader.group(1), 16),
            "free": int(idf_bootloader.group(2), 16),
        }

    return size


def capture_serial_pyserial(port: str, baud: int, timeout_s: int, log_path: Path,
                            dtr: bool = True, rts: bool = True) -> str:
    try:
        import serial  # type: ignore[import-not-found]
    except ImportError as exc:
        raise RuntimeError(
            "pyserial is required for serial capture. Install it with: "
            r"C:\Users\Vindar\anaconda3\envs\tgxmesh3d2\python.exe -m pip install pyserial"
        ) from exc

    print(f"+ pyserial capture {port} @ {baud} dtr={int(dtr)} rts={int(rts)}")
    log_path.parent.mkdir(parents=True, exist_ok=True)
    lines: List[str] = []
    end_seen = False
    repeat_seen = False

    open_deadline = _dt.datetime.now() + _dt.timedelta(seconds=min(20, max(3, timeout_s // 3)))
    last_open_error: Exception | None = None
    sp = None
    while _dt.datetime.now() < open_deadline:
        try:
            sp = serial.Serial(port=port, baudrate=baud, timeout=0.2, dsrdtr=False, rtscts=False)
            break
        except Exception as exc:  # pyserial raises several platform-specific subclasses here.
            last_open_error = exc
            time.sleep(0.5)
    if sp is None:
        raise RuntimeError(f"could not open serial port {port}: {last_open_error}")

    with sp:
        sp.dtr = dtr
        sp.rts = rts
        deadline = _dt.datetime.now() + _dt.timedelta(seconds=timeout_s)
        while _dt.datetime.now() < deadline:
            raw = sp.readline()
            if not raw:
                continue
            line = raw.decode("utf-8", errors="replace").rstrip("\r\n")
            print(line)
            lines.append(line)
            if line.startswith("TGXNB3D_END"):
                end_seen = True
            elif end_seen and line.startswith("TGXNB3D_REPEAT"):
                repeat_seen = True
                break

    text = "\n".join(lines) + ("\n" if lines else "")
    log_path.write_text(text, encoding="utf-8")
    if not end_seen:
        raise RuntimeError(f"serial capture timed out before TGXNB3D_END on {port}")
    if not repeat_seen:
        raise RuntimeError(f"serial capture timed out before TGXNB3D_REPEAT on {port}")
    return text


def parse_line(line: str) -> Tuple[str, Dict[str, str]] | None:
    line = line.strip()
    if not line.startswith("TGXNB3D_"):
        return None
    parts = line.split(",")
    record = parts[0]
    fields: Dict[str, str] = {}
    for part in parts[1:]:
        if "=" not in part:
            raise RuntimeError(f"malformed telemetry field in line: {line}")
        key, value = part.split("=", 1)
        if key == "":
            raise RuntimeError(f"empty telemetry field name in line: {line}")
        fields[key] = value
    return record, fields


def parse_telemetry(text: str) -> Dict[str, object]:
    begin: Dict[str, str] = {}
    info: Dict[str, str] = {}
    end: Dict[str, str] = {}
    tests: Dict[str, Dict[str, str]] = {}
    results: List[Dict[str, str]] = []

    for line in text.splitlines():
        parsed = parse_line(line)
        if not parsed:
            continue
        record, fields = parsed
        if record == "TGXNB3D_BEGIN":
            begin = fields
        elif record == "TGXNB3D_INFO":
            info = fields
        elif record == "TGXNB3D_TEST_BEGIN":
            tests[fields.get("id", "")] = fields
        elif record == "TGXNB3D_RESULT":
            test_id = fields.get("id", "")
            row = dict(tests.get(test_id, {}))
            row.update(fields)
            results.append(row)
        elif record == "TGXNB3D_END":
            end = fields

    return {"begin": begin, "info": info, "end": end, "results": results}


def scaled_x100(value: str) -> str:
    if value == "":
        return ""
    try:
        return f"{int(value) / 100.0:.2f}"
    except ValueError:
        return value


def write_results(parsed: Dict[str, object], run_dir: Path, board_id: str, module_id: str,
                  board: Dict[str, Any], module: Dict[str, Any], commands: Dict[str, List[str]],
                  build_size: Dict[str, Any]) -> None:
    run_dir.mkdir(parents=True, exist_ok=True)
    (run_dir / "results.json").write_text(json.dumps(parsed, indent=2), encoding="utf-8")

    begin = parsed.get("begin", {})
    info = parsed.get("info", {})
    results = parsed.get("results", [])
    csv_path = run_dir / "results.csv"
    fieldnames = [
        "board_id",
        "board",
        "profile",
        "module",
        "test",
        "status",
        "width",
        "height",
        "shaders",
        "frames",
        "total_us",
        "mean_us",
        "min_us",
        "max_us",
        "stddev_us",
        "fps",
        "checksum",
        "reason",
    ]
    with csv_path.open("w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        writer.writeheader()
        for result in results:  # type: ignore[assignment]
            writer.writerow(
                {
                    "board_id": board_id,
                    "board": begin.get("board", ""),
                    "profile": begin.get("profile", ""),
                    "module": begin.get("module", module_id),
                    "test": result.get("id", ""),
                    "status": result.get("status", ""),
                    "width": begin.get("width", ""),
                    "height": begin.get("height", ""),
                    "shaders": result.get("shaders", ""),
                    "frames": result.get("frames", ""),
                    "total_us": result.get("total_us", ""),
                    "mean_us": scaled_x100(result.get("mean_us_x100", "")),
                    "min_us": result.get("min_us", ""),
                    "max_us": result.get("max_us", ""),
                    "stddev_us": scaled_x100(result.get("stddev_us_x100", "")),
                    "fps": scaled_x100(result.get("fps_x100", "")),
                    "checksum": result.get("checksum", ""),
                    "reason": result.get("reason", ""),
                }
            )

    manifest = {
        "created": _dt.datetime.now().isoformat(timespec="seconds"),
        "root": str(ROOT),
        "git": git_info(),
        "board_id": board_id,
        "module": module_id,
        "board_config": board,
        "module_config": module,
        "commands": commands,
        "build_size": build_size,
        "begin": begin,
        "info": info,
        "end": parsed.get("end", {}),
        "result_count": len(results),  # type: ignore[arg-type]
    }
    (run_dir / "manifest.json").write_text(json.dumps(manifest, indent=2), encoding="utf-8")


def validate_telemetry(parsed: Dict[str, object], module_id: str) -> None:
    begin = parsed.get("begin", {})
    end = parsed.get("end", {})
    results = parsed.get("results", [])
    if not isinstance(begin, dict) or not begin:
        raise RuntimeError("telemetry is missing TGXNB3D_BEGIN")
    if begin.get("module") != module_id:
        raise RuntimeError(f"telemetry module mismatch: expected {module_id}, got {begin.get('module')}")
    if not isinstance(end, dict) or not end:
        raise RuntimeError("telemetry is missing TGXNB3D_END")
    if not isinstance(results, list):
        raise RuntimeError("telemetry results are malformed")

    expected_tests = int(str(begin.get("tests", "0")))
    if len(results) != expected_tests:
        raise RuntimeError(f"telemetry result count mismatch: expected {expected_tests}, parsed {len(results)}")

    seen_ids = set()
    duplicate_ids = set()
    for row in results:
        if not isinstance(row, dict):
            raise RuntimeError("telemetry contains a malformed result row")
        test_id = row.get("id", "")
        if not test_id:
            raise RuntimeError("telemetry contains a result without id")
        if test_id in seen_ids:
            duplicate_ids.add(test_id)
        seen_ids.add(test_id)
    if duplicate_ids:
        raise RuntimeError("telemetry contains duplicate results: " + ", ".join(sorted(duplicate_ids)))

    required_ok_fields = ["frames", "total_us", "mean_us_x100", "min_us", "max_us", "stddev_us_x100", "fps_x100", "checksum"]
    for row in results:
        if not isinstance(row, dict):
            continue
        status = row.get("status", "")
        if status == "OK":
            missing = [field for field in required_ok_fields if field not in row]
            if missing:
                raise RuntimeError(f"telemetry OK result {row.get('id', '')} is missing fields: {', '.join(missing)}")
        elif status == "SKIP":
            if "reason" not in row:
                raise RuntimeError(f"telemetry SKIP result {row.get('id', '')} is missing reason")
        elif status != "FAIL":
            raise RuntimeError(f"telemetry result {row.get('id', '')} has unknown status {status}")

    failed = [row.get("id", "") for row in results if isinstance(row, dict) and row.get("status") == "FAIL"]
    if failed:
        raise RuntimeError("telemetry contains failed tests: " + ", ".join(failed))

    end_tests = int(str(end.get("tests", expected_tests)))
    end_ok = int(str(end.get("ok", "0")))
    end_skip = int(str(end.get("skip", "0")))
    end_fail = int(str(end.get("fail", "0")))
    if (end.get("status") != "OK") or (end_tests != expected_tests) or ((end_ok + end_skip + end_fail) != expected_tests) or (end_fail != 0):
        raise RuntimeError(
            "telemetry end marker is not clean: "
            f"status={end.get('status')} tests={end_tests} ok={end_ok} skip={end_skip} fail={end_fail}"
        )


def run_cpu(board_id: str, board: Dict[str, Any], module_id: str, module: Dict[str, Any],
            run_dir: Path, no_compile: bool) -> Tuple[str, Dict[str, List[str]], Dict[str, Any]]:
    exe_name = str(module.get("cpu_exe", f"bench3d_cpu_{module_id}.exe"))
    exe = TMP_ROOT / exe_name
    exe.parent.mkdir(parents=True, exist_ok=True)
    commands: Dict[str, List[str]] = {}
    build_size: Dict[str, Any] = {}
    if not no_compile:
        compile_command = cpu_compile_command(exe, module)
        commands["compile"] = compile_command
        log = run_command(compile_command, ROOT, run_dir / "compile.log", timeout_s=int(board.get("compile_timeout_s", 120)))
        build_size = parse_build_size(log)
    run_cmd = [str(exe), "--module", module_id]
    commands["run"] = run_cmd
    telemetry = run_command(run_cmd, ROOT, run_dir / "telemetry_raw.txt", timeout_s=int(board.get("run_timeout_s", 120)))
    return telemetry, commands, build_size


def run_arduino(board_id: str, board: Dict[str, Any], module_id: str, module: Dict[str, Any],
                run_dir: Path, no_compile: bool) -> Tuple[str, Dict[str, List[str]], Dict[str, Any]]:
    commands: Dict[str, List[str]] = {}
    build_size: Dict[str, Any] = {}
    if not no_compile:
        compile_command = arduino_compile_upload_command(board, module)
        commands["compile_upload"] = compile_command
        log = run_command(
            compile_command,
            ROOT,
            run_dir / "compile_upload.log",
            timeout_s=int(board.get("compile_timeout_s", 240)),
        )
        build_size = parse_build_size(log)
    capture_cmd = [
        "pyserial",
        str(board["serial_port"]),
        str(board["serial_baud"]),
        f"timeout={board['serial_timeout_s']}",
        f"dtr={int(bool(board.get('serial_dtr', True)))}",
        f"rts={int(bool(board.get('serial_rts', True)))}",
    ]
    commands["serial_capture"] = capture_cmd
    telemetry = capture_serial_pyserial(
        str(board["serial_port"]),
        int(board["serial_baud"]),
        int(board["serial_timeout_s"]),
        run_dir / "telemetry_raw.txt",
        bool(board.get("serial_dtr", True)),
        bool(board.get("serial_rts", True)),
    )
    return telemetry, commands, build_size


def run_espidf(board_id: str, board: Dict[str, Any], module_id: str, module: Dict[str, Any],
               run_dir: Path, no_compile: bool) -> Tuple[str, Dict[str, List[str]], Dict[str, Any]]:
    commands: Dict[str, List[str]] = {}
    build_size: Dict[str, Any] = {}
    project = bench_path(str(board.get("project", "")))
    if not project:
        raise RuntimeError(f"ESP-IDF board {board_id} has no project path")

    env = espidf_env()
    if not no_compile:
        build_flash_command = espidf_build_flash_command(board, module_id)
        commands["build_flash"] = build_flash_command
        log = run_command(
            build_flash_command,
            project,
            run_dir / "build_flash.log",
            timeout_s=int(board.get("compile_timeout_s", 600)),
            env=env,
        )
        build_size = parse_build_size(log)

    capture_cmd = [
        "pyserial",
        str(board["serial_port"]),
        str(board["serial_baud"]),
        f"timeout={board['serial_timeout_s']}",
        f"dtr={int(bool(board.get('serial_dtr', False)))}",
        f"rts={int(bool(board.get('serial_rts', False)))}",
    ]
    commands["serial_capture"] = capture_cmd
    telemetry = capture_serial_pyserial(
        str(board["serial_port"]),
        int(board["serial_baud"]),
        int(board["serial_timeout_s"]),
        run_dir / "telemetry_raw.txt",
        bool(board.get("serial_dtr", False)),
        bool(board.get("serial_rts", False)),
    )
    return telemetry, commands, build_size


def list_configs(boards: Dict[str, Dict[str, Any]], modules: Dict[str, Dict[str, Any]]) -> None:
    print("Boards:")
    for board_id in sorted(boards):
        board = boards[board_id]
        print(f"  {board_id}: kind={board.get('kind')} modules={','.join(board.get('modules', []))}")
    print("Modules:")
    for module_id in sorted(modules):
        module = modules[module_id]
        print(f"  {module_id}: define={module.get('define')} sketch={module.get('sketch', '')}")


def split_csv_arg(value: str) -> List[str]:
    return [item.strip() for item in value.split(",") if item.strip()]


def expand_board_ids(value: str, boards: Dict[str, Dict[str, Any]]) -> List[str]:
    if value == "all-connected":
        missing = [board_id for board_id in CONNECTED_BOARD_IDS if board_id not in boards]
        if missing:
            raise RuntimeError("connected board config is missing: " + ", ".join(missing))
        return list(CONNECTED_BOARD_IDS)
    if value == "all":
        return sorted(boards)

    board_ids = split_csv_arg(value)
    if not board_ids:
        raise RuntimeError("empty board selection")
    for board_id in board_ids:
        if board_id not in boards:
            raise RuntimeError(f"unknown board {board_id}; use --list")
    return board_ids


def expand_module_ids(value: str, board: Dict[str, Any], modules: Dict[str, Dict[str, Any]]) -> List[str]:
    supported_modules = list(board.get("modules", []))
    if value == "all":
        module_ids = supported_modules if supported_modules else sorted(modules)
    else:
        module_ids = split_csv_arg(value)

    if not module_ids:
        raise RuntimeError("empty module selection")
    for module_id in module_ids:
        if module_id not in modules:
            raise RuntimeError(f"unknown module {module_id}; use --list")
        if supported_modules and module_id not in supported_modules:
            raise RuntimeError(f"board {board['id']} is not configured for module {module_id}")
    return module_ids


def run_one(board_id: str, module_id: str, boards: Dict[str, Dict[str, Any]],
            modules: Dict[str, Dict[str, Any]], out_root: Path, no_compile: bool) -> Dict[str, Any]:
    board = boards[board_id]
    module = dict(modules[module_id])
    module["id"] = module_id
    run_dir = out_root / f"{now_stamp()}_{board_id}_{module_id}"
    run_dir.mkdir(parents=True, exist_ok=True)

    try:
        if board["kind"] == "cpu":
            telemetry, commands, build_size = run_cpu(board_id, board, module_id, module, run_dir, no_compile)
        elif board["kind"] == "arduino":
            telemetry, commands, build_size = run_arduino(board_id, board, module_id, module, run_dir, no_compile)
        elif board["kind"] == "espidf":
            telemetry, commands, build_size = run_espidf(board_id, board, module_id, module, run_dir, no_compile)
        else:
            raise RuntimeError(f"unsupported board kind {board['kind']}")

        parsed = parse_telemetry(telemetry)
        validate_telemetry(parsed, module_id)
        write_results(parsed, run_dir, board_id, module_id, board, module, commands, build_size)

        end = parsed.get("end", {})
        status = end.get("status", "UNKNOWN") if isinstance(end, dict) else "UNKNOWN"
        print(f"TGXNB3D_TOOL_DONE,board={board_id},module={module_id},status={status},run_dir={run_dir}")
        return {
            "board": board_id,
            "module": module_id,
            "status": status,
            "run_dir": str(run_dir),
            "results_csv": str(run_dir / "results.csv"),
            "error": "",
        }
    except Exception as exc:
        error = {
            "created": _dt.datetime.now().isoformat(timespec="seconds"),
            "board": board_id,
            "module": module_id,
            "run_dir": str(run_dir),
            "error": str(exc),
        }
        (run_dir / "error.json").write_text(json.dumps(error, indent=2), encoding="utf-8")
        print(f"TGXNB3D_TOOL_DONE,board={board_id},module={module_id},status=ERROR,run_dir={run_dir}")
        print(f"TGXNB3D_TOOL_ERROR,board={board_id},module={module_id},error={exc}")
        return {
            "board": board_id,
            "module": module_id,
            "status": "ERROR",
            "run_dir": str(run_dir),
            "results_csv": "",
            "error": str(exc),
        }


def write_batch_outputs(batch_dir: Path, selections: List[Tuple[str, str]], summaries: List[Dict[str, Any]]) -> None:
    batch_dir.mkdir(parents=True, exist_ok=True)
    manifest = {
        "created": _dt.datetime.now().isoformat(timespec="seconds"),
        "root": str(ROOT),
        "git": git_info(),
        "selections": [{"board": board_id, "module": module_id} for board_id, module_id in selections],
        "runs": summaries,
    }
    (batch_dir / "batch_manifest.json").write_text(json.dumps(manifest, indent=2), encoding="utf-8")

    fieldnames = [
        "run_dir",
        "board_id",
        "board",
        "profile",
        "module",
        "test",
        "status",
        "width",
        "height",
        "shaders",
        "frames",
        "total_us",
        "mean_us",
        "min_us",
        "max_us",
        "stddev_us",
        "fps",
        "checksum",
        "reason",
    ]
    with (batch_dir / "batch_results.csv").open("w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        writer.writeheader()
        for summary in summaries:
            csv_path = summary.get("results_csv", "")
            if csv_path:
                with Path(csv_path).open("r", newline="", encoding="utf-8") as src:
                    reader = csv.DictReader(src)
                    for row in reader:
                        row = dict(row)
                        row["run_dir"] = summary.get("run_dir", "")
                        writer.writerow({field: row.get(field, "") for field in fieldnames})
            else:
                writer.writerow(
                    {
                        "run_dir": summary.get("run_dir", ""),
                        "board_id": summary.get("board", ""),
                        "module": summary.get("module", ""),
                        "status": "ERROR",
                        "reason": summary.get("error", ""),
                    }
                )


def main(argv: Iterable[str]) -> int:
    boards = load_boards()
    modules = load_modules()

    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--board", default="cpu", help="Board id, comma list, 'all', or 'all-connected'.")
    parser.add_argument("--module", default="core_raster", help="Module id, comma list, or 'all'.")
    parser.add_argument("--out", default=str(TMP_ROOT / "runs"))
    parser.add_argument("--no-compile", action="store_true")
    parser.add_argument("--stop-on-error", action="store_true")
    parser.add_argument("--list", action="store_true")
    args = parser.parse_args(list(argv))

    if args.list:
        list_configs(boards, modules)
        return 0

    board_ids = expand_board_ids(args.board, boards)
    selections: List[Tuple[str, str]] = []
    for board_id in board_ids:
        for module_id in expand_module_ids(args.module, boards[board_id], modules):
            selections.append((board_id, module_id))

    out_root = Path(args.out)
    batch_mode = len(selections) > 1
    if batch_mode:
        batch_dir = out_root / f"{now_stamp()}_batch"
        runs_root = batch_dir / "runs"
    else:
        batch_dir = out_root
        runs_root = out_root

    summaries: List[Dict[str, Any]] = []
    for board_id, module_id in selections:
        summary = run_one(board_id, module_id, boards, modules, runs_root, args.no_compile)
        summaries.append(summary)
        if summary.get("status") != "OK" and args.stop_on_error:
            break

    if batch_mode:
        write_batch_outputs(batch_dir, selections, summaries)
        ok_count = sum(1 for summary in summaries if summary.get("status") == "OK")
        print(f"TGXNB3D_BATCH_DONE,status={'OK' if ok_count == len(summaries) else 'ERROR'},ok={ok_count},total={len(summaries)},run_dir={batch_dir}")

    return 0 if all(summary.get("status") == "OK" for summary in summaries) else 1


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
