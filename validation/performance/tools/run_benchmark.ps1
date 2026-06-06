param(
    [Parameter(Mandatory=$true)][string]$Board,
    [Parameter(Mandatory=$true)][string]$Label,
    [string]$RootPath = ".",
    [string]$SketchPath = "",
    [string]$ExtraFlags = "",
    [switch]$CompileOnly
)

$ErrorActionPreference = "Stop"

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..\..")
$root = Resolve-Path $RootPath
$sketch = if ($SketchPath) { Resolve-Path $SketchPath } else { Join-Path $root "examples\benchmark" }
$outDir = Join-Path $repoRoot "tmp\config_optimization\logs"
$buildRoot = Join-Path $repoRoot "tmp\config_optimization\builds"
New-Item -ItemType Directory -Force $outDir,$buildRoot | Out-Null

$boardConfig = @{
    pico2 = @{
        Fqbn = "rp2040:rp2040:rpipico2"
        UploadPort = "COM21"
        SerialPort = "COM21"
        Define = "TGX_BENCHMARK_RP2350"
        Timeout = 420
        Baud = 9600
    }
    picow = @{
        Fqbn = "rp2040:rp2040:rpipicow"
        UploadPort = "COM19"
        SerialPort = "COM19"
        Define = "TGX_BENCHMARK_RP2040"
        Timeout = 420
        Baud = 9600
    }
    core2 = @{
        Fqbn = "esp32:esp32:m5stack_core2"
        UploadPort = "COM5"
        SerialPort = "COM5"
        Define = "TGX_BENCHMARK_ESP32"
        Timeout = 360
        Baud = 9600
    }
    cores3 = @{
        Fqbn = "esp32:esp32:m5stack_cores3"
        UploadPort = "COM10"
        SerialPort = "COM10"
        Define = "TGX_BENCHMARK_ESP32S3"
        Timeout = 420
        Baud = 9600
    }
    teensy41 = @{
        Fqbn = "teensy:avr:teensy41:usb=serial,speed=600,opt=o3std"
        UploadPort = "usb:80000/3/0/1"
        SerialPort = "COM3"
        Define = "TGX_BENCHMARK_T4"
        Timeout = 420
        Baud = 9600
    }
}

if (-not $boardConfig.ContainsKey($Board)) {
    throw "Unknown board '$Board'"
}

$cfg = $boardConfig[$Board]
$safeLabel = ($Label -replace "[^A-Za-z0-9_.-]", "_")
$buildPath = Join-Path $buildRoot "${Board}_${safeLabel}"
$compileLog = Join-Path $outDir "${Board}_${safeLabel}_compile.txt"
$serialLog = Join-Path $outDir "${Board}_${safeLabel}_serial.txt"
$flags = "-D$($cfg.Define)"
if ($ExtraFlags) {
    $flags = "$flags $ExtraFlags"
}

Write-Host "[bench] board=$Board label=$Label root=$root"
Write-Host "[bench] sketch=$sketch"
Write-Host "[bench] fqbn=$($cfg.Fqbn)"
Write-Host "[bench] upload=$($cfg.UploadPort) serial=$($cfg.SerialPort)"
Write-Host "[bench] flags=$flags"

$compileArgs = @(
    "compile",
    "--fqbn", $cfg.Fqbn,
    "--libraries", "D:\Programmation\arduino\libraries",
    "--library", $root,
    "--build-path", $buildPath
)

if ($Board -eq "teensy41") {
    $compileArgs += @("--build-property", "build.flags.defs=-D__IMXRT1062__ -DTEENSYDUINO=160 $flags")
} else {
    $compileArgs += @("--build-property", "compiler.cpp.extra_flags=$flags")
}

if (-not $CompileOnly) {
    $compileArgs += @("--port", $cfg.UploadPort, "--upload")
}

$compileArgs += @($sketch)

$oldErrorActionPreference = $ErrorActionPreference
$ErrorActionPreference = "Continue"
$compileOutput = & arduino-cli @compileArgs 2>&1
$compileExitCode = $LASTEXITCODE
$compileOutput | Set-Content -Path $compileLog
$compileOutput | Where-Object {
    $_ -match "Le croquis utilise|Les variables globales utilisent|Memory Usage|FLASH:|RAM1:|RAM2:|error|fatal|Sketch uses|Global variables use|Could not|Failed"
} | ForEach-Object { Write-Host $_ }
$ErrorActionPreference = $oldErrorActionPreference
if ($compileExitCode -ne 0) {
    throw "Compile/upload failed for $Board/$Label with exit code $compileExitCode"
}

if ($CompileOnly) {
    Write-Host "[bench] compile-only complete: $compileLog"
    exit 0
}

Write-Host "[bench] serial capture: $serialLog"
$lines = New-Object System.Collections.Generic.List[string]
$opened = $false
$retryDeadline = (Get-Date).AddSeconds(60)
while (-not $opened -and (Get-Date) -lt $retryDeadline) {
    try {
        $serial = [System.IO.Ports.SerialPort]::new($cfg.SerialPort, $cfg.Baud, [System.IO.Ports.Parity]::None, 8, [System.IO.Ports.StopBits]::One)
        $serial.ReadTimeout = 500
        $serial.WriteTimeout = 1000
        $serial.DtrEnable = $true
        $serial.RtsEnable = $true
        $serial.Open()
        $opened = $true
    } catch {
        Start-Sleep -Seconds 1
    }
}

if (-not $opened) {
    throw "Could not open serial port $($cfg.SerialPort)"
}

try {
    Start-Sleep -Milliseconds 2500
    try { $serial.Write("x") } catch {}
    $start = Get-Date
    while (((Get-Date) - $start).TotalSeconds -lt $cfg.Timeout) {
        try {
            $line = $serial.ReadLine().TrimEnd("`r", "`n")
            $lines.Add($line)
            if ($line -match "TGX Benchmark|BENCHMARK FINAL SCORE|Benchmark completed|FINAL SCORE|SYNTHETIC SCORE") {
                Write-Host $line
            }
            if ($line -match "Benchmark completed") { break }
        } catch [System.TimeoutException] {}
    }
} finally {
    if ($serial.IsOpen) { $serial.Close() }
}

[System.IO.File]::WriteAllLines($serialLog, $lines)
Write-Host "[bench] captured $($lines.Count) lines to $serialLog"

if (-not (($lines -join "`n") -match "Benchmark completed")) {
    throw "Benchmark did not complete for $Board/$Label"
}
