param(
    [string]$OutDir = "tmp\tgx_2d_cpu_suite",
    [string]$BuildDir = "tmp\build_2d_cpu",
    [string]$Baseline = "benchmark\2d\baselines\cpu_hashes.csv",
    [switch]$UpdateBaseline,
    [switch]$NoBaseline,
    [switch]$Large,
    [int]$LargeSize = 2048,
    [switch]$Golden,
    [switch]$UpdateGolden,
    [string]$GoldenDir = "benchmark\2d\golden\cpu",
    [string]$GoldenDiffDir = ""
)

$ErrorActionPreference = "Stop"

$root = Resolve-Path (Join-Path $PSScriptRoot "..\..")
Push-Location $root
try {
    if ($Large) {
        if ($OutDir -eq "tmp\tgx_2d_cpu_suite") {
            $OutDir = "tmp\tgx_2d_cpu_suite_large"
        }
        if ($Baseline -eq "benchmark\2d\baselines\cpu_hashes.csv") {
            $Baseline = "benchmark\2d\baselines\cpu_large_hashes.csv"
        }
    }

    cmake -S benchmark\2d\cpu -B $BuildDir -G "Visual Studio 17 2022" -A x64
    if ($LASTEXITCODE -ne 0) {
        throw "CPU 2D CMake configure failed with exit code $LASTEXITCODE"
    }

    cmake --build $BuildDir --config Release
    if ($LASTEXITCODE -ne 0) {
        throw "CPU 2D build failed with exit code $LASTEXITCODE"
    }

    $exe = Join-Path $BuildDir "Release\tgx_2d_cpu_suite.exe"
    $args = @("--out", $OutDir)
    if ($Large) {
        $args += @("--large", "--large-size", $LargeSize)
    }
    if ($Golden) {
        $args += @("--golden", $GoldenDir)
        if ($GoldenDiffDir -ne "") {
            $args += @("--golden-diff", $GoldenDiffDir)
        }
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
        throw "CPU 2D suite failed with exit code $LASTEXITCODE"
    }
} finally {
    Pop-Location
}
