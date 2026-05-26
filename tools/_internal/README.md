# TGX tools internal files

This directory contains implementation details for the public tools in
`tools/`.

Users normally do not need to run or edit anything here. Use the public entry
points instead:

- `tools/tgx_setup.py`
- `tools/tgx_image.py`
- `tools/tgx_font.py`
- `tools/tgx_mesh.py`
- `tools/cli_tools/tgx_image_cli.py`
- `tools/cli_tools/tgx_font_cli.py`
- `tools/cli_tools/tgx_mesh_cli.py`

Generated helper executables are placed in `_internal/bin/` by
`tools/tgx_setup.py`; build files are placed in `_internal/build/`.
