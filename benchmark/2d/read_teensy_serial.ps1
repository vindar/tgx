param(
    [string]$ComPort = "COM3",
    [string]$OutFile = "tmp\tgx_2d_teensy_results.txt",
    [string]$CsvOut = "tmp\tgx_2d_teensy_results.csv",
    [string]$Baseline = "benchmark\2d\baselines\teensy4_hashes.csv",
    [switch]$UpdateBaseline,
    [switch]$NoBaseline,
    [int]$TimeoutSeconds = 120
)

$ErrorActionPreference = "Stop"

$root = Resolve-Path (Join-Path $PSScriptRoot "..\..")
Push-Location $root
try {
    $port = New-Object System.IO.Ports.SerialPort $ComPort, 9600, 'None', 8, 'One'
    $port.ReadTimeout = 500
    $port.DtrEnable = $true
    $port.RtsEnable = $true
    $lines = New-Object System.Collections.Generic.List[string]

    try {
        $port.Open()
        Start-Sleep -Milliseconds 2000
        $port.DiscardInBuffer()
        $port.Write("`n")
        $start = Get-Date
        $quietSince = $null
        $doneSeen = $false

        while (((Get-Date) - $start).TotalSeconds -lt $TimeoutSeconds) {
            try {
                $line = $port.ReadLine().TrimEnd("`r", "`n")
                Write-Output $line
                $lines.Add($line)
                if ($line -like "*TGX 2D suite done*") {
                    $doneSeen = $true
                    $quietSince = Get-Date
                } elseif (-not $doneSeen) {
                    $quietSince = $null
                }
            } catch [System.TimeoutException] {
                if ($quietSince -ne $null -and (((Get-Date) - $quietSince).TotalSeconds -gt 3)) {
                    break
                }
            }
        }
    } finally {
        if ($port.IsOpen) {
            $port.Close()
        }
    }

    New-Item -ItemType Directory -Force (Split-Path $OutFile) | Out-Null
    [System.IO.File]::WriteAllLines((Join-Path (Get-Location) $OutFile), $lines)

    if ($lines -match "ok=false" -or $lines -match "sample_validation=false") {
        throw "Teensy 2D suite reported a failure"
    }

    $resultRows = @()
    foreach ($line in $lines) {
        if ($line.StartsWith("RESULT,")) {
            $resultRows += $line.Substring(7)
        }
    }

    if ($resultRows.Count -gt 0) {
        $csvHeader = "schema_version,platform,color_type,group,kind,iterations,total_us,per_iter_us,hash,changed_pixels,ok,note"
        New-Item -ItemType Directory -Force (Split-Path $CsvOut) | Out-Null
        [System.IO.File]::WriteAllLines((Join-Path (Get-Location) $CsvOut), @($csvHeader) + $resultRows)

        $baselineRows = @("platform,color_type,group,kind,hash,changed_pixels")
        foreach ($row in $resultRows) {
            $cols = $row.Split(",")
            if ($cols.Count -ge 11) {
                $baselineRows += "$($cols[1]),$($cols[2]),$($cols[3]),$($cols[4]),$($cols[8]),$($cols[9])"
            }
        }

        if ($UpdateBaseline) {
            New-Item -ItemType Directory -Force (Split-Path $Baseline) | Out-Null
            [System.IO.File]::WriteAllLines((Join-Path (Get-Location) $Baseline), $baselineRows)
        } elseif (-not $NoBaseline -and (Test-Path $Baseline)) {
            $expected = @{}
            Import-Csv $Baseline | ForEach-Object {
                $key = "$($_.platform)|$($_.color_type)|$($_.group)|$($_.kind)"
                $expected[$key] = "$($_.hash)|$($_.changed_pixels)"
            }

            foreach ($row in $resultRows) {
                $cols = $row.Split(",")
                if ($cols.Count -ge 11) {
                    $key = "$($cols[1])|$($cols[2])|$($cols[3])|$($cols[4])"
                    $actual = "$($cols[8])|$($cols[9])"
                    if (-not $expected.ContainsKey($key)) {
                        throw "Missing Teensy baseline row for $key"
                    }
                    if ($expected[$key] -ne $actual) {
                        throw "Teensy baseline mismatch for $key expected $($expected[$key]) got $actual"
                    }
                }
            }
        }
    }
} finally {
    Pop-Location
}
