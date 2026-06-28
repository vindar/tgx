param(
    [string[]]$Boards = @("teensy41"),
    [string[]]$Variants = @("default", "uber_empty", "uber_noinline", "select_empty", "raster_empty", "shading_empty", "all_empty"),
    [switch]$Benchmark,
    [switch]$Examples,
    [switch]$CompileOnly,
    [int]$BenchmarkTimeoutSeconds = 190,
    [int]$ExampleTimeoutSeconds = 24,
    [int]$RetryCount = 1
)

$ErrorActionPreference = "Stop"
$root = Resolve-Path (Join-Path $PSScriptRoot "..\..\..\..")
$capture = Join-Path $root "validation\performance\tools\upload_and_capture.ps1"
$outRoot = Join-Path $root "validation\performance\investigations\2026-06-inline-macros"
$summaryDir = Join-Path $outRoot "hardware"
New-Item -ItemType Directory -Force $summaryDir | Out-Null

$variantFlags = @{
    "default"        = ""
    "uber_empty"    = "-DTGX_UBER_SHADER_INLINE="
    "uber_noinline" = "-DTGX_UBER_SHADER_INLINE=__attribute__((noinline))"
    "select_empty"  = "-DTGX_SHADER_SELECT_INLINE="
    "raster_empty"  = "-DTGX_RASTERIZE_TRIANGLE_INLINE="
    "shading_empty" = "-DTGX_RENDERER3D_SHADING_INLINE="
    "all_empty"     = "-DTGX_UBER_SHADER_INLINE= -DTGX_SHADER_SELECT_INLINE= -DTGX_RASTERIZE_TRIANGLE_INLINE= -DTGX_RENDERER3D_SHADING_INLINE="
}

$boardCfg = @{
    "teensy41" = @{ Port = "COM3"; BenchmarkBaud = 9600; BenchmarkGlobals = 3; BenchmarkSubtests = 150 }
    "core2"    = @{ Port = "COM5"; BenchmarkBaud = 9600; BenchmarkGlobals = 2; BenchmarkSubtests = 100 }
    "cores3"   = @{ Port = "COM10"; BenchmarkBaud = 9600; BenchmarkGlobals = 3; BenchmarkSubtests = 150 }
    "pico2"    = @{ Port = "COM21"; BenchmarkBaud = 9600; BenchmarkGlobals = 3; BenchmarkSubtests = 150 }
}

$exampleCfg = @{
    "teensy41" = @(
        @{ Name = "pointlight_textured_meshes"; Sketch = "examples\Teensy4\3D\pointlight_textured_meshes"; Baud = 9600 },
        @{ Name = "spotlight_checkerboard"; Sketch = "examples\Teensy4\3D\spotlight_checkerboard"; Baud = 9600 },
        @{ Name = "spotlights_room"; Sketch = "examples\Teensy4\3D\spotlights_room"; Baud = 9600 }
    )
    "core2" = @(
        @{ Name = "pointlight_textured_meshes"; Sketch = "examples\M5Stack\pointlight_textured_meshes"; Baud = 115200 },
        @{ Name = "spotlight_checkerboard"; Sketch = "examples\M5Stack\spotlight_checkerboard"; Baud = 115200 },
        @{ Name = "borg_cube"; Sketch = "examples\M5Stack\borg_cube"; Baud = 115200 }
    )
    "cores3" = @(
        @{ Name = "pointlight_textured_meshes"; Sketch = "examples\M5Stack\pointlight_textured_meshes"; Baud = 115200 },
        @{ Name = "spotlight_checkerboard"; Sketch = "examples\M5Stack\spotlight_checkerboard"; Baud = 115200 },
        @{ Name = "borg_cube"; Sketch = "examples\M5Stack\borg_cube"; Baud = 115200 }
    )
    "pico2" = @(
        @{ Name = "pointlight_textured_meshes"; Sketch = "examples\Pico_RP2040_RP2350\pointlight_textured_meshes"; Baud = 115200 },
        @{ Name = "spotlight_checkerboard"; Sketch = "examples\Pico_RP2040_RP2350\spotlight_checkerboard"; Baud = 115200 },
        @{ Name = "borg_cube"; Sketch = "examples\Pico_RP2040_RP2350\borg_cube"; Baud = 115200 }
    )
}

function Invoke-Capture {
    param(
        [string]$Board,
        [string]$Label,
        [string]$Sketch,
        [string]$ParseMode,
        [int]$Baud,
        [int]$TimeoutSeconds,
        [string]$ExtraFlags,
        [int]$ExpectedGlobalScores,
        [int]$MinSubtests,
        [int]$MinTelemetryLines
    )

    $args = @(
        "-ExecutionPolicy", "Bypass",
        "-File", $capture,
        "-Board", $Board,
        "-Sketch", (Join-Path $root $Sketch),
        "-Label", $Label,
        "-ParseMode", $ParseMode,
        "-Baud", [string]$Baud,
        "-TimeoutSeconds", [string]$TimeoutSeconds,
        "-RetryCount", [string]$RetryCount
    )
    if ($CompileOnly) { $args += "-CompileOnly" }
    if ($ExtraFlags) { $args += @("-ExtraFlags", $ExtraFlags) }
    if ($ParseMode -eq "benchmark") {
        $args += @(
            "-UseBoardBenchmarkDefine",
            "-ExpectedGlobalScores", [string]$ExpectedGlobalScores,
            "-MinSubtests", [string]$MinSubtests,
            "-KickText", "x"
        )
    } elseif ($ParseMode -eq "example") {
        $args += @(
            "-MinTelemetryLines", [string]$MinTelemetryLines,
            "-QuietPeriodSeconds", "6"
        )
    }

    Write-Host "[matrix] $Board $Label flags='$ExtraFlags'"
    & powershell @args
    return $LASTEXITCODE
}

$rows = New-Object System.Collections.Generic.List[object]
foreach ($board in $Boards) {
    if (-not $boardCfg.ContainsKey($board)) { throw "Unknown board '$board'" }
    foreach ($variant in $Variants) {
        if (-not $variantFlags.ContainsKey($variant)) { throw "Unknown variant '$variant'" }
        $flags = $variantFlags[$variant]
        if ($Benchmark) {
            $label = "inline_${variant}_benchmark_${board}"
            $exitCode = Invoke-Capture -Board $board -Label $label -Sketch "examples\benchmark" `
                -ParseMode "benchmark" -Baud $boardCfg[$board].BenchmarkBaud -TimeoutSeconds $BenchmarkTimeoutSeconds `
                -ExtraFlags $flags -ExpectedGlobalScores $boardCfg[$board].BenchmarkGlobals `
                -MinSubtests $boardCfg[$board].BenchmarkSubtests -MinTelemetryLines 0
            $rows.Add([pscustomobject]@{ board=$board; variant=$variant; kind="benchmark"; example="benchmark"; label=$label; flags=$flags; exit_code=$exitCode }) | Out-Null
        }
        if ($Examples) {
            foreach ($ex in $exampleCfg[$board]) {
                $label = "inline_${variant}_example_$($ex.Name)_${board}"
                $exitCode = Invoke-Capture -Board $board -Label $label -Sketch $ex.Sketch `
                    -ParseMode "example" -Baud $ex.Baud -TimeoutSeconds $ExampleTimeoutSeconds `
                    -ExtraFlags $flags -ExpectedGlobalScores 0 -MinSubtests 0 -MinTelemetryLines 4
                $rows.Add([pscustomobject]@{ board=$board; variant=$variant; kind="example"; example=$ex.Name; label=$label; flags=$flags; exit_code=$exitCode }) | Out-Null
            }
        }
    }
}

$suffix = if ($CompileOnly) { "compile_only" } else { "hardware" }
$path = Join-Path $summaryDir "inline_macro_matrix_${suffix}_$(Get-Date -Format 'yyyyMMdd_HHmmss').csv"
$rows | Export-Csv -NoTypeInformation -Encoding UTF8 -Path $path
Write-Host "[matrix] summary=$path"
