$ErrorActionPreference = 'Stop'

$exePath = Join-Path $PSScriptRoot '..\cmake-build-debug\Tinalux.exe'
$exePath = [System.IO.Path]::GetFullPath($exePath)

$lldbCommand = $null
if (-not [string]::IsNullOrWhiteSpace($env:TINALUX_LLDB)) {
    $lldbCommand = Get-Command $env:TINALUX_LLDB -ErrorAction SilentlyContinue
}
if ($lldbCommand -eq $null) {
    $lldbCommand = Get-Command 'lldb.exe' -ErrorAction SilentlyContinue
}
if ($lldbCommand -eq $null) {
    $lldbCommand = Get-Command 'lldb' -ErrorAction SilentlyContinue
}

if (-not (Test-Path $exePath)) {
    throw "Executable not found: $exePath"
}

if ($lldbCommand -eq $null) {
    throw 'LLDB not found. Set TINALUX_LLDB or add lldb to PATH.'
}

$lldbPath = $lldbCommand.Source

$closeJob = Start-Job -ScriptBlock {
    Start-Sleep -Seconds 4
    $process = Get-Process Tinalux -ErrorAction SilentlyContinue |
        Select-Object -First 1
    if ($process -ne $null) {
        $null = $process.CloseMainWindow()
    }
}

try {
    & $lldbPath `
        -b `
        -o 'break set -n abort' `
        -o 'run' `
        -o 'bt' `
        -o 'quit' `
        -- `
        $exePath
} finally {
    Receive-Job $closeJob -Wait -AutoRemoveJob | Out-Null
}
