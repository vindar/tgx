# ESP-IDF Project

This directory contains the ESP-IDF project used for boards that are not
handled by Arduino, currently ESP32-P4.

`tgx_benchmark3d/` is generated/compiled by the Python runner using the board
configuration in `tools/config/boards/esp32p4.json`. Build products and
per-module `sdkconfig` files should stay under repository-level `tmp/`, not in
this source directory.
