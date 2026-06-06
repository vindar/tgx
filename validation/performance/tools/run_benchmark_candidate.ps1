param(
    [Parameter(Mandatory=$true)][string]$Candidate,
    [string[]]$Boards = @("pico2", "cores3", "teensy41", "core2"),
    [string]$ExtraFlags = "",
    [int]$TimeoutSeconds = 180,
    [int]$RetryCount = 1
)

$ErrorActionPreference = "Stop"
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$root = Resolve-Path (Join-Path $scriptDir "..\..\..")
$runner = Join-Path $root "tmp\hardware_validation\tools\upload_and_capture.ps1"
$summaryPath = Join-Path $root "tmp\inline_granularity\benchmark_runs_$Candidate.csv"

$boardRows = @{
    "core2" = @{ board="core2"; port="COM5"; expected=2; minSubtests=100 }
    "cores3" = @{ board="cores3"; port="COM10"; expected=3; minSubtests=150 }
    "pico2" = @{ board="pico2"; port="COM21"; expected=3; minSubtests=150 }
    "teensy41" = @{ board="teensy41"; port="COM3"; expected=3; minSubtests=150 }
}

$rows = New-Object System.Collections.Generic.List[object]
foreach ($name in $Boards) {
    if (-not $boardRows.ContainsKey($name)) {
        Write-Warning "Unknown board '$name', skipping"
        continue
    }
    $b = $boardRows[$name]
    $label = "${Candidate}_bench_$($b.board)"
    Write-Host "=== $Candidate / $($b.board) ==="
    $args = @(
        "-ExecutionPolicy", "Bypass",
        "-File", $runner,
        "-Board", $b.board,
        "-Port", $b.port,
        "-Sketch", (Join-Path $root "examples\benchmark"),
        "-Label", $label,
        "-ParseMode", "benchmark",
        "-Baud", "115200",
        "-TimeoutSeconds", [string]$TimeoutSeconds,
        "-RetryCount", [string]$RetryCount,
        "-ExpectedGlobalScores", [string]$b.expected,
        "-MinSubtests", [string]$b.minSubtests,
        "-KickText", "x",
        "-UseBoardBenchmarkDefine"
    )
    if ($ExtraFlags) {
        $args += @("-ExtraFlags", $ExtraFlags)
    }
    $output = & powershell @args
    $exitCode = $LASTEXITCODE
    $output | Write-Host

    $metadataPath = Join-Path $root "tmp\hardware_validation\parsed\$($b.board)_$label.json"
    $status = "NO_METADATA"
    $globals = ""
    $subtests = ""
    if (Test-Path $metadataPath) {
        try {
            $m = Get-Content $metadataPath -Raw | ConvertFrom-Json
            $status = $m.run_status
            $globals = $m.parsed_global_scores
            $subtests = $m.parsed_subtests
        } catch {
            $status = "METADATA_PARSE_FAILED"
        }
    }
    $rows.Add([pscustomobject]@{
        candidate = $Candidate
        board = $b.board
        port = $b.port
        label = $label
        extra_flags = $ExtraFlags
        exit_code = $exitCode
        status = $status
        parsed_global_scores = $globals
        parsed_subtests = $subtests
        metadata = $metadataPath
    }) | Out-Null
}

$rows | Export-Csv -NoTypeInformation -Encoding UTF8 -Path $summaryPath
Write-Host "Benchmark candidate summary: $summaryPath"
$rows | Format-Table -AutoSize
