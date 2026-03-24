param(
    [Parameter(Mandatory = $true)]
    [string]$OutputRoot,
    [Parameter(Mandatory = $true)]
    [string]$ArtifactsRoot
)

$ErrorActionPreference = "Stop"

function New-MissingStageRecord {
    param(
        [string]$StageName,
        [string]$StageLabel
    )

    return [pscustomobject]@{
        stage = $StageName
        label = $StageLabel
        status = "missing"
        message = "No stage metadata was produced."
    }
}

function Resolve-ExecutionStatus {
    param(
        [int]$FailedSteps,
        [int]$MissingSteps,
        [int]$CompletedSteps
    )

    if ($FailedSteps -gt 0) {
        return "failed"
    }

    if ($MissingSteps -gt 0 -or $CompletedSteps -eq 0) {
        return "missing"
    }

    return "succeeded"
}

function ConvertTo-ExecutionStep {
    param([psobject]$Record)

    $step = [ordered]@{
        name = $Record.stage
        label = $Record.label
        status = $Record.status
    }

    if ($Record.PSObject.Properties.Name -contains "durationSeconds") {
        $step.durationSeconds = $Record.durationSeconds
    }

    if ($Record.PSObject.Properties.Name -contains "exitCode") {
        $step.exitCode = $Record.exitCode
    }

    if ($Record.PSObject.Properties.Name -contains "message") {
        $step.message = $Record.message
    }

    if ($Record.PSObject.Properties.Name -contains "errorMessage") {
        $step.errorMessage = $Record.errorMessage
    }

    return [pscustomobject]$step
}

$outputRootPath = [System.IO.Path]::GetFullPath($OutputRoot)
$artifactsRootPath = [System.IO.Path]::GetFullPath($ArtifactsRoot)
New-Item -ItemType Directory -Force -Path $outputRootPath | Out-Null
New-Item -ItemType Directory -Force -Path $artifactsRootPath | Out-Null

$stageDefinitions = @(
    @{ stage = "stage"; label = "Run Android Stage Script Smoke" },
    @{ stage = "validate"; label = "Run Android Build Validate Smoke" }
)

$stageRecords = New-Object System.Collections.Generic.List[object]
foreach ($definition in $stageDefinitions) {
    $payloadPath = Join-Path $outputRootPath ("script-{0}.json" -f $definition.stage)
    if (Test-Path $payloadPath) {
        $stageRecords.Add((Get-Content -Path $payloadPath | ConvertFrom-Json))
    } else {
        $stageRecords.Add((New-MissingStageRecord -StageName $definition.stage -StageLabel $definition.label))
    }
}

$records = $stageRecords.ToArray()
$completedRecords = @($records | Where-Object { $_.status -eq "succeeded" -or $_.status -eq "failed" })
$totalDurationSeconds = if (@($completedRecords).Count -gt 0) {
    [Math]::Round((@($completedRecords) | Measure-Object -Property durationSeconds -Sum).Sum, 2)
} else {
    0
}
$failedStages = @($records | Where-Object { $_.status -eq "failed" }).Count
$missingStages = @($records | Where-Object { $_.status -eq "missing" }).Count
$executionStatus = Resolve-ExecutionStatus -FailedSteps $failedStages -MissingSteps $missingStages -CompletedSteps @($completedRecords).Count
$steps = @($records | ForEach-Object { ConvertTo-ExecutionStep -Record $_ })

$preservedTempRoots = @(
    Get-ChildItem -Path $artifactsRootPath -Directory -ErrorAction SilentlyContinue |
        Where-Object { $_.Name -ne "metadata" } |
        Sort-Object -Property Name |
        ForEach-Object { $_.Name }
)

$payload = [ordered]@{
    schemaVersion = 1
    generatedAtUtc = (Get-Date).ToUniversalTime().ToString("o")
    workflow = [ordered]@{
        name = "android-build-scripts-smoke"
        metadataRoot = $outputRootPath
        artifactsRoot = $artifactsRootPath
    }
    status = $executionStatus
    durationSeconds = $totalDurationSeconds
    totalStepCount = @($steps).Count
    completedStepCount = @($completedRecords).Count
    failedStepCount = $failedStages
    missingStepCount = $missingStages
    preservedTempRoots = $preservedTempRoots
    steps = $steps
}

$summaryLines = @(
    "## Android Build Scripts Smoke 阶段耗时摘要"
    ""
    ('- 已记录阶段数：`{0}`' -f @($completedRecords).Count)
    ('- 阶段累计耗时：`{0} sec`' -f $totalDurationSeconds)
    ('- 失败阶段数：`{0}`' -f $failedStages)
    ('- 缺失阶段数：`{0}`' -f $missingStages)
    ('- 保留临时目录数：`{0}`' -f @($preservedTempRoots).Count)
    ""
    "阶段明细："
)

foreach ($record in $records) {
    if ($record.status -eq "missing") {
        $summaryLines += ('- {0}：`missing`' -f $record.label)
        continue
    }

    $summaryLines += ('- {0}：`{1:N2}s` [{2}]' -f $record.label, [double]$record.durationSeconds, $record.status)
}

$summaryLines += ""
$summaryLines += "保留临时目录："
if (@($preservedTempRoots).Count -eq 0) {
    $summaryLines += "- 未检测到保留的 smoke 临时目录。"
} else {
    foreach ($directoryName in $preservedTempRoots) {
        $summaryLines += ('- `{0}`' -f $directoryName)
    }
}

$executionSummaryJsonPath = Join-Path $outputRootPath "execution-summary.json"
$executionSummaryMarkdownPath = Join-Path $outputRootPath "execution-summary.md"
$summaryMarkdown = $summaryLines -join [Environment]::NewLine

$payload | ConvertTo-Json -Depth 6 | Set-Content -Path $executionSummaryJsonPath
Set-Content -Path $executionSummaryMarkdownPath -Value $summaryMarkdown

if (-not [string]::IsNullOrWhiteSpace($env:GITHUB_STEP_SUMMARY)) {
    Add-Content -Path $env:GITHUB_STEP_SUMMARY -Value $summaryMarkdown
}

Write-Host "Captured Android build scripts smoke execution summary:"
Write-Host "  $executionSummaryJsonPath"
Write-Host "  $executionSummaryMarkdownPath"
