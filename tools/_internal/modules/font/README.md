# TGX font converter internals

This module implements the public font tools:

- `tools/tgx_font.py`: graphical converter for interactive use.
- `tools/cli_tools/tgx_font_cli.py`: command-line converter for scripts.

Supported input files are TrueType/OpenType fonts that Pillow can load
(`.ttf`, `.otf`, and many `.ttc` fonts depending on the local Pillow build).

Supported output formats:

- `ili9341-v23`: TGX/ILI9341_t3 packed font, including 2/4/8-bit
  antialiasing.
- `ili9341-v1`: older monochrome ILI9341_t3 format.
- `adafruit-gfx`: monochrome Adafruit `GFXfont` format.

The converter intentionally exports one continuous character range. Unselected
characters inside that range are emitted as empty glyphs, which keeps the
runtime format simple while still allowing sparse selections.

Typical CLI usage:

```bash
python tools/cli_tools/tgx_font_cli.py MyFont.ttf -o my_font.h --name my_font --sizes 12,16,24 --chars printable --aa 4
```

Use `--preview-png preview.png` to generate a desktop preview grid of the
exported characters. When several sizes are exported, the same preview image
contains one section per size.

Generated C++ font symbols always include the pixel size suffix. For the
antialiased ILI9341 v2.3 format, they also include the AA depth for
compatibility with TGX font naming. For example,
`--name ui_font --sizes 16 --aa 4` creates `ui_font_AA4_16`. Monochrome outputs
use names such as `ui_font_16`.
