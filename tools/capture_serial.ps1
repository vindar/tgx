param(
    [string]$Port = "COM3",
    [int]$Baud = 9600,
    [double]$ProbeSeconds = 3.0,
    [double]$RetryDelaySeconds = 1.0,
    [double]$CaptureSeconds = 3.0,
    [double]$MaxWaitSeconds = 120.0
)

$deadline = (Get-Date).AddSeconds($MaxWaitSeconds)

while ((Get-Date) -lt $deadline) {
    $serial = $null
    try {
        $serial = New-Object System.IO.Ports.SerialPort $Port, $Baud, None, 8, one
        $serial.ReadTimeout = 100
        $serial.WriteTimeout = 100
        $serial.Open()

        $probeDeadline = (Get-Date).AddSeconds($ProbeSeconds)
        $captureStart = $null
        $buffer = New-Object System.Text.StringBuilder

        while ((Get-Date) -lt $probeDeadline) {
            $chunk = $serial.ReadExisting()
            if ($chunk.Length -gt 0) {
                [void]$buffer.Append($chunk)
                $captureStart = Get-Date
                break
            }
            Start-Sleep -Milliseconds 50
        }

        if ($null -eq $captureStart) {
            $serial.Close()
            Start-Sleep -Seconds $RetryDelaySeconds
            continue
        }

        $captureDeadline = $captureStart.AddSeconds($CaptureSeconds)
        while ((Get-Date) -lt $captureDeadline) {
            $chunk = $serial.ReadExisting()
            if ($chunk.Length -gt 0) {
                [void]$buffer.Append($chunk)
            }
            Start-Sleep -Milliseconds 50
        }

        $serial.Close()
        $buffer.ToString()
        exit 0
    }
    catch {
        if ($null -ne $serial -and $serial.IsOpen) {
            $serial.Close()
        }
        Start-Sleep -Seconds $RetryDelaySeconds
    }
}

Write-Error "No serial data received on $Port within $MaxWaitSeconds seconds."
exit 1
