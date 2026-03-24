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

$preservedTempRoots = @(
    Get-ChildItem -Path $artifactsRootPath -Directory -ErrorAction SilentlyContinue |
        Where-Object { $_.Name -ne "metadata" } |
        Sort-Object -Property Name |
        ForEach-Object { $_.Name }
)

$payload = [ordered]@{
    generatedAtUtc = (Get-Date).ToUniversalTime().ToString("o")
    totalDurationSeconds = $totalDurationSeconds
    failedStages = $failedStages
    missingStages = $missingStages
    preservedTempRoots = $preservedTempRoots
    stages = $records
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

$scriptTimingsJsonPath = Join-Path $outputRootPath "script-timings.json"
$scriptTimingsSummaryPath = Join-Path $outputRootPath "script-timings-summary.md"
$summaryMarkdown = $summaryLines -join [Environment]::NewLine

$payload | ConvertTo-Json -Depth 6 | Set-Content -Path $scriptTimingsJsonPath
Set-Content -Path $scriptTimingsSummaryPath -Value $summaryMarkdown

if (-not [string]::IsNullOrWhiteSpace($env:GITHUB_STEP_SUMMARY)) {
    Add-Content -Path $env:GITHUB_STEP_SUMMARY -Value $summaryMarkdown
}

Write-Host "Captured Android build scripts smoke stage summary:"
Write-Host "  $scriptTimingsJsonPath"
Write-Host "  $scriptTimingsSummaryPath"
