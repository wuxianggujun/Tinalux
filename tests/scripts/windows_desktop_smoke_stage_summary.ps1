param(
    [Parameter(Mandatory = $true)]
    [string]$OutputRoot
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
New-Item -ItemType Directory -Force -Path $outputRootPath | Out-Null

$stageDefinitions = @(
    @{ stage = "configure"; label = "Configure Desktop Smoke Build" },
    @{ stage = "build"; label = "Build Desktop Smoke Tests" },
    @{ stage = "test"; label = "Run Desktop Smoke Tests" }
)

$stageRecords = New-Object System.Collections.Generic.List[object]
foreach ($definition in $stageDefinitions) {
    $payloadPath = Join-Path $outputRootPath ("stage-{0}.json" -f $definition.stage)
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

$payload = [ordered]@{
    generatedAtUtc = (Get-Date).ToUniversalTime().ToString("o")
    totalDurationSeconds = $totalDurationSeconds
    failedStages = $failedStages
    missingStages = $missingStages
    stages = $records
}

$summaryLines = @(
    "## Windows Desktop Smoke 阶段耗时摘要"
    ""
    ('- 已记录阶段数：`{0}`' -f @($completedRecords).Count)
    ('- 阶段累计耗时：`{0} sec`' -f $totalDurationSeconds)
    ('- 失败阶段数：`{0}`' -f $failedStages)
    ('- 缺失阶段数：`{0}`' -f $missingStages)
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

$stageTimingsJsonPath = Join-Path $outputRootPath "stage-timings.json"
$stageTimingsSummaryPath = Join-Path $outputRootPath "stage-timings-summary.md"
$summaryMarkdown = $summaryLines -join [Environment]::NewLine

$payload | ConvertTo-Json -Depth 6 | Set-Content -Path $stageTimingsJsonPath
Set-Content -Path $stageTimingsSummaryPath -Value $summaryMarkdown

if (-not [string]::IsNullOrWhiteSpace($env:GITHUB_STEP_SUMMARY)) {
    Add-Content -Path $env:GITHUB_STEP_SUMMARY -Value $summaryMarkdown
}

Write-Host "Captured Windows desktop smoke stage summary:"
Write-Host "  $stageTimingsJsonPath"
Write-Host "  $stageTimingsSummaryPath"
