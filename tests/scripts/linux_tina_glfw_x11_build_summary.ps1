param(
    [Parameter(Mandatory = $true)]
    [string]$OutputRoot,
    [Parameter(Mandatory = $true)]
    [string]$BuildDir,
    [string]$Compiler = ""
)

$ErrorActionPreference = "Stop"

function Format-ByteSize {
    param([double]$Bytes)

    $units = @("B", "KB", "MB", "GB", "TB")
    $value = $Bytes
    $unitIndex = 0
    while ($value -ge 1024 -and $unitIndex -lt ($units.Count - 1)) {
        $value /= 1024
        $unitIndex += 1
    }

    return "{0:N2} {1}" -f $value, $units[$unitIndex]
}

$outputRootPath = [System.IO.Path]::GetFullPath($OutputRoot)
$buildDirPath = if ([System.IO.Path]::IsPathRooted($BuildDir)) {
    [System.IO.Path]::GetFullPath($BuildDir)
} else {
    [System.IO.Path]::GetFullPath((Join-Path (Get-Location) $BuildDir))
}
New-Item -ItemType Directory -Force -Path $outputRootPath | Out-Null

$buildRunPath = Join-Path $outputRootPath "build-run.json"
$buildRun = if (Test-Path $buildRunPath) {
    Get-Content -Path $buildRunPath | ConvertFrom-Json
} else {
    [pscustomobject]@{
        compiler = $Compiler
        buildDir = $buildDirPath
        status = "missing"
        message = "No Linux X11 build metadata was produced."
    }
}

$fileCount = 0
$totalBytes = 0
if (Test-Path $buildDirPath) {
    $files = @(Get-ChildItem -Path $buildDirPath -File -Recurse -ErrorAction SilentlyContinue)
    $fileCount = $files.Count
    $totalBytes = if ($fileCount -gt 0) {
        ($files | Measure-Object -Property Length -Sum).Sum
    } else {
        0
    }
}

$payload = [ordered]@{
    generatedAtUtc = (Get-Date).ToUniversalTime().ToString("o")
    compiler = if ($buildRun.compiler) { $buildRun.compiler } else { $Compiler }
    buildDir = if ($buildRun.buildDir) { $buildRun.buildDir } else { $buildDirPath }
    status = $buildRun.status
    durationSeconds = if ($null -ne $buildRun.durationSeconds) { $buildRun.durationSeconds } else { $null }
    exitCode = if ($null -ne $buildRun.exitCode) { $buildRun.exitCode } else { $null }
    buildDirectory = [ordered]@{
        exists = (Test-Path $buildDirPath)
        fileCount = $fileCount
        totalBytes = $totalBytes
    }
}

if ($buildRun.PSObject.Properties.Name -contains "errorMessage") {
    $payload.errorMessage = $buildRun.errorMessage
}

$summaryLines = @(
    "## Linux X11 构建摘要"
    ""
    ('- Compiler：`{0}`' -f $payload.compiler)
    ('- BuildDir：`{0}`' -f $payload.buildDir)
    ('- 状态：`{0}`' -f $payload.status)
)

if ($null -ne $payload.durationSeconds) {
    $summaryLines += ('- 构建耗时：`{0} sec`' -f $payload.durationSeconds)
}

if ($null -ne $payload.exitCode) {
    $summaryLines += ('- ExitCode：`{0}`' -f $payload.exitCode)
}

$summaryLines += ('- 构建目录文件数：`{0}`' -f $payload.buildDirectory.fileCount)
$summaryLines += ('- 构建目录体积：`{0}`' -f (Format-ByteSize -Bytes $payload.buildDirectory.totalBytes))

if ($payload.PSObject.Properties.Name -contains "errorMessage") {
    $summaryLines += ('- 错误：`{0}`' -f $payload.errorMessage)
}

$summaryMarkdown = $summaryLines -join [Environment]::NewLine
$executionSummaryJsonPath = Join-Path $outputRootPath "execution-summary.json"
$executionSummaryMarkdownPath = Join-Path $outputRootPath "execution-summary.md"

$payload | ConvertTo-Json -Depth 6 | Set-Content -Path $executionSummaryJsonPath
Set-Content -Path $executionSummaryMarkdownPath -Value $summaryMarkdown

if (-not [string]::IsNullOrWhiteSpace($env:GITHUB_STEP_SUMMARY)) {
    Add-Content -Path $env:GITHUB_STEP_SUMMARY -Value $summaryMarkdown
}

Write-Host "Captured Linux X11 execution summary:"
Write-Host "  $executionSummaryJsonPath"
Write-Host "  $executionSummaryMarkdownPath"
