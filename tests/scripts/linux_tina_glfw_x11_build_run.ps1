param(
    [Parameter(Mandatory = $true)]
    [string]$RepoRoot,
    [Parameter(Mandatory = $true)]
    [string]$BuildDir,
    [Parameter(Mandatory = $true)]
    [string]$OutputRoot,
    [string]$Compiler = "",
    [string]$BuildType = "Debug"
)

$ErrorActionPreference = "Stop"

$repoRootPath = [System.IO.Path]::GetFullPath((Resolve-Path $RepoRoot).Path)
$buildDirPath = if ([System.IO.Path]::IsPathRooted($BuildDir)) {
    [System.IO.Path]::GetFullPath($BuildDir)
} else {
    [System.IO.Path]::GetFullPath((Join-Path $repoRootPath $BuildDir))
}
$outputRootPath = [System.IO.Path]::GetFullPath($OutputRoot)
New-Item -ItemType Directory -Force -Path $outputRootPath | Out-Null

$validateScriptPath = Join-Path $repoRootPath "scripts/validateTinaGlfwX11Linux.sh"
if (-not (Test-Path $validateScriptPath)) {
    throw "Linux X11 validation script not found: $validateScriptPath"
}

$startedAt = Get-Date
$startedAtUtc = $startedAt.ToUniversalTime().ToString("o")
$finishedAtUtc = $null
$durationSeconds = 0
$exitCode = 0
$status = "succeeded"
$errorMessage = $null

$previousBuildDir = $env:BUILD_DIR
$previousBuildType = $env:BUILD_TYPE

try {
    $env:BUILD_DIR = $buildDirPath
    $env:BUILD_TYPE = $BuildType
    & bash $validateScriptPath
    if ($null -ne $LASTEXITCODE) {
        $exitCode = [int]$LASTEXITCODE
    }
} catch {
    $exitCode = 1
    $status = "failed"
    $errorMessage = $_.Exception.Message
} finally {
    if ($null -ne $previousBuildDir) {
        $env:BUILD_DIR = $previousBuildDir
    } else {
        Remove-Item Env:BUILD_DIR -ErrorAction SilentlyContinue
    }

    if ($null -ne $previousBuildType) {
        $env:BUILD_TYPE = $previousBuildType
    } else {
        Remove-Item Env:BUILD_TYPE -ErrorAction SilentlyContinue
    }

    $finishedAt = Get-Date
    $finishedAtUtc = $finishedAt.ToUniversalTime().ToString("o")
    $durationSeconds = [Math]::Round(($finishedAt - $startedAt).TotalSeconds, 2)

    if ($exitCode -ne 0) {
        $status = "failed"
    }

    $payload = [ordered]@{
        compiler = $Compiler
        buildType = $BuildType
        buildDir = $buildDirPath
        validateScript = $validateScriptPath
        cc = $env:CC
        cxx = $env:CXX
        startedAtUtc = $startedAtUtc
        finishedAtUtc = $finishedAtUtc
        durationSeconds = $durationSeconds
        exitCode = $exitCode
        status = $status
    }

    if (-not [string]::IsNullOrWhiteSpace($errorMessage)) {
        $payload.errorMessage = $errorMessage
    }

    $payloadPath = Join-Path $outputRootPath "build-run.json"
    $payload | ConvertTo-Json -Depth 6 | Set-Content -Path $payloadPath

    Write-Host "Captured Linux X11 build run:"
    Write-Host "  compiler=$Compiler"
    Write-Host "  duration=${durationSeconds}s"
    Write-Host "  status=$status"
    Write-Host "  $payloadPath"
}

if (-not [string]::IsNullOrWhiteSpace($errorMessage)) {
    Write-Error $errorMessage
}

if ($exitCode -ne 0) {
    exit $exitCode
}
