param(
    [Parameter(Mandatory=$true)][string]$Board,
    [Parameter(Mandatory=$true)][string]$Label,
    [switch]$CompileOnly
)

$ErrorActionPreference = "Stop"
$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..\..")
$sketch = Join-Path $repoRoot "tmp\config_optimization\macro_probe"
$outDir = Join-Path $repoRoot "tmp\config_optimization\macro_probe_logs"
$buildRoot = Join-Path $repoRoot "tmp\config_optimization\macro_probe_builds"
New-Item -ItemType Directory -Force $outDir,$buildRoot | Out-Null

$boardConfig = @{
    pico2 = @{ Fqbn = "rp2040:rp2040:rpipico2"; UploadPort = "COM21"; SerialPort = "COM21"; Baud = 115200 }
    picow = @{ Fqbn = "rp2040:rp2040:rpipicow"; UploadPort = "COM19"; SerialPort = "COM19"; Baud = 115200 }
    core2 = @{ Fqbn = "esp32:esp32:m5stack_core2"; UploadPort = "COM5"; SerialPort = "COM5"; Baud = 115200 }
    cores3 = @{ Fqbn = "esp32:esp32:m5stack_cores3"; UploadPort = "COM10"; SerialPort = "COM10"; Baud = 115200 }
    teensy41 = @{ Fqbn = "teensy:avr:teensy41:usb=serial,speed=600,opt=o3std"; UploadPort = "usb:80000/3/0/1"; SerialPort = "COM3"; Baud = 115200 }
}

if (-not $boardConfig.ContainsKey($Board)) { throw "Unknown board '$Board'" }
$cfg = $boardConfig[$Board]
$safe = "${Board}_${Label}" -replace "[^A-Za-z0-9_.-]", "_"
$buildPath = Join-Path $buildRoot $safe
$compileLog = Join-Path $outDir "${safe}_compile.txt"
$serialLog = Join-Path $outDir "${safe}_serial.txt"

$compileArgs = @(
    "compile",
    "--fqbn", $cfg.Fqbn,
    "--libraries", "D:\Programmation\arduino\libraries",
    "--library", $repoRoot,
    "--build-path", $buildPath
)
if (-not $CompileOnly) {
    $compileArgs += @("--port", $cfg.UploadPort, "--upload")
}
$compileArgs += $sketch

$oldErrorActionPreference = $ErrorActionPreference
$ErrorActionPreference = "Continue"
Write-Host "[macro] board=$Board label=$Label"
$compileOutput = & arduino-cli @compileArgs 2>&1
$compileExitCode = $LASTEXITCODE
$compileOutput | Set-Content -Path $compileLog
$compileOutput | Where-Object { $_ -match "error|fatal|Memory Usage|Le croquis utilise|Sketch uses|RAM1|RAM2|FLASH" } | ForEach-Object { Write-Host $_ }
$ErrorActionPreference = $oldErrorActionPreference
if ($compileExitCode -ne 0) { throw "Macro probe compile/upload failed for $Board/$Label" }
if ($CompileOnly) { exit 0 }

$lines = New-Object System.Collections.Generic.List[string]
$opened = $false
$deadline = (Get-Date).AddSeconds(90)
while (-not $opened -and (Get-Date) -lt $deadline) {
    try {
        $serial = [System.IO.Ports.SerialPort]::new($cfg.SerialPort, $cfg.Baud, [System.IO.Ports.Parity]::None, 8, [System.IO.Ports.StopBits]::One)
        $serial.ReadTimeout = 500
        $serial.DtrEnable = $true
        $serial.RtsEnable = $true
        $serial.Open()
        $opened = $true
    } catch {
        Start-Sleep -Seconds 1
    }
}
if (-not $opened) { throw "Could not open serial port $($cfg.SerialPort)" }

try {
    try { $serial.Write("x") } catch {}
    $start = Get-Date
    while (((Get-Date) - $start).TotalSeconds -lt 18) {
        try {
            $line = $serial.ReadLine().TrimEnd("`r", "`n")
            $lines.Add($line)
            Write-Host $line
            if ($line -match "TGX macro probe done") { break }
        } catch [System.TimeoutException] {}
    }
} finally {
    if ($serial.IsOpen) { $serial.Close() }
}

[System.IO.File]::WriteAllLines($serialLog, $lines)
