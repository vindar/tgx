TGX external stripifier helpers
===============================

The Mesh3D conversion tools can use external TSP solvers to create long
triangle strips from strict compatible triangle adjacency components.

Directories:

- `GA_EAX/`: bundled GA-EAX based stripifier source used by TGX.
- `LKH/`: optional local LKH source tree. LKH has its own license; keep or
  replace this directory locally if you want to use the patched LKH helper.
- `bin/`: generated helper executables created by
  `tools/mesh3d2_converter/setup_stripifiers.py`.

Run:

```sh
python tools/mesh3d2_converter/setup_stripifiers.py
```

The converter will always try GA-EAX when the executable is present, and will
also run patched LKH in parallel when available.
