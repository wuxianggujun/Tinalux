$ErrorActionPreference = 'Stop'

$exePath = Join-Path $PSScriptRoot '..\cmake-build-debug\Tinalux.exe'
$exePath = [System.IO.Path]::GetFullPath($exePath)
$tracePath = Join-Path $PSScriptRoot '..\runtime_abort_trace.txt'
$tracePath = [System.IO.Path]::GetFullPath($tracePath)
$crtReportPath = Join-Path $PSScriptRoot '..\debug_crt_report.txt'
$crtReportPath = [System.IO.Path]::GetFullPath($crtReportPath)
$heapTracePath = Join-Path $PSScriptRoot '..\debug_heap_trace.txt'
$heapTracePath = [System.IO.Path]::GetFullPath($heapTracePath)

if (-not (Test-Path $exePath)) {
    throw "Executable not found: $exePath"
}

Set-Content -Path $tracePath -Value ''
Set-Content -Path $crtReportPath -Value ''
Set-Content -Path $heapTracePath -Value ''

$process = Start-Process -FilePath $exePath -PassThru
Start-Sleep -Seconds 4
$null = $process.CloseMainWindow()

for ($i = 0; $i -lt 20; $i++) {
    Start-Sleep -Seconds 1
    $process.Refresh()
    if ($process.HasExited) {
        break
    }
}

if ($process.HasExited) {
    "EXIT:$($process.ExitCode)"
} else {
    $process.Refresh()
    "RUNNING:$($process.Id)"
    "RESPONDING:$($process.Responding)"
    "MAIN_WINDOW_TITLE:$($process.MainWindowTitle)"
}

'---TRACE---'
if (Test-Path $tracePath) {
    Get-Content $tracePath
}

'---CRT-REPORT---'
if (Test-Path $crtReportPath) {
    Get-Content $crtReportPath
}

'---HEAP-TRACE---'
if (Test-Path $heapTracePath) {
    Get-Content $heapTracePath
}
