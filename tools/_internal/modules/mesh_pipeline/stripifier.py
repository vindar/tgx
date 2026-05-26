from __future__ import annotations

from concurrent.futures import FIRST_COMPLETED, ThreadPoolExecutor, wait
from collections import deque
from collections import Counter
import json
import os
import random
import shutil
import subprocess
import tempfile
import threading
import time
from dataclasses import dataclass, field
from pathlib import Path

from .mesh import FaceVertex, Meshlet, ObjMesh
from .meshlets import build_triangle_adjacency
from .progress import Heartbeat, log


TOOL_ROOT = Path(__file__).resolve().parent
TOOLS_ROOT = Path(__file__).resolve().parents[3]
EXTERNAL_BIN = TOOLS_ROOT / "_internal" / "bin"
DEFAULT_GA_EAX_EXE = EXTERNAL_BIN / ("tgx_ga_eax_stripifier.exe" if os.name == "nt" else "tgx_ga_eax_stripifier")
DEFAULT_LKH_EXE = EXTERNAL_BIN / ("tgx_lkh_stripifier.exe" if os.name == "nt" else "tgx_lkh_stripifier")


def _safe_rmtree(path: Path) -> None:
    """Remove a temporary solver directory, tolerating short Windows handle delays."""
    for _ in range(8):
        try:
            shutil.rmtree(path)
            return
        except FileNotFoundError:
            return
        except PermissionError:
            time.sleep(0.15)
    shutil.rmtree(path, ignore_errors=True)


def default_thread_count() -> int:
    """Default worker count for stripification helper solvers."""
    return max(4, os.cpu_count() or 1)


def default_stripify_budget(triangle_count: int) -> float:
    """Default wall-clock solver budget for one strict compatible component."""
    if triangle_count <= 127:
        return 0.3 + triangle_count / 127.0
    if triangle_count <= 512:
        return 15.0
    if triangle_count <= 1024:
        return 30.0
    if triangle_count <= 2048:
        return 45.0
    return 60.0


def _solver_hard_timeout(seconds: float) -> float:
    """Small process grace over the requested solver budget.

    The GA-EAX/LKH helpers are external processes, so they need a little time to
    start, flush their best result, and exit. Keep the grace deliberately small
    for tiny components; otherwise a 0.5s logical budget can turn into several
    seconds per component.
    """
    seconds = max(0.1, float(seconds))
    if seconds <= 2.0:
        return seconds + 0.5
    if seconds <= 10.0:
        return seconds + 1.0
    return seconds + 5.0


def _run_solver_process(
    args: list[str],
    *,
    cwd: Path | None = None,
    env: dict[str, str] | None = None,
    timeout_s: float,
    stop_event: threading.Event | None = None,
) -> bool:
    """Run one external solver and allow early cooperative cancellation."""
    proc = subprocess.Popen(
        args,
        cwd=cwd,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
        text=True,
        env=env,
    )
    deadline = time.perf_counter() + timeout_s
    while True:
        rc = proc.poll()
        if rc is not None:
            if rc != 0:
                raise subprocess.CalledProcessError(rc, args)
            return True

        if stop_event is not None and stop_event.is_set():
            proc.terminate()
            try:
                proc.wait(timeout=0.5)
            except subprocess.TimeoutExpired:
                proc.kill()
                proc.wait()
            return False

        remaining = deadline - time.perf_counter()
        if remaining <= 0.0:
            proc.kill()
            proc.wait()
            raise subprocess.TimeoutExpired(args, timeout_s)

        time.sleep(min(0.05, remaining))


@dataclass
class StripifyOptions:
    max_time_s: float | None = None
    threads: int = field(default_factory=default_thread_count)
    ga_eax_exe: str | Path | None = None
    lkh_exe: str | Path | None = None


@dataclass
class StripifyCandidate:
    source: str
    chains: list[list[int]]
    elapsed_s: float


@dataclass
class StripifyResult:
    chains: list[list[int]]
    candidates: list[StripifyCandidate]

    @property
    def source(self) -> str:
        return self.candidates[0].source if self.candidates else "none"


_SOLVER_STATS: Counter[str] = Counter()


def reset_solver_stats() -> None:
    _SOLVER_STATS.clear()


def solver_stats_snapshot() -> dict[str, int]:
    return dict(_SOLVER_STATS)


def solver_stats_text() -> str:
    if not _SOLVER_STATS:
        return "none"
    return ", ".join(f"{key}={value}" for key, value in sorted(_SOLVER_STATS.items()))


@dataclass
class StripStats:
    meshlets: int
    triangles: int
    simple_chains: int
    lkh_chains: int
    lkh_runs: int

    @property
    def lkh_extra_vertex_refs(self) -> int:
        return 2 * self.lkh_chains

    @property
    def simple_vertices_loaded(self) -> int:
        return self.triangles + 2 * self.simple_chains

    @property
    def lkh_vertices_loaded(self) -> int:
        return self.triangles + 2 * self.lkh_chains


def _triangle_vertices(mesh: ObjMesh, tri_index: int) -> tuple[FaceVertex, FaceVertex, FaceVertex]:
    tri = mesh.triangles[tri_index]
    return tri.corners


def _edge(triangle: tuple[FaceVertex, FaceVertex, FaceVertex], i: int) -> tuple[FaceVertex, FaceVertex]:
    if i == 0:
        return triangle[0], triangle[1]
    if i == 1:
        return triangle[1], triangle[2]
    return triangle[2], triangle[0]


def _is_adjacent(a: tuple[FaceVertex, FaceVertex, FaceVertex], b: tuple[FaceVertex, FaceVertex, FaceVertex]) -> bool:
    a_edges = {_edge(a, 0), _edge(a, 1), _edge(a, 2)}
    b_edges_reversed = {(_edge(b, 0)[1], _edge(b, 0)[0]), (_edge(b, 1)[1], _edge(b, 1)[0]), (_edge(b, 2)[1], _edge(b, 2)[0])}
    return bool(a_edges.intersection(b_edges_reversed))


def _count_chains_in_order(triangles: list[tuple[FaceVertex, FaceVertex, FaceVertex]]) -> int:
    if not triangles:
        return 0
    chains = 1
    for i in range(1, len(triangles)):
        if not _is_adjacent(triangles[i - 1], triangles[i]):
            chains += 1
    return chains


def simple_chain_count(mesh: ObjMesh, meshlet: Meshlet) -> int:
    """Greedy chain count restricted to triangle adjacency inside one meshlet."""
    if not meshlet.triangles:
        return 0

    adjacency_all = build_triangle_adjacency(mesh)
    remaining = set(meshlet.triangles)
    chains = 0
    while remaining:
        chains += 1
        current = remaining.pop()
        while True:
            candidates = sorted(n for n in adjacency_all[current] if n in remaining)
            if not candidates:
                break
            current = candidates[0]
            remaining.remove(current)
    return chains


def _write_lkh_problem(workdir: Path, mesh: ObjMesh, tri_indices: list[int], *, time_limit_s: float | None = None) -> None:
    local_triangles = [_triangle_vertices(mesh, ti) for ti in tri_indices]
    local_count = len(local_triangles)

    with (workdir / "meshlet.par").open("w", newline="\n") as f:
        f.write("PROBLEM_FILE = meshlet.tsp\n")
        f.write("MOVE_TYPE = 5\n")
        f.write("PATCHING_C = 3\n")
        f.write("PATCHING_A = 2\n")
        f.write("RUNS = 4\n")
        if time_limit_s is not None and time_limit_s > 0.0:
            f.write(f"TIME_LIMIT = {float(time_limit_s):.3f}\n")
        f.write("OUTPUT_TOUR_FILE = meshlet_res.txt\n")

    with (workdir / "meshlet.tsp").open("w", newline="\n") as f:
        f.write("NAME: meshlet\n")
        f.write("TYPE: TSP\n")
        f.write(f"DIMENSION: {local_count}\n")
        f.write("EDGE_WEIGHT_TYPE: EXPLICIT\n")
        f.write("EDGE_WEIGHT_FORMAT: LOWER_DIAG_ROW\n")
        f.write("EDGE_WEIGHT_SECTION\n")
        for i in range(local_count):
            for j in range(i + 1):
                cost = 0 if i == j or _is_adjacent(local_triangles[i], local_triangles[j]) else 1
                f.write(f"{cost} ")
            f.write("\n")
        f.write("EOF\n")


def _read_lkh_tour(workdir: Path, tri_indices: list[int]) -> list[int]:
    lines = (workdir / "meshlet_res.txt").read_text().splitlines()
    try:
        start = next(i for i, line in enumerate(lines) if line.strip() == "TOUR_SECTION") + 1
    except StopIteration as exc:
        raise RuntimeError("LKH output tour does not contain TOUR_SECTION") from exc

    order: list[int] = []
    for line in lines[start:]:
        token = line.split()[0] if line.split() else ""
        if token == "-1" or token == "EOF":
            break
        order.append(tri_indices[int(token) - 1])
    return order


def _load_configured_exes() -> tuple[Path | None, Path | None]:
    config_path = EXTERNAL_BIN / "stripifiers.json"
    ga = DEFAULT_GA_EAX_EXE if DEFAULT_GA_EAX_EXE.exists() else None
    lkh = DEFAULT_LKH_EXE if Path(DEFAULT_LKH_EXE).exists() else None
    if config_path.exists():
        try:
            config = json.loads(config_path.read_text(encoding="utf-8"))
            if config.get("ga_eax") and Path(config["ga_eax"]).exists():
                ga = Path(config["ga_eax"])
            if config.get("lkh") and Path(config["lkh"]).exists():
                lkh = Path(config["lkh"])
        except Exception:
            pass
    return ga, lkh


def _component_adjacency(mesh: ObjMesh, tri_indices: list[int]) -> list[set[int]]:
    edge_owner: dict[tuple[FaceVertex, FaceVertex], int] = {}
    adjacency = [set() for _ in tri_indices]
    for local_index, tri_index in enumerate(tri_indices):
        tri = mesh.triangles[tri_index].corners
        for edge_index in range(3):
            a, b = _edge(tri, edge_index)
            other = edge_owner.get((b, a))
            if other is not None:
                adjacency[local_index].add(other)
                adjacency[other].add(local_index)
            edge_owner[(a, b)] = local_index
    return adjacency


def _tour_value(adj: list[set[int]], order: list[int]) -> int:
    if not order:
        return 0
    value = sum(b not in adj[a] for a, b in zip(order, order[1:]))
    value += order[0] not in adj[order[-1]]
    return int(value)


def _greedy_chain_order(adj: list[set[int]], rng: random.Random, *, start_mode: str = "low_degree", next_mode: str = "low_degree") -> list[int]:
    n = len(adj)
    unvisited = set(range(n))
    chains: list[list[int]] = []
    vertices = list(range(n))

    def free_degree(v: int) -> int:
        return sum(u in unvisited for u in adj[v])

    def start_score(v: int) -> tuple[int, float]:
        degree = free_degree(v)
        if start_mode == "high_degree":
            degree = -degree
        elif start_mode == "random":
            degree = 0
        return degree, rng.random()

    def choose_start() -> int:
        if len(unvisited) < 4096:
            return min(unvisited, key=start_score)
        sample: list[int] = []
        while len(sample) < 64:
            v = vertices[rng.randrange(n)]
            if v in unvisited:
                sample.append(v)
        return min(sample, key=start_score)

    while unvisited:
        start = choose_start()
        unvisited.remove(start)
        chain: deque[int] = deque([start])
        extended = True
        while extended:
            extended = False
            for at_front in (False, True):
                end = chain[0] if at_front else chain[-1]
                candidates = [v for v in adj[end] if v in unvisited]
                if not candidates:
                    continue

                def next_score(v: int) -> tuple[int, float]:
                    degree = free_degree(v)
                    if next_mode == "high_degree":
                        degree = -degree
                    elif next_mode == "random":
                        degree = 0
                    return degree, rng.random()

                nxt = min(candidates, key=next_score)
                unvisited.remove(nxt)
                if at_front:
                    chain.appendleft(nxt)
                else:
                    chain.append(nxt)
                extended = True
        chains.append(list(chain))

    chains.sort(key=lambda c: (-len(c), rng.random()))
    order: list[int] = []
    for chain in chains:
        if rng.random() < 0.5:
            chain = list(reversed(chain))
        order.extend(chain)
    return order


def _component_walk_seed(adj: list[set[int]], rng: random.Random) -> list[int]:
    seen = [False] * len(adj)
    starts = list(range(len(adj)))
    rng.shuffle(starts)
    order: list[int] = []
    for start in starts:
        if seen[start]:
            continue
        stack = [start]
        seen[start] = True
        while stack:
            v = stack.pop()
            order.append(v)
            candidates = [u for u in adj[v] if not seen[u]]
            rng.shuffle(candidates)
            for u in candidates:
                seen[u] = True
                stack.append(u)
    return order


def _random_path_cover_seed(adj: list[set[int]], rng: random.Random) -> list[int]:
    n = len(adj)
    unvisited = set(range(n))
    chains: list[list[int]] = []
    vertices = list(range(n))
    while unvisited:
        start = rng.choice(tuple(unvisited)) if len(unvisited) < 2048 else vertices[rng.randrange(n)]
        while start not in unvisited:
            start = vertices[rng.randrange(n)]
        unvisited.remove(start)
        chain: deque[int] = deque([start])
        while True:
            side = rng.randrange(2)
            end = chain[0] if side == 0 else chain[-1]
            candidates = [v for v in adj[end] if v in unvisited]
            if not candidates:
                other = chain[-1] if side == 0 else chain[0]
                candidates = [v for v in adj[other] if v in unvisited]
                if not candidates:
                    break
                side = 1 - side
            nxt = rng.choice(candidates)
            unvisited.remove(nxt)
            if side == 0:
                chain.appendleft(nxt)
            else:
                chain.append(nxt)
        chains.append(list(chain))
    rng.shuffle(chains)
    order: list[int] = []
    for chain in chains:
        if rng.random() < 0.5:
            chain.reverse()
        order.extend(chain)
    return order


def _make_seed_tours(adj: list[set[int]], count: int, seed: int) -> list[tuple[int, list[int]]]:
    rng = random.Random(seed)
    modes = [
        ("low_degree", "low_degree"),
        ("low_degree", "high_degree"),
        ("high_degree", "low_degree"),
        ("random", "low_degree"),
        ("random", "random"),
    ]
    tours: list[tuple[int, list[int]]] = []
    for i in range(count):
        kind = i % 8
        if kind == 6:
            order = _component_walk_seed(adj, rng)
        elif kind == 7:
            order = _random_path_cover_seed(adj, rng)
        else:
            start_mode, next_mode = modes[kind % len(modes)]
            order = _greedy_chain_order(adj, rng, start_mode=start_mode, next_mode=next_mode)
        tours.append((_tour_value(adj, order), order))
    tours.sort(key=lambda item: item[0])
    return tours


def _split_order_into_chains(adj: list[set[int]], order: list[int]) -> list[list[int]]:
    if not order:
        return []
    chains: list[list[int]] = []
    current = [order[0]]
    for a, b in zip(order, order[1:]):
        if b in adj[a]:
            current.append(b)
        else:
            chains.append(current)
            current = [b]
    if order[0] in adj[order[-1]] and chains:
        chains[0] = current + chains[0]
    else:
        chains.append(current)
    return chains


def _write_graph(path: Path, adj: list[set[int]]) -> None:
    with path.open("w", encoding="ascii", newline="\n") as f:
        f.write(f"{len(adj)}\n")
        for row in adj:
            values = sorted(row)
            f.write(str(len(values)))
            for value in values:
                f.write(f" {value}")
            f.write("\n")


def _write_seed_population(path: Path, adj: list[set[int]], population: int, seed: int) -> None:
    tours = _make_seed_tours(adj, population, seed)
    with path.open("w", encoding="ascii", newline="\n") as f:
        for value, order in tours[:population]:
            f.write(f"{len(order)} {value}\n")
            f.write(" ".join(str(v + 1) for v in order))
            f.write("\n")


def _read_ga_best(path: Path) -> list[int]:
    with path.open("r", encoding="ascii") as f:
        header = f.readline().split()
        if len(header) != 2:
            raise RuntimeError("invalid GA-EAX output header")
        order = [int(x) - 1 for x in f.read().split()]
    return order


def _run_ga_eax(adj: list[set[int]], graph_path: Path, exe: Path, work: Path, run_id: int, seconds: float, seed: int, large_budget: bool, stop_event: threading.Event | None = None) -> StripifyCandidate | None:
    n = len(adj)
    if n > 5000:
        if large_budget and seconds >= 90:
            population = 80
            kids = 24
            generations = 500
        elif large_budget and seconds >= 45:
            population = 40
            kids = 12
            generations = 240
        elif large_budget:
            population = 24
            kids = 8
            generations = 120
        else:
            population = 8
            kids = 4
            generations = 16
    else:
        population = 24 if n < 127 else 40
        kids = 8 if n < 127 else 12
        generations = 30 if n < 127 else 80
    run_dir = work / f"ga_{run_id:02d}"
    run_dir.mkdir(parents=True, exist_ok=True)
    prefix = run_dir / "result"
    start = time.perf_counter()
    try:
        completed = _run_solver_process(
            [str(exe), str(graph_path), str(prefix), str(population), str(kids), str(generations), str(seconds), "-", str(seed)],
            timeout_s=_solver_hard_timeout(seconds),
            stop_event=stop_event,
        )
        if not completed:
            return None
        order = _read_ga_best(Path(str(prefix) + "_BestSol"))
    except Exception:
        return None
    chains = _split_order_into_chains(adj, order)
    return StripifyCandidate(f"ga-eax#{run_id}", chains, time.perf_counter() - start)


def _bridge_candidates(adj: list[set[int]], node: int, count: int, rng: random.Random) -> list[int]:
    n = len(adj)
    if count <= 0 or n <= len(adj[node]) + 1:
        return []
    forbidden = set(adj[node])
    forbidden.add(node)
    extra: list[int] = []
    step = max(1, n // (count * 2 + 1))
    for k in range(1, count * 2 + 4):
        for candidate in ((node + k * step) % n, (node - k * step) % n):
            if candidate not in forbidden and candidate not in extra:
                extra.append(candidate)
                if len(extra) >= count:
                    return extra
    tries = 0
    while len(extra) < count and tries < max(64, count * 32):
        tries += 1
        candidate = rng.randrange(n)
        if candidate not in forbidden and candidate not in extra:
            extra.append(candidate)
    return extra


def _write_sparse_lkh_problem(work: Path, graph_path: Path, adj: list[set[int]], *, time_limit: float, bridge_candidates: int, seed: int, subgradient: bool) -> None:
    n = len(adj)
    rng = random.Random(seed)
    edges = sorted({(min(i, j), max(i, j)) for i, row in enumerate(adj) for j in row if i != j})
    with (work / "graph.tsp").open("w", encoding="ascii", newline="\n") as f:
        f.write("NAME: tgx_graph\nTYPE: TSP\n")
        f.write(f"DIMENSION: {n}\n")
        f.write("EDGE_WEIGHT_TYPE: SPECIAL\n")
        f.write("NODE_COORD_TYPE: TWOD_COORDS\n")
        f.write("NODE_COORD_SECTION\n")
        for i in range(n):
            f.write(f"{i + 1} {i} 0\n")
        f.write("EOF\n")
    with (work / "graph.edge").open("w", encoding="ascii", newline="\n") as f:
        f.write(f"{n} {len(edges)}\n")
        for a, b in edges:
            f.write(f"{a} {b} 0\n")
    with (work / "graph.cand").open("w", encoding="ascii", newline="\n") as f:
        f.write(f"{n}\n")
        for i, row in enumerate(adj):
            candidates: list[tuple[int, int]] = [(j, 0) for j in sorted(row)]
            candidates.extend((j, 1) for j in _bridge_candidates(adj, i, bridge_candidates, rng))
            f.write(f"{i + 1} 0 {len(candidates)}")
            for j, alpha in candidates:
                f.write(f" {j + 1} {alpha}")
            f.write("\n")
        f.write("-1\n")
    with (work / "graph.par").open("w", encoding="ascii", newline="\n") as f:
        f.write("PROBLEM_FILE = graph.tsp\n")
        f.write("MOVE_TYPE = 5\nPATCHING_C = 3\nPATCHING_A = 2\nRUNS = 1\n")
        f.write("MAX_CANDIDATES = 0\nTRACE_LEVEL = 0\n")
        if not subgradient:
            f.write("SUBGRADIENT = NO\nINITIAL_PERIOD = 0\n")
        f.write("CANDIDATE_FILE = graph.cand\nEDGE_FILE = graph.edge\n")
        f.write(f"TIME_LIMIT = {max(0.1, time_limit):.3f}\n")
        f.write("OUTPUT_TOUR_FILE = graph.tour\n")


def _read_local_lkh_tour(path: Path) -> list[int]:
    lines = path.read_text().splitlines()
    start = lines.index("TOUR_SECTION") + 1
    order: list[int] = []
    for line in lines[start:]:
        token = line.strip()
        if token in ("-1", "EOF"):
            break
        if token:
            order.append(int(token) - 1)
    return order


def _run_lkh_sparse(adj: list[set[int]], graph_path: Path, exe: Path, work: Path, max_time_s: float, seed: int) -> list[StripifyCandidate]:
    n = len(adj)
    if n < 127:
        variants = [0]
        per_variant = min(2.0, max_time_s)
        subgradient = False
    elif n > 5000:
        variants = [0]
        per_variant = max_time_s
        subgradient = False
    else:
        variants = [0, 2, 4]
        per_variant = max(1.0, max_time_s / len(variants))
        subgradient = True

    out: list[StripifyCandidate] = []
    for index, bridge_count in enumerate(variants):
        run_dir = work / f"lkh_br{bridge_count}"
        run_dir.mkdir(parents=True, exist_ok=True)
        start = time.perf_counter()
        try:
            _write_sparse_lkh_problem(run_dir, graph_path, adj, time_limit=per_variant, bridge_candidates=bridge_count, seed=seed + index * 1009, subgradient=subgradient)
            env = os.environ.copy()
            env["TGX_LKH_GRAPH"] = str(graph_path)
            subprocess.run(
                [str(exe), "graph.par"],
                cwd=run_dir,
                stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL,
                text=True,
                check=True,
                timeout=_solver_hard_timeout(per_variant),
                env=env,
            )
            order = _read_local_lkh_tour(run_dir / "graph.tour")
        except Exception:
            continue
        out.append(StripifyCandidate(f"lkh/br{bridge_count}", _split_order_into_chains(adj, order), time.perf_counter() - start))
    return out


def _lkh_plan(n: int, max_time_s: float) -> tuple[list[int], float, bool]:
    if n <= 127:
        return [0], max_time_s, False
    if n > 5000:
        return [0, 2, 4], max_time_s, False
    return [0, 2, 4], max(1.0, max_time_s), True


def _run_lkh_sparse_variant(
    adj: list[set[int]],
    graph_path: Path,
    exe: Path,
    work: Path,
    bridge_count: int,
    time_limit: float,
    seed: int,
    subgradient: bool,
    stop_event: threading.Event | None = None,
) -> StripifyCandidate | None:
    run_dir = work / f"lkh_br{bridge_count}"
    run_dir.mkdir(parents=True, exist_ok=True)
    start = time.perf_counter()
    try:
        _write_sparse_lkh_problem(run_dir, graph_path, adj, time_limit=time_limit, bridge_candidates=bridge_count, seed=seed, subgradient=subgradient)
        env = os.environ.copy()
        env["TGX_LKH_GRAPH"] = str(graph_path)
        completed = _run_solver_process(
            [str(exe), "graph.par"],
            cwd=run_dir,
            env=env,
            timeout_s=_solver_hard_timeout(time_limit),
            stop_event=stop_event,
        )
        if not completed:
            return None
        order = _read_local_lkh_tour(run_dir / "graph.tour")
    except Exception:
        return None
    return StripifyCandidate(f"lkh/br{bridge_count}", _split_order_into_chains(adj, order), time.perf_counter() - start)


def _best_candidate(candidates: list[StripifyCandidate]) -> StripifyCandidate:
    return min(candidates, key=lambda c: (len(c.chains), sum(len(chain) <= 2 for chain in c.chains), c.elapsed_s))


def _small_exact_chains(mesh: ObjMesh, tri_indices: list[int]) -> list[list[int]] | None:
    """Return exact valid chains for tiny components, or None when solvers are useful."""
    n = len(tri_indices)
    if n <= 1:
        return [tri_indices] if tri_indices else []

    adj = _component_adjacency(mesh, tri_indices)
    if n == 2:
        return [tri_indices] if 1 in adj[0] else [[tri_indices[0]], [tri_indices[1]]]

    if n == 3:
        # Any connected three-triangle component has a Hamiltonian path. Testing
        # all local orders is clearer and safer than trying to identify the
        # center triangle explicitly.
        orders = (
            (0, 1, 2),
            (0, 2, 1),
            (1, 0, 2),
            (1, 2, 0),
            (2, 0, 1),
            (2, 1, 0),
        )
        for order in orders:
            if order[1] in adj[order[0]] and order[2] in adj[order[1]]:
                return [[tri_indices[i] for i in order]]

        # Defensive fallback for an invalid/non-connected caller: keep the
        # returned chains valid instead of pretending one strip exists.
        best = min((_split_order_into_chains(adj, list(order)) for order in orders), key=lambda chains: (len(chains), -max(len(c) for c in chains)))
        return [[tri_indices[i] for i in chain] for chain in best]

    return None


def stripify_component(mesh: ObjMesh, tri_indices: list[int], options: StripifyOptions | None = None) -> StripifyResult:
    """Return triangle chains for one strict compatible connected component.

    The function always seeds a greedy baseline, tries GA-EAX when available, and
    runs patched sparse LKH in parallel when it is available. `tri_indices` may be
    any component, but quality is meaningful when it is one connected component
    under the strong `(vertex, texcoord, normal)` edge compatibility relation.
    """
    opt = options or StripifyOptions()
    tri_indices = list(tri_indices)
    if len(tri_indices) <= 3:
        chains = _small_exact_chains(mesh, tri_indices)
        assert chains is not None
        return StripifyResult(chains, [StripifyCandidate("trivial", chains, 0.0)])

    component_size = len(tri_indices)
    should_report = component_size >= 128
    if should_report:
        log(f"stripify component: {component_size} triangles")

    adj = _component_adjacency(mesh, tri_indices)
    greedy_seed_count = 1 if len(adj) > 5000 else (4 if len(adj) <= 32 else 8)
    greedy_start = time.perf_counter()
    greedy_order = _make_seed_tours(adj, greedy_seed_count, 1234)[0][1]
    greedy_chains = _split_order_into_chains(adj, greedy_order)
    greedy_candidate = StripifyCandidate("greedy", greedy_chains, time.perf_counter() - greedy_start)
    if len(greedy_chains) == 1:
        _SOLVER_STATS["greedy"] += 1
        if should_report:
            log("stripify best: greedy, 1 chains")
        return StripifyResult([[tri_indices[i] for i in chain] for chain in greedy_chains], [greedy_candidate])

    configured_ga, configured_lkh = _load_configured_exes()
    ga_exe = Path(opt.ga_eax_exe) if opt.ga_eax_exe else configured_ga
    lkh_exe = Path(opt.lkh_exe) if opt.lkh_exe else configured_lkh
    if ga_exe is not None and not ga_exe.exists():
        ga_exe = None
    if lkh_exe is not None and not lkh_exe.exists():
        lkh_exe = None

    candidates: list[StripifyCandidate] = []
    thread_count = max(1, int(opt.threads))
    budget = opt.max_time_s if opt.max_time_s is not None else default_stripify_budget(component_size)
    max_time_s = max(0.1, float(budget))
    work = Path(tempfile.mkdtemp(prefix="tgx_stripify_"))
    try:
        graph_path = work / "component.graph"
        _write_graph(graph_path, adj)
        futures = []
        future_labels = {}
        stop_event = threading.Event()
        with ThreadPoolExecutor(max_workers=thread_count) as pool:
            lkh_variants: list[int] = []
            lkh_time_limit = 0.0
            lkh_subgradient = False
            if lkh_exe:
                lkh_variants, lkh_time_limit, lkh_subgradient = _lkh_plan(len(adj), max_time_s)
            lkh_slots = min(len(lkh_variants), max(0, thread_count - 1)) if lkh_variants else 0
            ga_workers = thread_count - lkh_slots
            if len(adj) < 128:
                ga_workers = min(1, max(1, ga_workers))
            elif len(adj) < 1000:
                ga_workers = min(2, max(1, ga_workers))
            elif len(adj) < 5000:
                ga_workers = min(4, max(1, ga_workers))
            else:
                ga_workers = min(2, max(1, ga_workers))
            if ga_exe and ga_workers > 0:
                ga_workers = max(1, ga_workers)
                ga_seconds = max_time_s
                for run_id in range(ga_workers):
                    label = f"ga-eax#{run_id}"
                    fut = pool.submit(_run_ga_eax, adj, graph_path, ga_exe, work, run_id, ga_seconds, 1009 + run_id * 7919, lkh_exe is None, stop_event)
                    futures.append(fut)
                    future_labels[fut] = label
            if lkh_exe:
                for variant_index, bridge_count in enumerate(lkh_variants):
                    label = f"lkh/br{bridge_count}"
                    fut = pool.submit(
                        _run_lkh_sparse_variant,
                        adj,
                        graph_path,
                        lkh_exe,
                        work,
                        bridge_count,
                        lkh_time_limit,
                        424242 + variant_index * 1009,
                        lkh_subgradient,
                        stop_event,
                    )
                    futures.append(fut)
                    future_labels[fut] = label

            if should_report and futures:
                labels = ", ".join(future_labels[f] for f in futures)
                log(f"stripify solvers: {labels}; budget {max_time_s:.1f}s; threads {thread_count}")

            pending = set(futures)
            heartbeat = Heartbeat("stripify waiting")
            deadline = time.perf_counter() + _solver_hard_timeout(max_time_s) if futures else time.perf_counter()
            while pending:
                timeout = max(0.1, min(1.0, deadline - time.perf_counter()))
                if timeout <= 0.0:
                    for fut in pending:
                        fut.cancel()
                    break
                done, pending = wait(pending, timeout=timeout, return_when=FIRST_COMPLETED)
                for fut in done:
                    try:
                        result = fut.result()
                    except Exception as exc:
                        if should_report:
                            log(f"stripify solver {future_labels.get(fut, '?')} failed: {exc}")
                        continue
                    if isinstance(result, StripifyCandidate):
                        candidates.append(result)
                        if should_report:
                            log(f"stripify solver {result.source}: {len(result.chains)} chains in {result.elapsed_s:.2f}s")
                        if len(result.chains) == 1:
                            stop_event.set()
                            for pending_future in pending:
                                pending_future.cancel()
                    elif result:
                        candidates.extend(result)
                        if should_report:
                            for item in result:
                                log(f"stripify solver {item.source}: {len(item.chains)} chains in {item.elapsed_s:.2f}s")
                        if any(len(item.chains) == 1 for item in result):
                            stop_event.set()
                            for pending_future in pending:
                                pending_future.cancel()
                if pending and should_report:
                    labels = ", ".join(sorted(future_labels.get(f, "?") for f in pending))
                    heartbeat.ping(f"pending: {labels}")
    finally:
        _safe_rmtree(work)

    if not candidates:
        if should_report:
            log(f"stripify fallback: greedy ({greedy_seed_count} seed tours)")
        candidates.append(greedy_candidate)
    else:
        candidates.append(greedy_candidate)

    candidates.sort(key=lambda c: (len(c.chains), c.elapsed_s))
    best = _best_candidate(candidates)
    _SOLVER_STATS[best.source.split("#", 1)[0]] += 1
    if should_report:
        log(f"stripify best: {best.source}, {len(best.chains)} chains")
    return StripifyResult([[tri_indices[i] for i in chain] for chain in best.chains], candidates)


def lkh_chain_count(mesh: ObjMesh, meshlet: Meshlet, lkh_exe: str | Path = DEFAULT_LKH_EXE) -> tuple[int, bool]:
    """Return optimized chain count for a meshlet and whether an external solver was used."""
    tri_indices = meshlet.triangles
    if len(tri_indices) <= 1:
        return len(tri_indices), False
    if len(tri_indices) == 2:
        a = _triangle_vertices(mesh, tri_indices[0])
        b = _triangle_vertices(mesh, tri_indices[1])
        return (1 if _is_adjacent(a, b) else 2), False
    result = stripify_component(mesh, tri_indices, StripifyOptions(lkh_exe=lkh_exe))
    used_external = any(c.source.startswith(("ga-eax", "lkh")) for c in result.candidates)
    return len(result.chains), used_external


def strip_stats(mesh: ObjMesh, meshlets: list[Meshlet], lkh_exe: str | Path = DEFAULT_LKH_EXE) -> StripStats:
    simple = 0
    lkh = 0
    runs = 0
    triangles = 0
    for meshlet in meshlets:
        triangles += len(meshlet.triangles)
        simple += simple_chain_count(mesh, meshlet)
        count, did_run = lkh_chain_count(mesh, meshlet, lkh_exe)
        lkh += count
        if did_run:
            runs += 1
    return StripStats(len(meshlets), triangles, simple, lkh, runs)
