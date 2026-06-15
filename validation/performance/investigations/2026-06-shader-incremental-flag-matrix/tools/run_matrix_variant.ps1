param(
    [Parameter(Mandatory=$true)][string]$Variant,
    [string]$ExtraFlags = "",
    [switch]$BenchmarksOnly,
    [switch]$ExamplesOnly
)

$ErrorActionPreference = "Stop"
$repo = Resolve-Path (Join-Path $PSScriptRoot "..\..\..\..\..")
$tool = Join-Path $repo "validation\performance\tools\upload_and_capture.ps1"

$benchmarkBoards = @("teensy41", "pico2", "core2", "cores3")
$examples = @(
    @{ Board = "teensy41"; Name = "mars";         Sketch = "examples\Teensy4\3D\mars";         Baud = 9600;   Timeout = 80 },
    @{ Board = "teensy41"; Name = "test_shading"; Sketch = "examples\Teensy4\3D\test-shading"; Timeout = 80; Baud = 9600 },
    @{ Board = "teensy41"; Name = "test_texture"; Sketch = "examples\Teensy4\3D\test-texture"; Timeout = 80; Baud = 9600 },
    @{ Board = "teensy41"; Name = "buddha";       Sketch = "examples\Teensy4\3D\buddha";       Timeout = 80; Baud = 9600 },

    @{ Board = "pico2"; Name = "borg_cube"; Sketch = "examples\Pico_RP2040_RP2350\borg_cube"; Timeout = 100; Baud = 115200 },
    @{ Board = "pico2"; Name = "bunny_fig"; Sketch = "examples\Pico_RP2040_RP2350\bunny_fig"; Timeout = 130; Baud = 115200 },
    @{ Board = "pico2"; Name = "scream";    Sketch = "examples\Pico_RP2040_RP2350\scream";    Timeout = 100; Baud = 115200 },

    @{ Board = "core2"; Name = "borg_cube";  Sketch = "examples\M5Stack\borg_cube";  Timeout = 100; Baud = 115200 },
    @{ Board = "core2"; Name = "donkeykong"; Sketch = "examples\M5Stack\donkeykong"; Timeout = 100; Baud = 115200 },
    @{ Board = "core2"; Name = "scream";     Sketch = "examples\M5Stack\scream";     Timeout = 100; Baud = 115200 },

    @{ Board = "cores3"; Name = "borg_cube";  Sketch = "examples\M5Stack\borg_cube";  Timeout = 100; Baud = 115200 },
    @{ Board = "cores3"; Name = "donkeykong"; Sketch = "examples\M5Stack\donkeykong"; Timeout = 100; Baud = 115200 },
    @{ Board = "cores3"; Name = "scream";     Sketch = "examples\M5Stack\scream";     Timeout = 100; Baud = 115200 }
)

Push-Location $repo
try {
    if (-not $ExamplesOnly) {
        foreach ($board in $benchmarkBoards) {
            $label = "${Variant}__benchmark"
            Write-Host "[matrix] benchmark board=$board label=$label"
            $args = @(
                "-ExecutionPolicy", "Bypass", "-File", $tool,
                "-Board", $board,
                "-Sketch", "examples\benchmark",
                "-Label", $label,
                "-ParseMode", "benchmark",
                "-TimeoutSeconds", "420",
                "-KickText", "x",
                "-KickDelayMs", "2500",
                "-UseBoardBenchmarkDefine"
            )
            if ($ExtraFlags) { $args += @("-ExtraFlags", $ExtraFlags) }
            & powershell @args
            if ($LASTEXITCODE -ne 0) { throw "benchmark failed for $board/$label" }
        }
    }

    if (-not $BenchmarksOnly) {
        foreach ($ex in $examples) {
            $label = "${Variant}__$($ex.Name)"
            Write-Host "[matrix] example board=$($ex.Board) example=$($ex.Name) label=$label"
            $args = @(
                "-ExecutionPolicy", "Bypass", "-File", $tool,
                "-Board", $ex.Board,
                "-Sketch", $ex.Sketch,
                "-Label", $label,
                "-ParseMode", "example",
                "-Baud", "$($ex.Baud)",
                "-TimeoutSeconds", "$($ex.Timeout)"
            )
            if ($ExtraFlags) { $args += @("-ExtraFlags", $ExtraFlags) }
            & powershell @args
            if ($LASTEXITCODE -ne 0) { throw "example failed for $($ex.Board)/$label" }
        }
    }
} finally {
    Pop-Location
}
