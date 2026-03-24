param(
    [Parameter(Mandatory = $true)]
    [ValidateSet("stage", "validate")]
    [string]$Stage,
    [Parameter(Mandatory = $true)]
    [string]$RepoRoot,
    [Parameter(Mandatory = $true)]
    [string]$OutputRoot
)

$ErrorActionPreference = "Stop"

function Get-StageDefinition {
    param(
        [string]$StageName,
        [string]$RepoRootPath
    )

    switch ($StageName) {
        "stage" {
            return [ordered]@{
                label = "Run Android Stage Script Smoke"
                script = (Join-Path $RepoRootPath "tests/scripts/android_stage_script_smoke.ps1")
                parameters = @{
                    RepoRoot = $RepoRootPath
                }
            }
        }
        "validate" {
            return [ordered]@{
                label = "Run Android Build Validate Smoke"
                script = (Join-Path $RepoRootPath "tests/scripts/android_build_validate_smoke.ps1")
                parameters = @{
                    RepoRoot = $RepoRootPath
                }
            }
        }
        default {
            throw "Unsupported stage: $StageName"
        }
    }
}

$repoRootPath = [System.IO.Path]::GetFullPath((Resolve-Path $RepoRoot).Path)
$outputRootPath = [System.IO.Path]::GetFullPath($OutputRoot)
New-Item -ItemType Directory -Force -Path $outputRootPath | Out-Null

$stageDefinition = Get-StageDefinition -StageName $Stage -RepoRootPath $repoRootPath

$startedAt = Get-Date
$startedAtUtc = $startedAt.ToUniversalTime().ToString("o")
$finishedAtUtc = $null
$durationSeconds = 0
$exitCode = 0
$status = "succeeded"
$errorMessage = $null

try {
    $commandParameters = $stageDefinition.parameters
    & $stageDefinition.script @commandParameters
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
        repoRoot = $repoRootPath
        script = $stageDefinition.script
        parameters = $stageDefinition.parameters
        startedAtUtc = $startedAtUtc
        finishedAtUtc = $finishedAtUtc
        durationSeconds = $durationSeconds
        exitCode = $exitCode
        status = $status
    }

    if (-not [string]::IsNullOrWhiteSpace($errorMessage)) {
        $payload.errorMessage = $errorMessage
    }

    $payloadPath = Join-Path $outputRootPath ("script-{0}.json" -f $Stage)
    $payload | ConvertTo-Json -Depth 6 | Set-Content -Path $payloadPath

    Write-Host "Captured Android build scripts smoke stage:"
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
