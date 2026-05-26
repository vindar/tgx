# GA-EAX stripifier helper

This directory contains the GA-EAX source used by the TGX mesh conversion
tools to build `tgx_ga_eax_stripifier`.

The code is adapted for TGX stripification: it receives a triangle adjacency
graph and searches for a tour that can be split into a small number of triangle
chains. The helper is built automatically by:

```bash
python tools/tgx_setup.py
```

The generated executable is written to `tools/_internal/bin/` and is not stored
in the repository.

The bundled GA-EAX source is distributed under the Apache License 2.0. See
`LICENSE` in this directory.
