$ErrorActionPreference = "Stop"

$Python = $env:TGX_MESH3D2_PYTHON
if (-not $Python) {
    $CondaPython = "C:\Users\Vindar\anaconda3\envs\tgxmesh3d2\python.exe"
    if (Test-Path $CondaPython) {
        $Python = $CondaPython
    } else {
        $Python = "python"
    }
}

& $Python "$PSScriptRoot\generate_assets.py"
cmake -S $PSScriptRoot -B "$PSScriptRoot\build"
cmake --build "$PSScriptRoot\build" --config Release
& "$PSScriptRoot\build\Release\tgx_mesh3d2_validation.exe" "$PSScriptRoot\out"
