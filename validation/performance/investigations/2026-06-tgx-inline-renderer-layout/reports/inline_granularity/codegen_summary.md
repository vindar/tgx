# Codegen Summary

Pico2 symbol extraction used `arm-none-eabi-nm -C -S --size-sort`.

- pico2_baseline: `tmp\hardware_validation\builds\pico2_baseline_bench_pico2_run2\benchmark.ino.elf` size 3780216 bytes
- pico2_vec_policy_default: `tmp\hardware_validation\builds\pico2_vec_policy_default_bench_pico2\benchmark.ino.elf` size 3780228 bytes
- pico2_vec2_empty: `tmp\hardware_validation\builds\pico2_vec2_empty_pico2_bench_pico2\benchmark.ino.elf` size 3777860 bytes
- pico2_vec3_empty: `tmp\hardware_validation\builds\pico2_vec3_empty_pico2_bench_pico2\benchmark.ino.elf` size 3731820 bytes
- pico2_vec4_empty: `tmp\hardware_validation\builds\pico2_vec4_empty_pico2_bench_pico2\benchmark.ino.elf` size 3713644 bytes
- pico2_color_empty: `tmp\hardware_validation\builds\pico2_color_empty_pico2_bench_pico2\benchmark.ino.elf` size 3764032 bytes
- pico2_rgbf_empty: `tmp\hardware_validation\builds\pico2_rgbf_empty_pico2_bench_pico2\benchmark.ino.elf` size 3779112 bytes
- pico2_math_empty: `tmp\hardware_validation\builds\pico2_math_empty_pico2_bench_pico2\benchmark.ino.elf` size 3712416 bytes
- pico2_image_empty: `tmp\hardware_validation\builds\pico2_image_empty_pico2_bench_pico2\benchmark.ino.elf` size 3764116 bytes

See `tmp/inline_granularity/codegen/pico2_hot_symbols.csv` and `pico2_hot_symbol_deltas.csv`.
