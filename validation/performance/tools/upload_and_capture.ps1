param(
    [Parameter(Mandatory=$true)][string]$Board,
    [Parameter(Mandatory=$true)][string]$Sketch,
    [string]$Port = "",
    [string]$UploadPort = "",
    [string]$Fqbn = "",
    [string]$Label = "",
    [ValidateSet("generic", "probe", "benchmark", "example")][string]$ParseMode = "generic",
    [int]$Baud = 9600,
    [int]$TimeoutSeconds = 120,
    [int]$PortWaitSeconds = 120,
    [int]$RetryCount = 1,
    [string]$StartRegex = "",
    [string]$EndRegex = "",
    [string]$LineRegex = "",
    [int]$MinTelemetryLines = 0,
    [int]$ExpectedGlobalScores = 0,
    [int]$MinSubtests = 0,
    [int]$QuietPeriodSeconds = 0,
    [string]$KickText = "",
    [int]$KickDelayMs = 500,
    [int]$PostUploadDelaySeconds = -1,
    [switch]$FlushOnOpen,
    [switch]$CompileOnly,
    [switch]$SkipCompileUpload,
    [switch]$UseBoardBenchmarkDefine,
    [string]$ExtraFlags = "",
    [string]$Libraries = "D:\Programmation\arduino\libraries",
    [string]$ArduinoCli = "C:\Users\Vindar\AppData\Local\Programs\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe"
)

$ErrorActionPreference = "Stop"
[System.Threading.Thread]::CurrentThread.CurrentCulture = [System.Globalization.CultureInfo]::InvariantCulture
[System.Threading.Thread]::CurrentThread.CurrentUICulture = [System.Globalization.CultureInfo]::InvariantCulture

$scriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$hvRoot = Resolve-Path (Join-Path $scriptRoot "..")
$repoRoot = Resolve-Path (Join-Path $scriptRoot "..\..\..")
$logsDir = Join-Path $hvRoot "logs"
$telemetryDir = Join-Path $hvRoot "telemetry"
$parsedDir = Join-Path $hvRoot "parsed"
$buildsDir = Join-Path $hvRoot "builds"
New-Item -ItemType Directory -Force $logsDir, $telemetryDir, $parsedDir, $buildsDir | Out-Null

$boardConfig = @{
    "core2" = @{
        Fqbn = "esp32:esp32:m5stack_core2"
        Port = "COM5"
        UploadPort = "COM5"
        BenchmarkDefine = "TGX_BENCHMARK_ESP32"
        PostDelay = 2
        PortWait = 90
    }
    "cores3" = @{
        Fqbn = "esp32:esp32:m5stack_cores3"
        Port = "COM10"
        UploadPort = "COM10"
        BenchmarkDefine = "TGX_BENCHMARK_ESP32S3"
        PostDelay = 2
        PortWait = 90
    }
    "feathers3" = @{
        Fqbn = "esp32:esp32:adafruit_feather_esp32s3_tft"
        Port = "COM14"
        UploadPort = "COM14"
        BenchmarkDefine = "TGX_BENCHMARK_ESP32S3"
        PostDelay = 2
        PortWait = 90
    }
    "feathers2" = @{
        Fqbn = "esp32:esp32:adafruit_feather_esp32s2_tft"
        Port = "COM11"
        UploadPort = "COM11"
        BenchmarkDefine = "TGX_BENCHMARK_ESP32S2"
        PostDelay = 2
        PortWait = 90
    }
    "picow" = @{
        Fqbn = "rp2040:rp2040:rpipicow"
        Port = "COM19"
        UploadPort = "COM19"
        BenchmarkDefine = "TGX_BENCHMARK_RP2040"
        PostDelay = 2
        PortWait = 150
    }
    "pico2" = @{
        Fqbn = "rp2040:rp2040:rpipico2:opt=Fast"
        Port = "COM21"
        UploadPort = "COM21"
        BenchmarkDefine = "TGX_BENCHMARK_RP2350"
        PostDelay = 2
        PortWait = 150
    }
    "teensy41" = @{
        Fqbn = "teensy:avr:teensy41:usb=serial,speed=600,opt=o3std"
        Port = "COM3"
        UploadPort = "usb:80000/3/0/1"
        BenchmarkDefine = "TGX_BENCHMARK_T4"
        PostDelay = 4
        PortWait = 180
    }
    "teensy36" = @{
        Fqbn = "teensy:avr:teensy36:usb=serial,speed=180,opt=o3std"
        Port = "COM23"
        UploadPort = "usb:80000/3/0/1"
        BenchmarkDefine = "TGX_BENCHMARK_T36"
        PostDelay = 4
        PortWait = 180
        TeensyDefs = "-D__MK66FX1M0__ -DTEENSYDUINO=160"
    }
}

$boardKey = $Board.ToLowerInvariant()
if (-not $boardConfig.ContainsKey($boardKey)) {
    throw "Unknown board '$Board'. Known boards: $($boardConfig.Keys -join ', ')"
}

$cfg = $boardConfig[$boardKey]
if (-not $Fqbn) { $Fqbn = $cfg.Fqbn }
if (-not $Port) { $Port = $cfg.Port }
if (-not $UploadPort) { $UploadPort = $cfg.UploadPort }
if ($PortWaitSeconds -le 0) { $PortWaitSeconds = $cfg.PortWait }
if ($PostUploadDelaySeconds -lt 0) { $PostUploadDelaySeconds = $cfg.PostDelay }
if (-not $Label) { $Label = "$(Get-Date -Format 'yyyyMMdd_HHmmss')" }

if ($ParseMode -eq "benchmark") {
    if (-not $StartRegex) { $StartRegex = "TGX Benchmark" }
    if (-not $EndRegex) { $EndRegex = "Benchmark completed" }
    if ($ExpectedGlobalScores -eq 0) { $ExpectedGlobalScores = 1 }
    if ($MinSubtests -eq 0) { $MinSubtests = 1 }
}
if ($ParseMode -eq "probe") {
    if ($MinTelemetryLines -eq 0) { $MinTelemetryLines = 1 }
}
if ($ParseMode -eq "example") {
    if (-not $LineRegex) { $LineRegex = "(\[TGX telemetry\]|fps=| FPS=| fps)" }
    if ($MinTelemetryLines -eq 0) { $MinTelemetryLines = 3 }
    if ($QuietPeriodSeconds -eq 0 -and -not $EndRegex) { $QuietPeriodSeconds = 8 }
}

$safeBoard = ($Board -replace "[^A-Za-z0-9_.-]", "_")
$safeLabel = ($Label -replace "[^A-Za-z0-9_.-]", "_")
$runId = "${safeBoard}_${safeLabel}"
$logPath = Join-Path $logsDir "${runId}.log"
$telemetryPath = Join-Path $telemetryDir "${runId}.txt"
$metadataPath = Join-Path $parsedDir "${runId}.json"
$benchmarkCsvPath = Join-Path $parsedDir "${runId}_benchmark.csv"
$exampleCsvPath = Join-Path $parsedDir "${runId}_example.csv"
$buildPath = Join-Path $buildsDir $runId

function Add-Log {
    param([string]$Text)
    $line = "$(Get-Date -Format 'yyyy-MM-dd HH:mm:ss.fff') $Text"
    Write-Host $line
    Add-Content -Path $logPath -Value $line
}

function Get-DotNetPorts {
    try {
        return [System.IO.Ports.SerialPort]::GetPortNames() | Sort-Object
    } catch {
        return @()
    }
}

function Get-BoardListText {
    try {
        return (& $ArduinoCli board list 2>&1) -join "`n"
    } catch {
        return "arduino-cli board list failed: $($_.Exception.Message)"
    }
}

function Test-SerialOpen {
    param([string]$SerialPort, [int]$SerialBaud)
    $sp = $null
    try {
        $sp = [System.IO.Ports.SerialPort]::new($SerialPort, $SerialBaud, [System.IO.Ports.Parity]::None, 8, [System.IO.Ports.StopBits]::One)
        $sp.ReadTimeout = 250
        $sp.WriteTimeout = 250
        $espSerial = @("core2", "cores3") -contains $boardKey
        $rpSerial = @("picow", "pico2") -contains $boardKey
        $nativeUsbEsp = @("feathers2", "feathers3") -contains $boardKey
        $sp.DtrEnable = -not $espSerial
        $sp.RtsEnable = (-not $espSerial) -and (-not $rpSerial) -and (-not $nativeUsbEsp)
        $sp.Open()
        if ($espSerial) {
            $sp.DtrEnable = $false
            $sp.RtsEnable = $false
        }
        return $true
    } catch {
        return $false
    } finally {
        if ($sp -and $sp.IsOpen) { $sp.Close() }
        if ($sp) { $sp.Dispose() }
    }
}

function Wait-SerialVisible {
    param([string]$SerialPort, [int]$SerialBaud, [int]$Timeout)
    $deadline = (Get-Date).AddSeconds($Timeout)
    $seen = $false
    while ((Get-Date) -lt $deadline) {
        $ports = @(Get-DotNetPorts)
        if ($ports -contains $SerialPort) {
            $seen = $true
            return @{ Ok = $true; Seen = $seen; Ports = $ports }
        }
        Start-Sleep -Milliseconds 500
    }
    return @{ Ok = $false; Seen = $seen; Ports = @(Get-DotNetPorts) }
}

function Write-CsvRows {
    param([array]$Rows, [string]$Path)
    if ($Rows.Count -eq 0) {
        "" | Set-Content -Path $Path
    } else {
        $Rows | Export-Csv -NoTypeInformation -Path $Path
    }
}

function Parse-Benchmark {
    param([string[]]$Lines)
    $globalScores = New-Object System.Collections.Generic.List[object]
    $subtests = New-Object System.Collections.Generic.List[object]
    $rendererSize = ""
    $lineNumber = 0
    foreach ($line in $Lines) {
        $lineNumber++
        if ($line -match "^RENDERER SIZE:\s*(.+)$") {
            $rendererSize = $Matches[1].Trim()
        }
        if ($line -match "\*\*\*\s*BENCHMARK FINAL SCORE:\s*([0-9.]+)\s*fps\s*\*\*") {
            $globalScores.Add([pscustomobject]@{
                board = $Board; label = $Label; score_index = $globalScores.Count + 1
                score = [double]$Matches[1]; line_number = $lineNumber
            })
        }
        if ($line -match "^\s*(.+?)\s+\[[\.\s]+\]\s+frames:\s*([0-9]+).*?min:\s*([0-9]+)\s*us.*?max:\s*([0-9]+)\s*us.*?avg:\s*([0-9.]+)\s*us.*?stddev:\s*([0-9.]+)\s*us.*?fps:\s*([0-9.]+)") {
            $subtests.Add([pscustomobject]@{
                board = $Board; label = $Label; renderer_size = $rendererSize
                subtest = $Matches[1].Trim(); frames = [int]$Matches[2]
                min_us = [int]$Matches[3]; max_us = [int]$Matches[4]
                avg_us = [double]$Matches[5]; stddev_us = [double]$Matches[6]
                fps = [double]$Matches[7]; line_number = $lineNumber
            })
        }
    }
    Write-CsvRows -Rows $globalScores -Path ($benchmarkCsvPath -replace "_benchmark\.csv$", "_benchmark_global.csv")
    Write-CsvRows -Rows $subtests -Path $benchmarkCsvPath
    return @{
        ParseSucceeded = (($globalScores.Count -ge $ExpectedGlobalScores) -and ($subtests.Count -ge $MinSubtests))
        GlobalScores = $globalScores.Count
        Subtests = $subtests.Count
    }
}

function Parse-Example {
    param([string[]]$Lines)
    $rows = New-Object System.Collections.Generic.List[object]
    $lineNumber = 0
    foreach ($line in $Lines) {
        $lineNumber++
        $scene = ""
        $fps = $null
        $frameAvg = $null
        $frames = $null
        if ($line -match "\[TGX telemetry\].*?scene=([^ ]+).*?fps=([0-9.]+)") {
            $scene = $Matches[1]
            $fps = [double]$Matches[2]
            if ($line -match "frame_avg_us=([0-9.]+)") { $frameAvg = [double]$Matches[1] }
            if ($line -match "frames=([0-9]+)") { $frames = [int]$Matches[1] }
        } elseif ($line -match "SCENE=([^ ]+).*?FPS=([0-9.]+)") {
            $scene = $Matches[1]
            $fps = [double]$Matches[2]
        } elseif ($line -match "scene=([^ ]+).*?fps=([0-9.]+)") {
            $scene = $Matches[1]
            $fps = [double]$Matches[2]
        } elseif ($line -match "fps=([0-9.]+)") {
            $scene = "unknown"
            $fps = [double]$Matches[1]
        } elseif ($line -match "\bfps\s+([0-9.]+)") {
            $scene = "unknown"
            $fps = [double]$Matches[1]
        }
        if ($null -ne $fps) {
            $rows.Add([pscustomobject]@{
                board = $Board; label = $Label; scene = $scene; fps = $fps
                frame_avg_us = $frameAvg; frames = $frames; line_number = $lineNumber
                raw = $line
            })
        }
    }
    Write-CsvRows -Rows $rows -Path $exampleCsvPath
    return @{
        ParseSucceeded = ($rows.Count -ge $MinTelemetryLines)
        TelemetryRows = $rows.Count
    }
}

function Capture-Serial {
    param([int]$Attempt)
    $lines = New-Object System.Collections.Generic.List[string]
    $events = New-Object System.Collections.Generic.List[object]
    $startSeen = [string]::IsNullOrEmpty($StartRegex)
    $endSeen = [string]::IsNullOrEmpty($EndRegex)
    $lineMatches = 0
    $sp = $null
    $opened = $false
    try {
        $openDeadline = (Get-Date).AddSeconds([Math]::Max(5, $PortWaitSeconds))
        while (-not $opened -and (Get-Date) -lt $openDeadline) {
            try {
                $sp = [System.IO.Ports.SerialPort]::new($Port, $Baud, [System.IO.Ports.Parity]::None, 8, [System.IO.Ports.StopBits]::One)
                $sp.ReadTimeout = 250
                $sp.WriteTimeout = 1000
                $espSerial = @("core2", "cores3") -contains $boardKey
                $rpSerial = @("picow", "pico2") -contains $boardKey
                $nativeUsbEsp = @("feathers2", "feathers3") -contains $boardKey
                $sp.DtrEnable = -not $espSerial
                $sp.RtsEnable = (-not $espSerial) -and (-not $rpSerial) -and (-not $nativeUsbEsp)
                $sp.Open()
                if ($espSerial) {
                    $sp.DtrEnable = $false
                    $sp.RtsEnable = $false
                }
                $opened = $true
            } catch {
                if ($sp) {
                    if ($sp.IsOpen) { $sp.Close() }
                    $sp.Dispose()
                    $sp = $null
                }
                Start-Sleep -Milliseconds 500
            }
        }
        if (-not $opened) {
            return @{
                Status = "UPLOAD_OK_SERIAL_CAPTURE_FAILED"
                Lines = $lines
                LineCount = 0
                StartSeen = $startSeen
                EndSeen = $endSeen
                LineMatches = 0
            }
        }
        if ($FlushOnOpen) { $sp.DiscardInBuffer() }
        if ($KickText) {
            Start-Sleep -Milliseconds $KickDelayMs
            try { $sp.Write($KickText) } catch {}
        }
        $captureStart = Get-Date
        $lastLineTime = $captureStart
        while (((Get-Date) - $captureStart).TotalSeconds -lt $TimeoutSeconds) {
            try {
                $line = $sp.ReadLine()
                $line = $line.TrimEnd("`r", "`n")
                $lines.Add($line)
                $now = Get-Date
                $lastLineTime = $now
                $events.Add([pscustomobject]@{
                    t_ms = [int](($now - $captureStart).TotalMilliseconds)
                    line = $line
                })
                if (-not $startSeen -and $line -match $StartRegex) { $startSeen = $true }
                if (-not [string]::IsNullOrEmpty($LineRegex) -and $line -match $LineRegex) { $lineMatches++ }
                if (-not $endSeen -and $line -match $EndRegex) {
                    $endSeen = $true
                    break
                }
            } catch [System.TimeoutException] {
                if (($QuietPeriodSeconds -gt 0) -and ($lines.Count -gt 0) -and [string]::IsNullOrEmpty($EndRegex)) {
                    $quietFor = ((Get-Date) - $lastLineTime).TotalSeconds
                    $minOk = ($MinTelemetryLines -eq 0) -or ($lineMatches -ge $MinTelemetryLines) -or ($lines.Count -ge $MinTelemetryLines)
                    if ($minOk -and ($quietFor -ge $QuietPeriodSeconds)) { break }
                }
            } catch [System.IO.IOException] {
                break
            }
        }
    } finally {
        if ($sp -and $sp.IsOpen) { $sp.Close() }
        if ($sp) { $sp.Dispose() }
    }
    [System.IO.File]::WriteAllLines($telemetryPath, $lines)
    $events | ConvertTo-Json -Depth 4 | Set-Content -Path ($telemetryPath -replace "\.txt$", "_timed.json")

    $serialStatus = "SUCCESS"
    if (-not $opened) {
        $serialStatus = "UPLOAD_OK_SERIAL_CAPTURE_FAILED"
    } elseif ($lines.Count -eq 0) {
        $serialStatus = "SERIAL_TIMEOUT"
    } elseif (-not $startSeen -or -not $endSeen) {
        $serialStatus = "PARTIAL_TELEMETRY"
    } elseif (($MinTelemetryLines -gt 0) -and (-not [string]::IsNullOrEmpty($LineRegex)) -and ($lineMatches -lt $MinTelemetryLines)) {
        $serialStatus = "PARTIAL_TELEMETRY"
    } elseif (($MinTelemetryLines -gt 0) -and ([string]::IsNullOrEmpty($LineRegex)) -and ($lines.Count -lt $MinTelemetryLines)) {
        $serialStatus = "PARTIAL_TELEMETRY"
    }
    return @{
        Status = $serialStatus
        Lines = $lines
        LineCount = $lines.Count
        StartSeen = $startSeen
        EndSeen = $endSeen
        LineMatches = $lineMatches
    }
}

"" | Set-Content -Path $logPath
$runStart = Get-Date
$sketchResolved = Resolve-Path $Sketch
$portsBefore = @(Get-DotNetPorts)
$boardListBefore = Get-BoardListText
Add-Log "BOARD=$Board"
Add-Log "PORT=$Port"
Add-Log "UPLOAD_PORT=$UploadPort"
Add-Log "FQBN=$Fqbn"
Add-Log "SKETCH=$sketchResolved"
Add-Log "PARSE_MODE=$ParseMode"
Add-Log "VISIBLE_PORTS_BEFORE=$($portsBefore -join ',')"
Add-Log "ARDUINO_BOARD_LIST_BEFORE_BEGIN`n$boardListBefore`nARDUINO_BOARD_LIST_BEFORE_END"

$uploadStatus = "SKIPPED"
$serialResult = $null
$attempt = 0
$finalStatus = "SERIAL_TIMEOUT"
$compileCommandText = ""

while ($attempt -le $RetryCount) {
    $attempt++
    Add-Log "ATTEMPT=$attempt"
    [GC]::Collect()
    Start-Sleep -Milliseconds 250
    $preOpenOk = Test-SerialOpen -SerialPort $Port -SerialBaud $Baud
    Add-Log "PRE_UPLOAD_SERIAL_OPEN_TEST=$preOpenOk"

    if (-not $SkipCompileUpload) {
        $extra = $ExtraFlags
        if ($UseBoardBenchmarkDefine) {
            $extra = ("-D{0} {1}" -f $cfg.BenchmarkDefine, $extra).Trim()
        }
        if (Test-Path $buildPath) {
            $resolvedBuild = Resolve-Path $buildPath
            $allowedRoot = Resolve-Path $buildsDir
            if ($resolvedBuild.Path.StartsWith($allowedRoot.Path)) {
                Remove-Item -LiteralPath $resolvedBuild.Path -Recurse -Force
            } else {
                throw "Refusing to remove unexpected build path: $resolvedBuild"
            }
        }
        $compileArgs = @(
            "compile",
            "--fqbn", $Fqbn,
            "--libraries", $Libraries,
            "--library", $repoRoot.Path,
            "--build-path", $buildPath
        )
        if ($extra) {
            if ($boardKey -eq "teensy41") {
                $compileArgs += @("--build-property", "build.flags.defs=-D__IMXRT1062__ -DTEENSYDUINO=160 $extra")
            } elseif ($cfg.ContainsKey("TeensyDefs")) {
                $compileArgs += @("--build-property", "build.flags.defs=$($cfg.TeensyDefs) $extra")
            } else {
                $compileArgs += @("--build-property", "compiler.cpp.extra_flags=$extra")
            }
        }
        if (-not $CompileOnly) {
            $compileArgs += @("--port", $UploadPort, "--upload")
        }
        $compileArgs += @($sketchResolved.Path)
        $compileCommandText = "$ArduinoCli " + ($compileArgs -join " ")
        Add-Log "COMPILE_UPLOAD_COMMAND=$compileCommandText"
        $oldErrorActionPreference = $ErrorActionPreference
        $ErrorActionPreference = "Continue"
        try {
            $compileOutput = & $ArduinoCli @compileArgs 2>&1
            $compileExit = $LASTEXITCODE
        } finally {
            $ErrorActionPreference = $oldErrorActionPreference
        }
        $compileOutput | ForEach-Object { Add-Log "ARDUINO: $_" }
        Add-Log "ARDUINO_EXIT_CODE=$compileExit"
        if ($compileExit -ne 0) {
            $uploadStatus = "UPLOAD_FAILED"
            $finalStatus = "UPLOAD_FAILED"
            break
        }
        $uploadStatus = if ($CompileOnly) { "COMPILE_OK" } else { "UPLOAD_OK" }
    }

    if ($CompileOnly) {
        $finalStatus = "SUCCESS"
        break
    }

    $portsAfterUpload = @(Get-DotNetPorts)
    Add-Log "VISIBLE_PORTS_AFTER_UPLOAD=$($portsAfterUpload -join ',')"
    if ($PostUploadDelaySeconds -gt 0) {
        Add-Log "POST_UPLOAD_DELAY_SECONDS=$PostUploadDelaySeconds"
        Start-Sleep -Seconds $PostUploadDelaySeconds
    }
    $wait = Wait-SerialVisible -SerialPort $Port -SerialBaud $Baud -Timeout $PortWaitSeconds
    Add-Log "WAIT_PORT_OK=$($wait.Ok) SEEN=$($wait.Seen) PORTS=$($wait.Ports -join ',')"
    if (-not $wait.Ok) {
        $finalStatus = "PORT_NOT_AVAILABLE"
        if ($attempt -le $RetryCount) { continue }
        break
    }
    $serialResult = Capture-Serial -Attempt $attempt
    Add-Log "CAPTURE_STATUS=$($serialResult.Status) LINES=$($serialResult.LineCount) START=$($serialResult.StartSeen) END=$($serialResult.EndSeen) MATCHES=$($serialResult.LineMatches)"
    $boardListAfterUpload = Get-BoardListText
    Add-Log "ARDUINO_BOARD_LIST_AFTER_UPLOAD_BEGIN`n$boardListAfterUpload`nARDUINO_BOARD_LIST_AFTER_UPLOAD_END"
    if ($serialResult.Status -eq "SUCCESS") {
        $finalStatus = "SUCCESS"
        break
    }
    $finalStatus = $serialResult.Status
    if ($attempt -le $RetryCount) {
        Add-Log "RETRY_AFTER_STATUS=$finalStatus"
        Start-Sleep -Seconds 2
    }
}

$parseInfo = @{ ParseSucceeded = $true; GlobalScores = 0; Subtests = 0; TelemetryRows = 0 }
if ($finalStatus -eq "SUCCESS" -and $serialResult) {
    if ($ParseMode -eq "benchmark") {
        $parseInfo = Parse-Benchmark -Lines $serialResult.Lines
        if (-not $parseInfo.ParseSucceeded) { $finalStatus = "PARSE_FAILED" }
    } elseif ($ParseMode -eq "example") {
        $parseInfo = Parse-Example -Lines $serialResult.Lines
        if (-not $parseInfo.ParseSucceeded) { $finalStatus = "PARSE_FAILED" }
    } elseif ($ParseMode -eq "probe") {
        $parseInfo = Parse-Example -Lines $serialResult.Lines
    }
}

$runEnd = Get-Date
$metadata = [ordered]@{
    board = $Board
    port = $Port
    upload_port = $UploadPort
    fqbn = $Fqbn
    sketch = $sketchResolved.Path
    command = $compileCommandText
    start_time = $runStart.ToString("o")
    end_time = $runEnd.ToString("o")
    upload_status = $uploadStatus
    serial_status = if ($serialResult) { $serialResult.Status } else { "" }
    run_status = $finalStatus
    attempts = $attempt
    telemetry_file = $telemetryPath
    log_file = $logPath
    metadata_file = $metadataPath
    captured_lines = if ($serialResult) { $serialResult.LineCount } else { 0 }
    start_marker_seen = if ($serialResult) { $serialResult.StartSeen } else { $false }
    completion_marker_seen = if ($serialResult) { $serialResult.EndSeen } else { $false }
    parse_mode = $ParseMode
    parse_succeeded = $parseInfo.ParseSucceeded
    parsed_global_scores = $parseInfo.GlobalScores
    parsed_subtests = $parseInfo.Subtests
    parsed_telemetry_rows = $parseInfo.TelemetryRows
    visible_ports_before = $portsBefore
    visible_ports_after = @(Get-DotNetPorts)
}
$metadata | ConvertTo-Json -Depth 6 | Set-Content -Path $metadataPath
Add-Log "FINAL_STATUS=$finalStatus"
Write-Host "[result] $finalStatus metadata=$metadataPath telemetry=$telemetryPath log=$logPath"

if ($finalStatus -ne "SUCCESS") {
    exit 2
}
