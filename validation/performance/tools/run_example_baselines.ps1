param(
    [string[]]$Groups = @("teensy41", "core2", "cores3", "pico2"),
    [string]$LabelSuffix = "baseline",
    [int]$TimeoutSeconds = 45,
    [int]$RetryCount = 1
)

$ErrorActionPreference = "Stop"
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$root = Resolve-Path (Join-Path $scriptDir "..\..\..")
$runner = Join-Path $root "tmp\hardware_validation\tools\upload_and_capture.ps1"
$summaryPath = Join-Path $root "tmp\inline_granularity\baseline_example_runs_$LabelSuffix.csv"

$examples = @(
    @{ group="teensy41"; board="teensy41"; port="COM3"; name="characters"; sketch="examples\Teensy4\3D\characters"; baud=9600; regex="\[TGX telemetry\]"; min=5 },
    @{ group="teensy41"; board="teensy41"; port="COM3"; name="test_texture"; sketch="examples\Teensy4\3D\test-texture"; baud=9600; regex="\[TGX telemetry\]"; min=5 },
    @{ group="teensy41"; board="teensy41"; port="COM3"; name="test_shading"; sketch="examples\Teensy4\3D\test-shading"; baud=9600; regex="\[TGX telemetry\]"; min=5 },
    @{ group="teensy41"; board="teensy41"; port="COM3"; name="borg_cube"; sketch="examples\Teensy4\3D\borg_cube"; baud=9600; regex="\[TGX telemetry\]"; min=5 },
    @{ group="teensy41"; board="teensy41"; port="COM3"; name="scream"; sketch="examples\Teensy4\3D\scream"; baud=9600; regex="\[TGX telemetry\]"; min=5 },

    @{ group="core2"; board="core2"; port="COM5"; name="donkeykong"; sketch="examples\M5Stack\donkeykong"; baud=115200; regex="\[TGX telemetry\]"; min=5 },
    @{ group="core2"; board="core2"; port="COM5"; name="borg_cube"; sketch="examples\M5Stack\borg_cube"; baud=115200; regex="fps="; min=5 },
    @{ group="core2"; board="core2"; port="COM5"; name="scream"; sketch="examples\M5Stack\scream"; baud=115200; regex="fps="; min=5 },

    @{ group="cores3"; board="cores3"; port="COM10"; name="donkeykong"; sketch="examples\M5Stack\donkeykong"; baud=115200; regex="\[TGX telemetry\]"; min=5 },
    @{ group="cores3"; board="cores3"; port="COM10"; name="borg_cube"; sketch="examples\M5Stack\borg_cube"; baud=115200; regex="fps="; min=5 },
    @{ group="cores3"; board="cores3"; port="COM10"; name="scream"; sketch="examples\M5Stack\scream"; baud=115200; regex="fps="; min=5 },

    @{ group="pico2"; board="pico2"; port="COM21"; name="bunny_fig"; sketch="examples\Pico_RP2040_RP2350\bunny_fig"; baud=115200; regex="\[TGX telemetry\]"; min=5 },
    @{ group="pico2"; board="pico2"; port="COM21"; name="borg_cube"; sketch="examples\Pico_RP2040_RP2350\borg_cube"; baud=115200; regex="fps="; min=5 },
    @{ group="pico2"; board="pico2"; port="COM21"; name="scream"; sketch="examples\Pico_RP2040_RP2350\scream"; baud=115200; regex="fps="; min=5 }
)

$rows = New-Object System.Collections.Generic.List[object]
foreach ($ex in $examples) {
    if ($Groups -notcontains $ex.group) { continue }
    $label = "baseline_example_$($ex.group)_$($ex.name)_$LabelSuffix"
    Write-Host "=== $($ex.group) / $($ex.name) ==="
    $args = @(
        "-ExecutionPolicy", "Bypass",
        "-File", $runner,
        "-Board", $ex.board,
        "-Port", $ex.port,
        "-Sketch", (Join-Path $root $ex.sketch),
        "-Label", $label,
        "-ParseMode", "example",
        "-Baud", [string]$ex.baud,
        "-LineRegex", $ex.regex,
        "-MinTelemetryLines", [string]$ex.min,
        "-TimeoutSeconds", [string]$TimeoutSeconds,
        "-RetryCount", [string]$RetryCount
    )
    $output = & powershell @args
    $exitCode = $LASTEXITCODE
    $output | Write-Host

    $metadataPath = Join-Path $root "tmp\hardware_validation\parsed\$($ex.board)_$($label).json"
    $status = "NO_METADATA"
    $rowsParsed = ""
    if (Test-Path $metadataPath) {
        try {
            $m = Get-Content $metadataPath -Raw | ConvertFrom-Json
            $status = $m.run_status
            $rowsParsed = $m.parsed_telemetry_rows
        } catch {
            $status = "METADATA_PARSE_FAILED"
        }
    }
    $rows.Add([pscustomobject]@{
        group = $ex.group
        board = $ex.board
        port = $ex.port
        example = $ex.name
        label = $label
        exit_code = $exitCode
        status = $status
        parsed_rows = $rowsParsed
        metadata = $metadataPath
    }) | Out-Null
}

$rows | Export-Csv -NoTypeInformation -Encoding UTF8 -Path $summaryPath
Write-Host "Example baseline summary: $summaryPath"
$rows | Format-Table -AutoSize
