param(
    [Parameter(Mandatory = $true)]
    [ValidateSet("configure", "build", "test")]
    [string]$Stage,
    [Parameter(Mandatory = $true)]
    [string]$BuildDir,
    [Parameter(Mandatory = $true)]
    [string]$OutputRoot,
    [string]$Config = "Debug"
)

$ErrorActionPreference = "Stop"

function Get-StageDefinition {
    param(
        [string]$StageName,
        [string]$RepoRoot,
        [string]$ResolvedBuildDir,
        [string]$BuildConfig
    )

    switch ($StageName) {
        "configure" {
            return [ordered]@{
                label = "Configure Desktop Smoke Build"
                command = "cmake"
                arguments = @(
                    "-S", $RepoRoot,
                    "-B", $ResolvedBuildDir,
                    "-G", "Ninja",
                    "-DCMAKE_BUILD_TYPE=$BuildConfig",
                    "-DTINALUX_BUILD_TESTS=ON",
                    "-DTINALUX_BUILD_SAMPLES=OFF"
                )
            }
        }
        "build" {
            return [ordered]@{
                label = "Build Desktop Smoke Tests"
                command = (Join-Path $RepoRoot "scripts/runSmokeTests.ps1")
                parameters = @{
                    BuildDir = $ResolvedBuildDir
                    Config = $BuildConfig
                    Phase = "build"
                }
            }
        }
        "test" {
            return [ordered]@{
                label = "Run Desktop Smoke Tests"
                command = (Join-Path $RepoRoot "scripts/runSmokeTests.ps1")
                parameters = @{
                    BuildDir = $ResolvedBuildDir
                    Config = $BuildConfig
                    Phase = "test"
                }
            }
        }
        default {
            throw "Unsupported stage: $StageName"
        }
    }
}

$repoRootPath = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot "../.."))
$buildDirPath = if ([System.IO.Path]::IsPathRooted($BuildDir)) {
    [System.IO.Path]::GetFullPath($BuildDir)
} else {
    [System.IO.Path]::GetFullPath((Join-Path $repoRootPath $BuildDir))
}
$outputRootPath = [System.IO.Path]::GetFullPath($OutputRoot)
New-Item -ItemType Directory -Force -Path $outputRootPath | Out-Null

$stageDefinition = Get-StageDefinition `
    -StageName $Stage `
    -RepoRoot $repoRootPath `
    -ResolvedBuildDir $buildDirPath `
    -BuildConfig $Config

$startedAt = Get-Date
$startedAtUtc = $startedAt.ToUniversalTime().ToString("o")
$finishedAtUtc = $null
$durationSeconds = 0
$exitCode = 0
$status = "succeeded"
$errorMessage = $null

try {
    if ($null -ne $stageDefinition.parameters) {
        $commandParameters = $stageDefinition.parameters
        & $stageDefinition.command @commandParameters
    } else {
        $commandArguments = $stageDefinition.arguments
        & $stageDefinition.command @commandArguments
    }
    if ($null -ne $LASTEXITCODE) {
        $exitCode = [int]$LASTEXITCODE
    }
} catch {
    $exitCode = 1
    $status = "failed"
    $errorMessage = $_.Exception.Message
} finally {
    $finishedAt = Get-Date
    $finishedAtUtc = $finishedAt.ToUniversalTime().ToString("o")
    $durationSeconds = [Math]::Round(($finishedAt - $startedAt).TotalSeconds, 2)

    if ($exitCode -ne 0) {
        $status = "failed"
    }

    $payload = [ordered]@{
        stage = $Stage
        label = $stageDefinition.label
        buildDir = $buildDirPath
        config = $Config
        command = $stageDefinition.command
        startedAtUtc = $startedAtUtc
        finishedAtUtc = $finishedAtUtc
        durationSeconds = $durationSeconds
        exitCode = $exitCode
        status = $status
    }

    if ($null -ne $stageDefinition.arguments) {
        $payload.arguments = $stageDefinition.arguments
    }

    if ($null -ne $stageDefinition.parameters) {
        $payload.parameters = $stageDefinition.parameters
    }

    if (-not [string]::IsNullOrWhiteSpace($errorMessage)) {
        $payload.errorMessage = $errorMessage
    }

    $payloadPath = Join-Path $outputRootPath ("stage-{0}.json" -f $Stage)
    $payload | ConvertTo-Json -Depth 6 | Set-Content -Path $payloadPath

    Write-Host "Captured Windows desktop smoke stage:"
    Write-Host "  stage=$Stage"
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
