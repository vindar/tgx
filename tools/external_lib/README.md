# TGX external mesh helpers

This directory contains or receives the C/C++ helper sources used by the TGX
mesh conversion tools.

- `GA_EAX/` is bundled with TGX and is compiled by `tools/tgx_setup.py`.
- `LKH/` is optional. If you want LKH support, download LKH 2.x separately and
  copy or extract its source tree into `tools/external_lib/LKH/`, then rerun
  `tools/tgx_setup.py`.

Generated executables are written to `tools/_internal/bin/`. Build files are written to
`tools/_internal/build/`.
