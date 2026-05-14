param(
    [string]$BuildDir = "tmp\build_2d_teensy",
    [string]$Fqbn = "teensy:avr:teensy41:usb=serial,speed=600,opt=o3std",
    [string]$ArduinoLibraries = "D:\Programmation\arduino\libraries",
    [string]$Ili9341T4 = "D:\Programmation\ILI9341_T4",
    [string]$Sketch = "",
    [ValidateSet("None", "Decoders", "OpenFontRender", "All")]
    [string]$OptionalSet = "None"
)

$ErrorActionPreference = "Stop"

$root = Resolve-Path (Join-Path $PSScriptRoot "..\..")
Push-Location $root
try {
    if ($Sketch -eq "") {
        if ($OptionalSet -eq "None") {
            $Sketch = "benchmark\2d\teensy4\TGX2DTeensySuite"
        } else {
            $Sketch = "benchmark\2d\teensy4\TGX2DOptionalSuite"
        }
    }

    $optionalDefines = @()
    if (($OptionalSet -eq "Decoders" -or $OptionalSet -eq "All") -and
        (Get-ChildItem -Path $ArduinoLibraries -Recurse -Filter "PNGdec.h" -ErrorAction SilentlyContinue | Select-Object -First 1)) {
        $optionalDefines += "-DTGX_2D_BENCH_HAS_PNGDEC=1"
    }
    if (($OptionalSet -eq "Decoders" -or $OptionalSet -eq "All") -and
        (Get-ChildItem -Path $ArduinoLibraries -Recurse -Filter "JPEGDEC.h" -ErrorAction SilentlyContinue | Select-Object -First 1)) {
        $optionalDefines += "-DTGX_2D_BENCH_HAS_JPEGDEC=1"
    }
    if (($OptionalSet -eq "Decoders" -or $OptionalSet -eq "All") -and
        (Get-ChildItem -Path $ArduinoLibraries -Recurse -Filter "AnimatedGIF.h" -ErrorAction SilentlyContinue | Select-Object -First 1)) {
        $optionalDefines += "-DTGX_2D_BENCH_HAS_ANIMATEDGIF=1"
    }
    if (($OptionalSet -eq "OpenFontRender" -or $OptionalSet -eq "All") -and
        (Get-ChildItem -Path $ArduinoLibraries -Recurse -Filter "OpenFontRender.h" -ErrorAction SilentlyContinue | Select-Object -First 1)) {
        $optionalDefines += "-DTGX_2D_BENCH_HAS_OPENFONTRENDER=1"
    }
    $extraFlags = ($optionalDefines -join " ")
    $rootInclude = "-I$($root.Path.Replace('\', '/'))"
    $teensyCppFlags = "-std=gnu++17 -fno-exceptions -fpermissive -fno-rtti -fno-threadsafe-statics -felide-constructors -Wno-error=narrowing -Wno-psabi $rootInclude $extraFlags"

    arduino-cli compile `
        --fqbn $Fqbn `
        --libraries $ArduinoLibraries `
        --library $root `
        --library $Ili9341T4 `
        --build-property "build.flags.cpp=$teensyCppFlags" `
        --build-path $BuildDir `
        $Sketch
    if ($LASTEXITCODE -ne 0) {
        throw "Teensy 2D build failed with exit code $LASTEXITCODE"
    }
} finally {
    Pop-Location
}
