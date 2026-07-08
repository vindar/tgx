param(
    [Parameter(Mandatory=$true)][string]$Board,
    [Parameter(Mandatory=$true)][string]$Example,
    [Parameter(Mandatory=$true)][string]$Label,
    [int]$CaptureSeconds = 12,
    [switch]$CompileOnly
)

$ErrorActionPreference = "Stop"
$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..\..\..")
$outDir = Join-Path $repoRoot "tmp\config_optimization\example_logs"
$buildRoot = Join-Path $repoRoot "tmp\config_optimization\example_builds"
New-Item -ItemType Directory -Force $outDir,$buildRoot | Out-Null

$boardConfig = @{
    pico2 = @{ Fqbn = "rp2040:rp2040:rpipico2:opt=Fast"; UploadPort = "COM21"; SerialPort = "COM21"; Baud = 115200 }
    picow = @{ Fqbn = "rp2040:rp2040:rpipicow"; UploadPort = "COM19"; SerialPort = "COM19"; Baud = 115200 }
    core2 = @{ Fqbn = "esp32:esp32:m5stack_core2"; UploadPort = "COM5"; SerialPort = "COM5"; Baud = 115200 }
    cores3 = @{ Fqbn = "esp32:esp32:m5stack_cores3"; UploadPort = "COM10"; SerialPort = "COM10"; Baud = 115200 }
    feathers2 = @{ Fqbn = "esp32:esp32:adafruit_feather_esp32s2_tft"; UploadPort = "COM11"; SerialPort = "COM11"; Baud = 115200 }
    feathers3 = @{ Fqbn = "esp32:esp32:adafruit_feather_esp32s3_tft"; UploadPort = "COM14"; SerialPort = "COM14"; Baud = 115200 }
    teensy41 = @{ Fqbn = "teensy:avr:teensy41:usb=serial,speed=600,opt=o3std"; UploadPort = "usb:80000/1/0/5/4/4"; SerialPort = "COM3"; Baud = 9600 }
    teensy36 = @{ Fqbn = "teensy:avr:teensy36:usb=serial,speed=180,opt=o3std"; UploadPort = "usb:80000/3/0/1"; SerialPort = "COM23"; Baud = 9600 }
}

$exampleMap = @{
    pico2_bunny_fig = "examples\Pico_RP2040_RP2350\bunny_fig"
    pico2_borg_cube = "examples\Pico_RP2040_RP2350\borg_cube"
    pico2_scream = "examples\Pico_RP2040_RP2350\scream"
    picow_bunny_fig = "examples\Pico_RP2040_RP2350\bunny_fig"
    picow_borg_cube = "examples\Pico_RP2040_RP2350\borg_cube"
    picow_scream = "examples\Pico_RP2040_RP2350\scream"
    picow_2d_showcase = "examples\Pico_RP2040_RP2350\TGX_2D_Showcase"
    core2_donkeykong = "examples\M5Stack\donkeykong"
    core2_borg_cube = "examples\M5Stack\borg_cube"
    core2_scream = "examples\M5Stack\scream"
    cores3_donkeykong = "examples\M5Stack\donkeykong"
    cores3_borg_cube = "examples\M5Stack\borg_cube"
    cores3_scream = "examples\M5Stack\scream"
    feathers2_donkeykong = "examples\ESP32\donkeykong"
    feathers2_borg_cube = "examples\ESP32\borg_cube"
    feathers2_scream = "examples\ESP32\scream"
    feathers3_donkeykong = "examples\ESP32\donkeykong"
    feathers3_borg_cube = "examples\ESP32\borg_cube"
    feathers3_scream = "examples\ESP32\scream"
    teensy41_characters = "examples\Teensy4\3D\characters"
    teensy41_test_texture = "examples\Teensy4\3D\test-texture"
    teensy41_test_shading = "examples\Teensy4\3D\test-shading"
    teensy41_mars = "examples\Teensy4\3D\mars"
    teensy41_buddha = "examples\Teensy4\3D\buddha"
    teensy41_borg_cube = "examples\Teensy4\3D\borg_cube"
    teensy41_scream = "examples\Teensy4\3D\scream"
    teensy36_characters = "examples\Teensy3\3D\characters"
    teensy36_scream = "examples\Teensy3\3D\scream"
}

if (-not $boardConfig.ContainsKey($Board)) { throw "Unknown board '$Board'" }
$key = "${Board}_${Example}"
if (-not $exampleMap.ContainsKey($key)) { throw "Unknown example '$key'" }

$cfg = $boardConfig[$Board]
$safe = "${Board}_${Example}_${Label}" -replace "[^A-Za-z0-9_.-]", "_"
$buildPath = Join-Path $buildRoot $safe
$compileLog = Join-Path $outDir "${safe}_compile.txt"
$serialLog = Join-Path $outDir "${safe}_serial.txt"
$sketch = Join-Path $repoRoot $exampleMap[$key]

Write-Host "[example] board=$Board example=$Example label=$Label"
Write-Host "[example] sketch=$sketch"

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
$compileOutput = & arduino-cli @compileArgs 2>&1
$compileExitCode = $LASTEXITCODE
$compileOutput | Set-Content -Path $compileLog
$compileOutput | Where-Object {
    $_ -match "Le croquis utilise|Les variables globales utilisent|Memory Usage|FLASH:|RAM1:|RAM2:|error|fatal|Failed|Could not"
} | ForEach-Object { Write-Host $_ }
$ErrorActionPreference = $oldErrorActionPreference
if ($compileExitCode -ne 0) { throw "Compile/upload failed for $Board/$Example/$Label" }
if ($CompileOnly) { Write-Host "[example] compile-only complete"; exit 0 }

$lines = New-Object System.Collections.Generic.List[string]
$opened = $false
$serialOpenTimeout = 60
if ($Board -eq "teensy41" -or $Board -eq "teensy36") {
    $serialOpenTimeout = 150
}
$deadline = (Get-Date).AddSeconds($serialOpenTimeout)
while (-not $opened -and (Get-Date) -lt $deadline) {
    try {
        $serial = [System.IO.Ports.SerialPort]::new($cfg.SerialPort, $cfg.Baud, [System.IO.Ports.Parity]::None, 8, [System.IO.Ports.StopBits]::One)
        $serial.ReadTimeout = 500
        $serial.DtrEnable = $true
        $serial.RtsEnable = -not (@("feathers2", "feathers3") -contains $Board)
        $serial.Open()
        $opened = $true
    } catch {
        Start-Sleep -Seconds 1
    }
}
if (-not $opened) { throw "Could not open serial port $($cfg.SerialPort)" }

try {
    Start-Sleep -Milliseconds 1800
    try { $serial.Write("x") } catch {}
    $start = Get-Date
    while (((Get-Date) - $start).TotalSeconds -lt $CaptureSeconds) {
        try {
            $line = $serial.ReadLine().TrimEnd("`r", "`n")
            $lines.Add($line)
            if ($line -match "fps|telemetry|scene|display|scream|donkeykong|borg|TGX") {
                Write-Host $line
            }
        } catch [System.TimeoutException] {}
    }
} finally {
    if ($serial.IsOpen) { $serial.Close() }
}

[System.IO.File]::WriteAllLines($serialLog, $lines)
Write-Host "[example] captured $($lines.Count) lines to $serialLog"
