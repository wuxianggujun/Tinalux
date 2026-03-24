param(
    [Parameter(Mandatory = $true)]
    [string]$OutputRoot,
    [Parameter(Mandatory = $true)]
    [string]$ArtifactsRoot
)

$ErrorActionPreference = "Stop"

function Get-OptionalCommand {
    param([string]$CommandName)

    return Get-Command $CommandName -ErrorAction SilentlyContinue
}

function Get-ToolVersionLine {
    param(
        [string]$CommandName,
        [string[]]$Arguments = @()
    )

    $command = Get-OptionalCommand -CommandName $CommandName
    if (-not $command) {
        return $null
    }

    $output = & $command.Source @Arguments 2>&1
    $firstLine = $output | Where-Object { -not [string]::IsNullOrWhiteSpace($_) } | Select-Object -First 1
    if (-not $firstLine) {
        return $null
    }

    return $firstLine.ToString().Trim()
}

function Get-PythonToolInfo {
    $pyCommand = Get-OptionalCommand -CommandName "py"
    if ($pyCommand) {
        $versionLine = Get-ToolVersionLine -CommandName "py" -Arguments @("-3", "--version")
        if ($versionLine) {
            return [ordered]@{
                command = "py -3"
                path = $pyCommand.Source
                version = $versionLine
            }
        }
    }

    $pythonCommand = Get-OptionalCommand -CommandName "python"
    if ($pythonCommand) {
        return [ordered]@{
            command = "python"
            path = $pythonCommand.Source
            version = Get-ToolVersionLine -CommandName "python" -Arguments @("--version")
        }
    }

    return $null
}

$outputRootPath = [System.IO.Path]::GetFullPath($OutputRoot)
$artifactsRootPath = [System.IO.Path]::GetFullPath($ArtifactsRoot)
New-Item -ItemType Directory -Force -Path $outputRootPath | Out-Null
New-Item -ItemType Directory -Force -Path $artifactsRootPath | Out-Null

$pythonInfo = Get-PythonToolInfo
$cmakeCommand = Get-OptionalCommand -CommandName "cmake"

$runnerFingerprint = [ordered]@{
    generatedAtUtc = (Get-Date).ToUniversalTime().ToString("o")
    workflow = [ordered]@{
        name = "android-build-scripts-smoke"
        artifactsRoot = $artifactsRootPath
        metadataRoot = $outputRootPath
    }
    runner = [ordered]@{
        os = $env:RUNNER_OS
        arch = $env:RUNNER_ARCH
        name = $env:RUNNER_NAME
        imageOs = $env:ImageOS
        imageVersion = $env:ImageVersion
    }
    tools = [ordered]@{
        powershell = [ordered]@{
            edition = $PSVersionTable.PSEdition
            version = $PSVersionTable.PSVersion.ToString()
            path = (Get-Process -Id $PID).Path
        }
        cmake = [ordered]@{
            path = if ($cmakeCommand) { $cmakeCommand.Source } else { $null }
            version = Get-ToolVersionLine -CommandName "cmake" -Arguments @("--version")
        }
        python = $pythonInfo
    }
}

$runnerFingerprintPath = Join-Path $outputRootPath "runner-fingerprint.json"
$workflowSummaryPath = Join-Path $outputRootPath "workflow-summary.md"
$runnerFingerprint | ConvertTo-Json -Depth 6 | Set-Content -Path $runnerFingerprintPath

$runnerLabel = if (-not [string]::IsNullOrWhiteSpace($env:ImageOS)) {
    $label = $env:ImageOS
    if (-not [string]::IsNullOrWhiteSpace($env:ImageVersion)) {
        $label = "$label / $($env:ImageVersion)"
    }
    $label
} elseif (-not [string]::IsNullOrWhiteSpace($env:RUNNER_OS)) {
    $env:RUNNER_OS
} else {
    "unknown"
}

$markdown = @(
    "## Android Build Scripts Smoke 元数据"
    ""
    ('- Runner：{0}' -f $runnerLabel)
    ('- Artifacts root：`{0}`' -f $artifactsRootPath)
    ('- PowerShell：{0}' -f $runnerFingerprint.tools.powershell.version)
    ('- CMake：{0}' -f $runnerFingerprint.tools.cmake.version)
    ('- Python：{0}' -f $runnerFingerprint.tools.python.version)
    ""
    "元数据文件："
    '- `runner-fingerprint.json`'
) -join [Environment]::NewLine

Set-Content -Path $workflowSummaryPath -Value $markdown

if (-not [string]::IsNullOrWhiteSpace($env:GITHUB_STEP_SUMMARY)) {
    Add-Content -Path $env:GITHUB_STEP_SUMMARY -Value $markdown
}

Write-Host "Captured Android build scripts smoke metadata:"
Write-Host "  $runnerFingerprintPath"
Write-Host "  $workflowSummaryPath"
