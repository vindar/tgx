param(
    [string]$OutDir = "tmp\tgx_3d_cpu_suite",
    [string]$BuildDir = "tmp\build_3d_cpu",
    [string]$Baseline = "benchmark\3d\baselines\cpu_validation.csv",
    [switch]$UpdateBaseline,
    [switch]$NoBaseline,
    [string]$Compare = "tolerant",
    [switch]$UpdateGolden,
    [string]$GoldenDir = "benchmark\3d\golden\cpu",
    [switch]$Demo,
    [switch]$DemoOnly
)

$ErrorActionPreference = "Stop"

$root = Resolve-Path (Join-Path $PSScriptRoot "..\..")
Push-Location $root
try {
    cmake -S benchmark\3d\cpu -B $BuildDir -G "Visual Studio 17 2022" -A x64
    if ($LASTEXITCODE -ne 0) {
        throw "CPU 3D CMake configure failed with exit code $LASTEXITCODE"
    }

    cmake --build $BuildDir --config Release
    if ($LASTEXITCODE -ne 0) {
        throw "CPU 3D build failed with exit code $LASTEXITCODE"
    }

    $exe = Join-Path $BuildDir "Release\tgx_3d_cpu_suite.exe"
    $args = @("--out", $OutDir, "--compare", $Compare)
    if ($Demo) {
        $args += "--demo"
    }
    if ($DemoOnly) {
        $args += "--demo-only"
    }
    if ($UpdateGolden) {
        $args += @("--write-golden", $GoldenDir)
    }
    if ($UpdateBaseline) {
        $args += @("--write-baseline", $Baseline)
    } elseif (-not $NoBaseline -and (Test-Path $Baseline)) {
        $args += @("--baseline", $Baseline)
    }

    & $exe @args
    if ($LASTEXITCODE -ne 0) {
        throw "CPU 3D suite failed with exit code $LASTEXITCODE"
    }
} finally {
    Pop-Location
}
