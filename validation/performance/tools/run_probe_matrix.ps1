param(
    [string[]]$Boards = @("core2", "cores3", "picow", "pico2", "teensy41"),
    [string]$LabelSuffix = "",
    [int]$TimeoutSeconds = 100,
    [int]$RetryCount = 1
)

$ErrorActionPreference = "Stop"
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$root = Resolve-Path (Join-Path $scriptDir "..\..\..")
$runner = Join-Path $scriptDir "upload_and_capture.ps1"
$sketchRoot = Join-Path $root "tmp\hardware_validation\sketches"
$summaryDir = Join-Path $root "tmp\hardware_validation\parsed"
New-Item -ItemType Directory -Force -Path $summaryDir | Out-Null

$boardPorts = @{
    core2 = "COM5"
    cores3 = "COM10"
    picow = "COM28"
    pico2 = "COM21"
    teensy41 = "COM3"
}

$probes = @(
    @{
        name = "serial_alive"
        sketch = "SerialAliveProbe"
        start = "TGX_HW_PROBE_BEGIN"
        end = "TGX_HW_PROBE_END"
        line = "TGX_HW_PROBE_OK"
        minLines = 1
    },
    @{
        name = "macro_probe"
        sketch = "TgxMacroProbe"
        start = "TGX_MACRO_PROBE_BEGIN"
        end = "TGX_MACRO_PROBE_END"
        line = "TGX_MACRO_PROBE_OK"
        minLines = 1
    },
    @{
        name = "telemetry_probe"
        sketch = "TelemetryStreamProbe"
        start = "TGX_TELEMETRY_PROBE_BEGIN"
        end = "TGX_TELEMETRY_PROBE_END"
        line = "SCENE="
        minLines = 9
    }
)

$stamp = Get-Date -Format "yyyyMMdd_HHmmss"
$suffix = if ([string]::IsNullOrWhiteSpace($LabelSuffix)) { $stamp } else { $LabelSuffix }
$summaryPath = Join-Path $summaryDir "probe_matrix_$suffix.csv"
$rows = New-Object System.Collections.Generic.List[object]

foreach ($board in $Boards) {
    if (-not $boardPorts.ContainsKey($board)) {
        Write-Warning "Unknown board '$board', skipping"
        continue
    }

    foreach ($probe in $probes) {
        $label = "$($probe.name)_$suffix"
        $sketchPath = Join-Path $sketchRoot $probe.sketch
        $args = @(
            "-ExecutionPolicy", "Bypass",
            "-File", $runner,
            "-Board", $board,
            "-Port", $boardPorts[$board],
            "-Sketch", $sketchPath,
            "-Label", $label,
            "-ParseMode", "probe",
            "-StartRegex", $probe.start,
            "-EndRegex", $probe.end,
            "-LineRegex", $probe.line,
            "-KickText", "x",
            "-MinTelemetryLines", [string]$probe.minLines,
            "-TimeoutSeconds", [string]$TimeoutSeconds,
            "-RetryCount", [string]$RetryCount
        )

        Write-Host "=== $board / $($probe.name) on $($boardPorts[$board]) ==="
        $output = & powershell @args
        $exitCode = $LASTEXITCODE
        $output | Write-Host

        $metadataPath = Join-Path $root "tmp\hardware_validation\parsed\$($board)_$($label).json"
        $status = if (Test-Path $metadataPath) {
            try {
                $metadata = Get-Content $metadataPath -Raw | ConvertFrom-Json
                if ($metadata.run_status) { $metadata.run_status } else { $metadata.status }
            } catch {
                "METADATA_PARSE_FAILED"
            }
        } else {
            "NO_METADATA"
        }

        $rows.Add([pscustomobject]@{
            board = $board
            port = $boardPorts[$board]
            probe = $probe.name
            label = $label
            exit_code = $exitCode
            status = $status
            metadata = $metadataPath
        }) | Out-Null
    }
}

$rows | Export-Csv -NoTypeInformation -Encoding UTF8 -Path $summaryPath
Write-Host "Probe matrix summary: $summaryPath"
$rows | Format-Table -AutoSize
