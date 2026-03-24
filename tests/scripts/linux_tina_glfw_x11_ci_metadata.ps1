param(
    [Parameter(Mandatory = $true)]
    [string]$OutputRoot,
    [Parameter(Mandatory = $true)]
    [string]$BuildDir,
    [string]$Compiler = ""
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "ci_metadata_manifest_helpers.ps1")

function Get-OptionalCommand {
    param([string]$CommandName)

    return Get-Command $CommandName -ErrorAction SilentlyContinue
}

function Get-ExecutableVersionLine {
    param(
        [string]$Executable,
        [string[]]$Arguments = @("--version")
    )

    if ([string]::IsNullOrWhiteSpace($Executable)) {
        return $null
    }

    try {
        $output = & $Executable @Arguments 2>&1
    } catch {
        return $null
    }

    $firstLine = $output | Where-Object { -not [string]::IsNullOrWhiteSpace($_) } | Select-Object -First 1
    if (-not $firstLine) {
        return $null
    }

    return $firstLine.ToString().Trim()
}

function Get-PythonToolInfo {
    $pythonCommand = Get-OptionalCommand -CommandName "python3"
    if ($pythonCommand) {
        return [ordered]@{
            command = "python3"
            path = $pythonCommand.Source
            version = Get-ExecutableVersionLine -Executable $pythonCommand.Source
        }
    }

    return $null
}

function Format-OptionalValue {
    param([object]$Value)

    if ($null -eq $Value) {
        return "unavailable"
    }

    $text = $Value.ToString().Trim()
    if ([string]::IsNullOrWhiteSpace($text)) {
        return "unavailable"
    }

    return $text
}

$outputRootPath = [System.IO.Path]::GetFullPath($OutputRoot)
$buildDirPath = if ([System.IO.Path]::IsPathRooted($BuildDir)) {
    [System.IO.Path]::GetFullPath($BuildDir)
} else {
    [System.IO.Path]::GetFullPath((Join-Path (Get-Location) $BuildDir))
}
New-Item -ItemType Directory -Force -Path $outputRootPath | Out-Null

$cmakeCommand = Get-OptionalCommand -CommandName "cmake"
$ninjaCommand = Get-OptionalCommand -CommandName "ninja"
$pythonInfo = Get-PythonToolInfo

$runnerFingerprint = [ordered]@{
    schemaVersion = 1
    generatedAtUtc = (Get-Date).ToUniversalTime().ToString("o")
    workflow = [ordered]@{
        name = "linux-tina-glfw-x11"
        metadataRoot = $outputRootPath
        compiler = $Compiler
        buildDir = $buildDirPath
        cc = $env:CC
        cxx = $env:CXX
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
            version = if ($cmakeCommand) { Get-ExecutableVersionLine -Executable $cmakeCommand.Source } else { $null }
        }
        ninja = [ordered]@{
            path = if ($ninjaCommand) { $ninjaCommand.Source } else { $null }
            version = if ($ninjaCommand) { Get-ExecutableVersionLine -Executable $ninjaCommand.Source } else { $null }
        }
        cc = [ordered]@{
            command = $env:CC
            version = Get-ExecutableVersionLine -Executable $env:CC
        }
        cxx = [ordered]@{
            command = $env:CXX
            version = Get-ExecutableVersionLine -Executable $env:CXX
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
    "## Linux X11 构建元数据"
    ""
    ('- Compiler：`{0}`' -f (Format-OptionalValue -Value $Compiler))
    ('- Runner：{0}' -f $runnerLabel)
    ('- BuildDir：`{0}`' -f $buildDirPath)
    ('- CC：{0}' -f (Format-OptionalValue -Value $runnerFingerprint.tools.cc.version))
    ('- CXX：{0}' -f (Format-OptionalValue -Value $runnerFingerprint.tools.cxx.version))
    ('- CMake：{0}' -f (Format-OptionalValue -Value $runnerFingerprint.tools.cmake.version))
    ('- Ninja：{0}' -f (Format-OptionalValue -Value $runnerFingerprint.tools.ninja.version))
    ""
    "元数据文件："
    '- `metadata-manifest.json`'
    '- `runner-fingerprint.json`'
) -join [Environment]::NewLine

Set-Content -Path $workflowSummaryPath -Value $markdown

$metadataManifestPath = Update-MetadataManifest `
    -OutputRoot $outputRootPath `
    -WorkflowName "linux-tina-glfw-x11" `
    -WorkflowData @{
        buildDir = $buildDirPath
        cc = $env:CC
        compiler = $Compiler
        cxx = $env:CXX
    } `
    -Entries @(
        (New-MetadataManifestEntry -OutputRoot $outputRootPath -Id "runnerFingerprint" -Path "runner-fingerprint.json" -Format "json" -Role "runner-fingerprint"),
        (New-MetadataManifestEntry -OutputRoot $outputRootPath -Id "workflowSummary" -Path "workflow-summary.md" -Format "markdown" -Role "workflow-summary")
    )

if (-not [string]::IsNullOrWhiteSpace($env:GITHUB_STEP_SUMMARY)) {
    Add-Content -Path $env:GITHUB_STEP_SUMMARY -Value $markdown
}

Write-Host "Captured Linux X11 build metadata:"
Write-Host "  $runnerFingerprintPath"
Write-Host "  $workflowSummaryPath"
Write-Host "  $metadataManifestPath"
