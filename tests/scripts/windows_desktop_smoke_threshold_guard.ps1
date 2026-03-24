param(
    [Parameter(Mandatory = $true)]
    [string]$OutputRoot
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "ci_metadata_manifest_helpers.ps1")

function Test-ObjectProperty {
    param(
        [object]$InputObject,
        [string]$Name
    )

    return $null -ne $InputObject -and $InputObject.PSObject.Properties.Name -contains $Name
}

function Get-JsonInput {
    param(
        [string]$Id,
        [string]$Path
    )

    $result = [ordered]@{
        id = $Id
        path = [System.IO.Path]::GetFileName($Path)
        status = "missing"
    }

    if (-not (Test-Path $Path)) {
        $result.message = "Input file was not found."
        return [pscustomobject]@{
            input = [pscustomobject]$result
            payload = $null
        }
    }

    try {
        $payload = Get-Content -Path $Path -Raw | ConvertFrom-Json
    } catch {
        $result.status = "invalid"
        $result.message = ("JSON parse failed: {0}" -f $_.Exception.Message)
        return [pscustomobject]@{
            input = [pscustomobject]$result
            payload = $null
        }
    }

    $result.status = "loaded"
    return [pscustomobject]@{
        input = [pscustomobject]$result
        payload = $payload
    }
}

function New-CheckResult {
    param(
        [string]$Id,
        [string]$Label,
        [string]$Status,
        [string]$Message,
        [object]$Actual = $null,
        [object]$Threshold = $null,
        [string]$Unit = ""
    )

    $result = [ordered]@{
        id = $Id
        label = $Label
        status = $Status
        message = $Message
    }

    if ($PSBoundParameters.ContainsKey("Actual")) {
        $result.actual = $Actual
    }

    if ($PSBoundParameters.ContainsKey("Threshold")) {
        $result.threshold = $Threshold
    }

    if (-not [string]::IsNullOrWhiteSpace($Unit)) {
        $result.unit = $Unit
    }

    return [pscustomobject]$result
}

function New-MaxValueCheck {
    param(
        [string]$Id,
        [string]$Label,
        [object]$ActualValue,
        [double]$ThresholdValue,
        [string]$Unit,
        [string]$MissingMessage
    )

    if ($null -eq $ActualValue) {
        return New-CheckResult `
            -Id $Id `
            -Label $Label `
            -Status "missing" `
            -Message $MissingMessage `
            -Threshold $ThresholdValue `
            -Unit $Unit
    }

    $actual = [double]$ActualValue
    if ($actual -gt $ThresholdValue) {
        return New-CheckResult `
            -Id $Id `
            -Label $Label `
            -Status "warning" `
            -Message ("Observed {0:N2}{1}, above threshold {2:N2}{1}." -f $actual, $Unit, $ThresholdValue) `
            -Actual $actual `
            -Threshold $ThresholdValue `
            -Unit $Unit
    }

    return New-CheckResult `
        -Id $Id `
        -Label $Label `
        -Status "passed" `
        -Message ("Observed {0:N2}{1}, within threshold {2:N2}{1}." -f $actual, $Unit, $ThresholdValue) `
        -Actual $actual `
        -Threshold $ThresholdValue `
        -Unit $Unit
}

function New-CacheStateCheck {
    param(
        [string]$Id,
        [string]$Label,
        [string]$State
    )

    if ([string]::IsNullOrWhiteSpace($State)) {
        return New-CheckResult `
            -Id $Id `
            -Label $Label `
            -Status "missing" `
            -Message "Cache state was not provided."
    }

    if ($State -eq "miss") {
        return New-CheckResult `
            -Id $Id `
            -Label $Label `
            -Status "warning" `
            -Message "Cache state is miss; this run likely incurred a cold-start penalty." `
            -Actual $State `
            -Threshold "warm-restore-or-better"
    }

    return New-CheckResult `
        -Id $Id `
        -Label $Label `
        -Status "passed" `
        -Message ("Cache state is {0}." -f $State) `
        -Actual $State `
        -Threshold "warm-restore-or-better"
}

function Get-StepDurationSeconds {
    param(
        [object]$ExecutionSummary,
        [string]$StepName
    )

    if (-not (Test-ObjectProperty -InputObject $ExecutionSummary -Name "steps")) {
        return $null
    }

    $step = @($ExecutionSummary.steps | Where-Object { $_.name -eq $StepName } | Select-Object -First 1)
    if (@($step).Count -eq 0) {
        return $null
    }

    if (-not (Test-ObjectProperty -InputObject $step[0] -Name "durationSeconds")) {
        return $null
    }

    return [double]$step[0].durationSeconds
}

function Format-CheckValue {
    param(
        [object]$Value,
        [string]$Unit = ""
    )

    if ($null -eq $Value) {
        return "n/a"
    }

    if ($Value -is [double] -or $Value -is [float] -or $Value -is [decimal]) {
        $text = ('{0:N2}' -f [double]$Value)
    } else {
        $text = $Value.ToString()
    }

    if (-not [string]::IsNullOrWhiteSpace($Unit)) {
        return "$text$Unit"
    }

    return $text
}

function Format-GitHubCommandMessage {
    param([string]$Message)

    if ($null -eq $Message) {
        return ""
    }

    return $Message.Replace("%", "%25").Replace("`r", "%0D").Replace("`n", "%0A")
}

function Publish-GitHubWarnings {
    param(
        [string]$WorkflowName,
        [object[]]$Checks
    )

    foreach ($check in @($Checks | Where-Object { $_.status -eq "warning" })) {
        $message = "{0} threshold {1}: {2}" -f $WorkflowName, $check.id, $check.message
        Write-Host ("::warning::{0}" -f (Format-GitHubCommandMessage -Message $message))
    }
}

function New-ThresholdSummaryMarkdown {
    param(
        [string]$Status,
        [object[]]$Inputs,
        [object[]]$Checks
    )

    $warningChecks = @($Checks | Where-Object { $_.status -eq "warning" })
    $missingChecks = @($Checks | Where-Object { $_.status -eq "missing" })
    $loadedInputs = @($Inputs | Where-Object { $_.status -eq "loaded" })

    $lines = @(
        "## Windows Desktop Smoke 阈值守卫"
        ""
        ('- 状态：`{0}`' -f $Status)
        ('- 告警数：`{0}`' -f @($warningChecks).Count)
        ('- 缺失检查数：`{0}`' -f @($missingChecks).Count)
        ('- 输入加载数：`{0}/{1}`' -f @($loadedInputs).Count, @($Inputs).Count)
        '- 模式：`warning-only`（当前只汇总告警，不拦截 CI）'
        ""
        "输入文件："
    )

    foreach ($input in $Inputs) {
        $line = ('- `{0}` [{1}]' -f $input.path, $input.status)
        if (Test-ObjectProperty -InputObject $input -Name "message") {
            $line = "$line：$($input.message)"
        }

        $lines += $line
    }

    $lines += ""
    $lines += "告警项："
    if (@($warningChecks).Count -eq 0) {
        $lines += "- 未发现阈值告警。"
    } else {
        foreach ($check in $warningChecks) {
            $actualText = if (Test-ObjectProperty -InputObject $check -Name "actual") {
                Format-CheckValue -Value $check.actual -Unit $check.unit
            } else {
                "n/a"
            }
            $thresholdText = if (Test-ObjectProperty -InputObject $check -Name "threshold") {
                Format-CheckValue -Value $check.threshold -Unit $check.unit
            } else {
                "n/a"
            }

            $lines += ('- `{0}`：actual=`{1}` / threshold=`{2}`；{3}' -f $check.id, $actualText, $thresholdText, $check.message)
        }
    }

    $lines += ""
    $lines += "检查明细："
    foreach ($check in $Checks) {
        $actualText = if (Test-ObjectProperty -InputObject $check -Name "actual") {
            Format-CheckValue -Value $check.actual -Unit $check.unit
        } else {
            "n/a"
        }
        $thresholdText = if (Test-ObjectProperty -InputObject $check -Name "threshold") {
            Format-CheckValue -Value $check.threshold -Unit $check.unit
        } else {
            "n/a"
        }

        $lines += ('- `{0}` [{1}] actual=`{2}` / threshold=`{3}`；{4}' -f $check.id, $check.status, $actualText, $thresholdText, $check.message)
    }

    return ($lines -join [Environment]::NewLine)
}

$outputRootPath = [System.IO.Path]::GetFullPath($OutputRoot)
New-Item -ItemType Directory -Force -Path $outputRootPath | Out-Null

$executionSummaryPath = Join-Path $outputRootPath "execution-summary.json"
$testTimingsPath = Join-Path $outputRootPath "test-timings.json"
$cacheSummaryPath = Join-Path $outputRootPath "cache-summary.json"
$thresholdCheckJsonPath = Join-Path $outputRootPath "threshold-check.json"
$thresholdCheckMarkdownPath = Join-Path $outputRootPath "threshold-check.md"

$thresholds = [ordered]@{
    executionTotalSeconds = 2700
    configureSeconds = 600
    buildSeconds = 1800
    testSeconds = 900
    totalTestSeconds = 600
    slowestTestSeconds = 20
    slowTestsOverOneSecond = 25
}

$executionSummaryInput = Get-JsonInput -Id "executionSummary" -Path $executionSummaryPath
$testTimingsInput = Get-JsonInput -Id "testTimings" -Path $testTimingsPath
$cacheSummaryInput = Get-JsonInput -Id "cacheSummary" -Path $cacheSummaryPath

$inputs = @(
    $executionSummaryInput.input
    $testTimingsInput.input
    $cacheSummaryInput.input
)

$executionSummary = $executionSummaryInput.payload
$testTimings = $testTimingsInput.payload
$cacheSummary = $cacheSummaryInput.payload

$checks = New-Object System.Collections.Generic.List[object]
$metrics = [ordered]@{}

$executionDurationSeconds = $null
$configureDurationSeconds = $null
$buildDurationSeconds = $null
$testDurationSeconds = $null

if ($null -ne $executionSummary) {
    if (Test-ObjectProperty -InputObject $executionSummary -Name "durationSeconds") {
        $executionDurationSeconds = [double]$executionSummary.durationSeconds
    }

    $configureDurationSeconds = Get-StepDurationSeconds -ExecutionSummary $executionSummary -StepName "configure"
    $buildDurationSeconds = Get-StepDurationSeconds -ExecutionSummary $executionSummary -StepName "build"
    $testDurationSeconds = Get-StepDurationSeconds -ExecutionSummary $executionSummary -StepName "test"

    $metrics.executionSummary = [ordered]@{
        status = if (Test-ObjectProperty -InputObject $executionSummary -Name "status") { $executionSummary.status } else { $null }
        durationSeconds = $executionDurationSeconds
        stageDurations = [ordered]@{
            configure = $configureDurationSeconds
            build = $buildDurationSeconds
            test = $testDurationSeconds
        }
    }
}

$checks.Add((New-MaxValueCheck -Id "execution-total-duration" -Label "Overall smoke duration" -ActualValue $executionDurationSeconds -ThresholdValue $thresholds.executionTotalSeconds -Unit "s" -MissingMessage "execution-summary.json did not provide durationSeconds."))
$checks.Add((New-MaxValueCheck -Id "configure-duration" -Label "Configure stage duration" -ActualValue $configureDurationSeconds -ThresholdValue $thresholds.configureSeconds -Unit "s" -MissingMessage "Configure stage duration was not available in execution-summary.json."))
$checks.Add((New-MaxValueCheck -Id "build-duration" -Label "Build stage duration" -ActualValue $buildDurationSeconds -ThresholdValue $thresholds.buildSeconds -Unit "s" -MissingMessage "Build stage duration was not available in execution-summary.json."))
$checks.Add((New-MaxValueCheck -Id "test-duration" -Label "Test stage duration" -ActualValue $testDurationSeconds -ThresholdValue $thresholds.testSeconds -Unit "s" -MissingMessage "Test stage duration was not available in execution-summary.json."))

$totalTestDurationSeconds = $null
$slowTestsOverOneSecond = $null
$slowestTestDurationSeconds = $null
$slowestTestName = $null

if ($null -ne $testTimings) {
    $slowestEntry = @()
    if ((Test-ObjectProperty -InputObject $testTimings -Name "status") -and $testTimings.status -eq "parsed") {
        if (Test-ObjectProperty -InputObject $testTimings -Name "totalDurationSeconds") {
            $totalTestDurationSeconds = [double]$testTimings.totalDurationSeconds
        }

        if (Test-ObjectProperty -InputObject $testTimings -Name "slowTestsOverOneSecond") {
            $slowTestsOverOneSecond = [int]$testTimings.slowTestsOverOneSecond
        }

        if (Test-ObjectProperty -InputObject $testTimings -Name "topEntries") {
            $slowestEntry = @($testTimings.topEntries | Select-Object -First 1)
        }

        if (@($slowestEntry).Count -gt 0 -and (Test-ObjectProperty -InputObject $slowestEntry[0] -Name "seconds")) {
            $slowestTestDurationSeconds = [double]$slowestEntry[0].seconds
            $slowestTestName = if (Test-ObjectProperty -InputObject $slowestEntry[0] -Name "name") { $slowestEntry[0].name } else { $null }
        }
    }

    $metrics.testTimings = [ordered]@{
        status = if (Test-ObjectProperty -InputObject $testTimings -Name "status") { $testTimings.status } else { $null }
        totalDurationSeconds = $totalTestDurationSeconds
        slowTestsOverOneSecond = $slowTestsOverOneSecond
        slowestTest = [ordered]@{
            name = $slowestTestName
            durationSeconds = $slowestTestDurationSeconds
        }
    }
}

$checks.Add((New-MaxValueCheck -Id "test-total-duration" -Label "Total parsed test duration" -ActualValue $totalTestDurationSeconds -ThresholdValue $thresholds.totalTestSeconds -Unit "s" -MissingMessage "test-timings.json did not provide parsed totalDurationSeconds."))
$checks.Add((New-MaxValueCheck -Id "slowest-test-duration" -Label "Slowest parsed test duration" -ActualValue $slowestTestDurationSeconds -ThresholdValue $thresholds.slowestTestSeconds -Unit "s" -MissingMessage "test-timings.json did not provide a slowest test entry."))
$checks.Add((New-MaxValueCheck -Id "slow-tests-over-one-second" -Label "Count of tests >= 1 second" -ActualValue $slowTestsOverOneSecond -ThresholdValue $thresholds.slowTestsOverOneSecond -Unit "" -MissingMessage "test-timings.json did not provide slowTestsOverOneSecond."))

$sourceCacheState = $null
$buildCacheState = $null

if ($null -ne $cacheSummary) {
    if ((Test-ObjectProperty -InputObject $cacheSummary -Name "source") -and (Test-ObjectProperty -InputObject $cacheSummary.source -Name "state")) {
        $sourceCacheState = $cacheSummary.source.state
    }

    if ((Test-ObjectProperty -InputObject $cacheSummary -Name "build") -and (Test-ObjectProperty -InputObject $cacheSummary.build -Name "state")) {
        $buildCacheState = $cacheSummary.build.state
    }

    $metrics.cacheSummary = [ordered]@{
        status = if (Test-ObjectProperty -InputObject $cacheSummary -Name "status") { $cacheSummary.status } else { $null }
        source = [ordered]@{
            state = $sourceCacheState
        }
        build = [ordered]@{
            state = $buildCacheState
        }
    }
}

$checks.Add((New-CacheStateCheck -Id "source-cache-state" -Label "Skia source cache state" -State $sourceCacheState))
$checks.Add((New-CacheStateCheck -Id "build-cache-state" -Label "Skia build cache state" -State $buildCacheState))

$checkArray = @($checks.ToArray())
$warningChecks = @($checkArray | Where-Object { $_.status -eq "warning" })
$missingChecks = @($checkArray | Where-Object { $_.status -eq "missing" })
$passedChecks = @($checkArray | Where-Object { $_.status -eq "passed" })
$inputIssues = @($inputs | Where-Object { $_.status -ne "loaded" })

$guardStatus = if (@($warningChecks).Count -gt 0) {
    "warning"
} elseif (@($missingChecks).Count -gt 0 -or @($inputIssues).Count -gt 0) {
    "missing"
} else {
    "succeeded"
}

$payload = [ordered]@{
    schemaVersion = 1
    generatedAtUtc = (Get-Date).ToUniversalTime().ToString("o")
    workflow = [ordered]@{
        name = "windows-desktop-smoke"
        metadataRoot = $outputRootPath
    }
    mode = "warning-only"
    status = $guardStatus
    warningCount = @($warningChecks).Count
    missingCheckCount = @($missingChecks).Count
    passedCheckCount = @($passedChecks).Count
    inputIssueCount = @($inputIssues).Count
    inputs = $inputs
    thresholds = $thresholds
    metrics = $metrics
    checks = $checkArray
    warnings = $warningChecks
}

$summaryMarkdown = New-ThresholdSummaryMarkdown `
    -Status $guardStatus `
    -Inputs $inputs `
    -Checks $checkArray

$payload | ConvertTo-Json -Depth 8 | Set-Content -Path $thresholdCheckJsonPath
Set-Content -Path $thresholdCheckMarkdownPath -Value $summaryMarkdown

$metadataManifestPath = Update-MetadataManifest `
    -OutputRoot $outputRootPath `
    -WorkflowName "windows-desktop-smoke" `
    -Entries @(
        (New-MetadataManifestEntry -OutputRoot $outputRootPath -Id "thresholdCheck" -Path "threshold-check.json" -Format "json" -Role "threshold-check"),
        (New-MetadataManifestEntry -OutputRoot $outputRootPath -Id "thresholdCheckMarkdown" -Path "threshold-check.md" -Format "markdown" -Role "threshold-check")
    )

if (-not [string]::IsNullOrWhiteSpace($env:GITHUB_STEP_SUMMARY)) {
    Add-Content -Path $env:GITHUB_STEP_SUMMARY -Value $summaryMarkdown
}

Publish-GitHubWarnings -WorkflowName "windows-desktop-smoke" -Checks $checkArray

Write-Host "Captured Windows desktop smoke threshold summary:"
Write-Host "  status=$guardStatus"
Write-Host "  $thresholdCheckJsonPath"
Write-Host "  $thresholdCheckMarkdownPath"
Write-Host "  $metadataManifestPath"

exit 0
