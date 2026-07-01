# Board Profiles

Each JSON file describes one benchmark target for `run_bench3d.py`.

Profiles define the backend kind (`cpu`, `arduino` or `espidf`), supported
modules, compile/upload settings, serial port, telemetry timeout and board
specific options such as FQBN or ESP-IDF target.

Keep fixed local ports here only for this workstation. If a board is moved or
reconnected, update the profile before running a full batch.
