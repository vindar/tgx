# New Benchmark3D Baseline - 2026-07-01
## Scope
- Dev branch: `feature/point-spot-lights` at `1849ef549cdf4bf457733b40052e18a489757192`.
- Main/release worktree: `main` at `d46bdb8f89e79ac2e571de1cf97525c9d195510f`.
- Boards: Teensy 4.1, Core2/ESP32, CoreS3/ESP32-S3, Pico W/RP2040, Pico2/RP2350, ESP32P4/Waveshare.
- Measurement policy: benchmark modules render to framebuffer only; example telemetry uses existing sketch FPS output.

## Validity Notes
- The first benchmark attempts were invalidated because Arduino could pick the global TGX mirror. Their large raw logs were pruned; they must not be used.
- Valid benchmark runs are the `*_local_*` files; Arduino compile commands explicitly passed `--library <repo-under-test>`.
- The benchmark harness was made main-compatible by replacing direct calls to new fast trig APIs with local benchmark wrappers.
- ESP32P4 main benchmark used a temporary component alias in the main worktree only, because that worktree directory is not named `tgx`.
- The temporary main worktree was removed after the compact CSVs and logs were archived here.

## Benchmark Run Status
- Dev benchmark: {'SUCCESS': 54} across 54 board/module runs.
- Main benchmark: {'SUCCESS': 30, 'FAILED_OR_UNSUPPORTED': 24} across 54 board/module runs.
- Main unsupported modules are expected: `lighting_shading` needs spotlights, `primitives` needs cylinder/cone/truncated cone, and `integrated`/`mixed_stress` use those new APIs.
- Comparable benchmark rows: 396 per-test measurements, grouped into 30 board/module summaries.

## Synthetic Benchmark Highlights
The modular benchmark is useful for localizing codegen-sensitive paths. It is more sensitive than real examples, so these rows are signals to investigate, not automatic release blockers.

| Board | Module | Matched Tests | Median Delta FPS | Min | Max | Regr. Tests | Impr. Tests |
|---|---:|---:|---:|---:|---:|---:|---:|
| core2 | core_raster | 20 | -11.079% | -15.397% | 0.095% | 17 | 0 |
| core2 | mesh | 15 | -2.820% | -17.226% | 2.375% | 9 | 4 |
| core2 | texture_raster | 8 | -0.411% | -10.301% | 0.027% | 3 | 0 |
| core2 | wire_points | 17 | 2.867% | -6.786% | 20.903% | 1 | 14 |
| cores3 | core_raster | 20 | -3.171% | -6.610% | 0.478% | 14 | 0 |
| cores3 | mesh | 15 | 0.800% | -3.423% | 2.975% | 1 | 9 |
| cores3 | wire_points | 17 | 0.142% | -10.771% | 17.263% | 4 | 7 |
| esp32p4 | wire_points | 17 | 0.684% | -5.334% | 12.774% | 2 | 8 |
| pico2 | mesh | 15 | 0.019% | -29.581% | 146.746% | 6 | 5 |
| pico2 | skybox_noz | 6 | -4.694% | -31.096% | -1.715% | 6 | 0 |
| pico2 | wire_points | 17 | -0.554% | -35.344% | 17.939% | 8 | 4 |
| teensy41 | mesh | 15 | 1.021% | -1.212% | 5.758% | 1 | 8 |
| teensy41 | wire_points | 17 | 1.046% | -0.699% | 6.557% | 0 | 10 |

Worst per-test benchmark deltas:
- `pico2/wire_points/wire.lines.batch.fast`: -35.344% FPS.
- `pico2/skybox_noz/skybox_noz.partial_faces`: -31.096% FPS.
- `pico2/mesh/mesh.v2.grid.flat.noz`: -29.581% FPS.
- `pico2/wire_points/wire.quads.batch.thick`: -19.537% FPS.
- `pico2/wire_points/wire.lines.batch.thick`: -17.667% FPS.
- `core2/mesh/mesh.v2.grid.gouraud.small_tri`: -17.226% FPS.
- `core2/mesh/mesh.v2.culling.gouraud`: -16.967% FPS.
- `core2/core_raster/core_raster.far_clip.flat.z`: -15.397% FPS.
- `core2/core_raster/core_raster.screen_clip.discard.flat.z`: -13.261% FPS.
- `core2/core_raster/core_raster.culling.flat.z`: -12.975% FPS.

Best per-test benchmark deltas:
- `pico2/mesh/mesh.v2.offscreen.discard`: +146.746% FPS.
- `core2/wire_points/wire.mesh.v2.aa`: +20.903% FPS.
- `pico2/wire_points/wire.mesh.legacy.fast`: +17.939% FPS.
- `cores3/wire_points/wire.mesh.v2.aa`: +17.263% FPS.
- `core2/wire_points/wire.lines.batch.aa`: +17.089% FPS.
- `core2/wire_points/wire.triangle_strip.aa`: +16.352% FPS.
- `pico2/wire_points/wire.triangle_strip.aa`: +13.245% FPS.
- `esp32p4/wire_points/wire.mesh.v2.aa`: +12.774% FPS.

## Real Example Status
- Dev examples: {'SUCCESS': 32} across 32 runs.
- Main examples: {'SUCCESS': 17, 'SKIPPED_NO_MAIN_EXAMPLES': 1} across 18 rows, including one explicit ESP32P4 skip because main has no source example set for that board.
- Comparable example scene groups: 29. Dev-only new pointlight/spotlight groups: 15.

Most real examples are within the +/-0.7% noise band. Notable rows:

| Board | Example | Scene | Dev FPS | Main FPS | Delta | Verdict |
|---|---|---|---:|---:|---:|---|
| core2 | donkeykong | wireframe | 27.345 | 25.565 | 6.963% | improvement |
| cores3 | donkeykong | wireframe | 29.6 | 28.075 | 5.432% | improvement |
| pico2 | scream | unknown | 30.375 | 31.375 | -3.187% | regression |
| teensy41 | mars | beginning | 189.8 | 191.93 | -1.110% | regression |
| teensy41 | test_shading | bunny_flat | 108.67 | 109.45 | -0.713% | regression |
| teensy41 | test_shading | teapot_gouraud | 171.867 | 173.33 | -0.844% | regression |

Important interpretation:
- `pico2/scream` is the same sketch on dev and main and shows a uniform ~1 FPS drop: 30.375 FPS dev vs 31.375 FPS main (-3.187%). This is the main real-example regression to track.
- `teensy41/mars` is not a strict apples-to-apples sketch comparison because dev contains explicit `drawSphere` flash placement instantiations in `mars.ino`; its small delta should not be read as a pure TGX core regression.
- `teensy41/test_shading` small drops are borderline, just over the chosen 0.7% noise threshold on two scenes.
- `core2` and `cores3` improve on `donkeykong` wireframe by about 5-7%, while gouraud/texture remains in the noise band.

## Files
- `manifests/run_manifest.json`
- `summaries/dev_benchmark_local_results.csv`
- `summaries/main_benchmark_local_results.csv`
- `summaries/benchmark_dev_vs_main_comparison.csv`
- `summaries/benchmark_dev_vs_main_module_comparison.csv`
- `summaries/dev_example_telemetry_summary.csv`
- `summaries/main_example_telemetry_summary.csv`
- `summaries/example_dev_vs_main_comparison.csv`
- `summaries/dev_only_example_summary.csv`
- `summaries/main_benchmark_unsupported_modules.csv`

## Next Watch Points
- Investigate Pico2/RP2350 `scream` before release if RP2350 example performance is a strict target.
- Use the synthetic benchmark module summaries to localize future codegen-sensitive changes, especially Core2 `core_raster` and Pico2 `skybox_noz`/`wire_points`.
- Keep comparing against the `*_local_*` CSVs only.
