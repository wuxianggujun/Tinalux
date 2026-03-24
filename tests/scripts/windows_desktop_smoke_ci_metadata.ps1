param(
    [Parameter(Mandatory = $true)]
    [string]$OutputRoot,
    [string]$SkiaRevision = "",
    [string]$SourceCachePrimaryKey = "",
    [string]$SourceCacheMatchedKey = "",
    [string]$SourceCacheHit = "",
    [string]$SourceCacheExpectedKey = "",
    [string]$BuildCachePrimaryKey = "",
    [string]$BuildCacheMatchedKey = "",
    [string]$BuildCacheHit = "",
    [string]$BuildCacheExpectedKey = ""
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

function Resolve-CacheState {
    param(
        [string]$CacheHit,
        [string]$MatchedKey
    )

    if ($CacheHit -eq "true") {
        return "exact-hit"
    }

    if (-not [string]::IsNullOrWhiteSpace($MatchedKey)) {
        return "warm-restore"
    }

    return "miss"
}

function Resolve-CacheSavePolicy {
    param(
        [string]$ExpectedKey,
        [string]$MatchedKey,
        [string]$CacheHit
    )

    if ([string]::IsNullOrWhiteSpace($ExpectedKey)) {
        return "unknown"
    }

    if ($CacheHit -eq "true" -or $MatchedKey -eq $ExpectedKey) {
        return "skip-save"
    }

    return "save-exact-key"
}

function Format-CacheSummaryLine {
    param([hashtable]$Cache)

    $parts = @("状态=$($Cache.state)")

    if (-not [string]::IsNullOrWhiteSpace($Cache.matchedKey)) {
        $parts += "matchedKey=$($Cache.matchedKey)"
    }

    if (-not [string]::IsNullOrWhiteSpace($Cache.expectedKey)) {
        $parts += "expectedKey=$($Cache.expectedKey)"
    }

    $parts += "savePolicy=$($Cache.savePolicy)"
    return ($parts -join "；")
}

$outputRootPath = [System.IO.Path]::GetFullPath($OutputRoot)
New-Item -ItemType Directory -Force -Path $outputRootPath | Out-Null

$pythonInfo = Get-PythonToolInfo
$cmakeCommand = Get-OptionalCommand -CommandName "cmake"
$ninjaCommand = Get-OptionalCommand -CommandName "ninja"
$clCommand = Get-OptionalCommand -CommandName "cl"

$runnerFingerprint = [ordered]@{
    schemaVersion = 1
    generatedAtUtc = (Get-Date).ToUniversalTime().ToString("o")
    workflow = [ordered]@{
        name = "windows-desktop-smoke"
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
        ninja = [ordered]@{
            path = if ($ninjaCommand) { $ninjaCommand.Source } else { $null }
            version = Get-ToolVersionLine -CommandName "ninja" -Arguments @("--version")
        }
        python = $pythonInfo
        msvc = [ordered]@{
            path = if ($clCommand) { $clCommand.Source } else { $null }
            version = Get-ToolVersionLine -CommandName "cl"
        }
    }
}

$cacheSummary = [ordered]@{
    generatedAtUtc = $runnerFingerprint.generatedAtUtc
    skiaRevision = $SkiaRevision
    source = [ordered]@{
        state = Resolve-CacheState -CacheHit $SourceCacheHit -MatchedKey $SourceCacheMatchedKey
        exactHit = $SourceCacheHit
        primaryKey = $SourceCachePrimaryKey
        matchedKey = $SourceCacheMatchedKey
        expectedKey = $SourceCacheExpectedKey
        savePolicy = Resolve-CacheSavePolicy -ExpectedKey $SourceCacheExpectedKey -MatchedKey $SourceCacheMatchedKey -CacheHit $SourceCacheHit
    }
    build = [ordered]@{
        state = Resolve-CacheState -CacheHit $BuildCacheHit -MatchedKey $BuildCacheMatchedKey
        exactHit = $BuildCacheHit
        primaryKey = $BuildCachePrimaryKey
        matchedKey = $BuildCacheMatchedKey
        expectedKey = $BuildCacheExpectedKey
        savePolicy = Resolve-CacheSavePolicy -ExpectedKey $BuildCacheExpectedKey -MatchedKey $BuildCacheMatchedKey -CacheHit $BuildCacheHit
    }
}

$runnerFingerprintPath = Join-Path $outputRootPath "runner-fingerprint.json"
$cacheSummaryPath = Join-Path $outputRootPath "cache-summary.json"
$workflowSummaryPath = Join-Path $outputRootPath "workflow-summary.md"

$runnerFingerprint | ConvertTo-Json -Depth 6 | Set-Content -Path $runnerFingerprintPath
$cacheSummary | ConvertTo-Json -Depth 6 | Set-Content -Path $cacheSummaryPath

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
    "## Windows Desktop Smoke 元数据"
    ""
    "- Skia revision：$SkiaRevision"
    "- Source cache：$(Format-CacheSummaryLine -Cache $cacheSummary.source)"
    "- Build cache：$(Format-CacheSummaryLine -Cache $cacheSummary.build)"
    "- Runner：$runnerLabel"
    "- PowerShell：$($runnerFingerprint.tools.powershell.version)"
    "- CMake：$($runnerFingerprint.tools.cmake.version)"
    "- Ninja：$($runnerFingerprint.tools.ninja.version)"
    "- Python：$($runnerFingerprint.tools.python.version)"
    "- MSVC：$($runnerFingerprint.tools.msvc.version)"
    ""
    "元数据文件："
    '- `runner-fingerprint.json`'
    '- `cache-summary.json`'
) -join [Environment]::NewLine

Set-Content -Path $workflowSummaryPath -Value $markdown

if (-not [string]::IsNullOrWhiteSpace($env:GITHUB_STEP_SUMMARY)) {
    Add-Content -Path $env:GITHUB_STEP_SUMMARY -Value $markdown
}

Write-Host "Captured Windows desktop smoke metadata:"
Write-Host "  $runnerFingerprintPath"
Write-Host "  $cacheSummaryPath"
Write-Host "  $workflowSummaryPath"
