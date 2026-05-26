# Optional LKH source location

LKH is optional and is not bundled with TGX because it has its own license.

To enable it, download LKH 2.x and copy or extract the source tree here so that
one of these layouts exists:

```text
tools/external_lib/LKH/SRC/...
tools/external_lib/LKH/LKH-2.x/SRC/...
```

Then rerun:

```bash
python tools/tgx_setup.py
```
