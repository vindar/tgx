param(
    [string]$BuildDir = "tmp\build_2d_teensy",
    [string]$Fqbn = "teensy:avr:teensy41:usb=serial,speed=600,opt=o3std",
    [string]$Port = "usb:80000/3/0/4",
    [string]$Sketch = "benchmark\2d\teensy4\TGX2DTeensySuite"
)

$ErrorActionPreference = "Stop"

$root = Resolve-Path (Join-Path $PSScriptRoot "..\..")
Push-Location $root
try {
    arduino-cli upload `
        -p $Port `
        --protocol teensy `
        --fqbn $Fqbn `
        --build-path $BuildDir `
        $Sketch
    if ($LASTEXITCODE -ne 0) {
        throw "Teensy upload failed with exit code $LASTEXITCODE"
    }
} finally {
    Pop-Location
}
