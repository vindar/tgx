from __future__ import annotations

import os
import time
from contextlib import contextmanager
from dataclasses import dataclass
from typing import Iterator


def enabled() -> bool:
    value = os.environ.get("TGX_MESH_PROGRESS", "1").strip().lower()
    return value not in ("0", "false", "no", "off")


def log(message: str) -> None:
    if enabled():
        print(f"[tgxmesh] {message}", flush=True)


@contextmanager
def step(name: str, detail: str = "") -> Iterator[None]:
    if not enabled():
        yield
        return
    suffix = f" ({detail})" if detail else ""
    start = time.perf_counter()
    print(f"[tgxmesh] begin {name}{suffix}", flush=True)
    try:
        yield
    finally:
        elapsed = time.perf_counter() - start
        print(f"[tgxmesh] end   {name}: {elapsed:.2f}s", flush=True)


@dataclass
class Heartbeat:
    label: str
    interval_s: float = 5.0
    start_s: float = 0.0
    next_s: float = 0.0

    def __post_init__(self) -> None:
        now = time.perf_counter()
        self.start_s = now
        self.next_s = now + self.interval_s

    def ping(self, detail: str = "") -> None:
        if not enabled():
            return
        now = time.perf_counter()
        if now < self.next_s:
            return
        elapsed = now - self.start_s
        suffix = f", {detail}" if detail else ""
        print(f"[tgxmesh] {self.label}: {elapsed:.1f}s elapsed{suffix}", flush=True)
        self.next_s = now + self.interval_s
