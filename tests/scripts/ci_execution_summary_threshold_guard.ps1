param(
    [Parameter(Mandatory = $true)]
    [string]$OutputRoot,
    [Parameter(Mandatory = $true)]
    [string]$WorkflowName,
    [string]$SummaryTitle = "CI Threshold Guard",
    [double]$OverallDurationThresholdSeconds = 0,
    [string[]]$StageThresholds = @(),
    [string]$BaselineRoot = ""
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

function Get-OptionalJsonPayload {
    param([string]$Path)

    if (-not (Test-Path $Path)) {
        return $null
    }

    try {
        return Get-Content -Path $Path -Raw | ConvertFrom-Json
    } catch {
        return $null
    }
}

function Get-ObjectPropertyValue {
    param(
        [object]$InputObject,
        [string[]]$PropertyPath
    )

    $current = $InputObject
    foreach ($propertyName in $PropertyPath) {
        if (-not (Test-ObjectProperty -InputObject $current -Name $propertyName)) {
            return $null
        }

        $current = $current.$propertyName
        if ($null -eq $current) {
            return $null
        }
    }

    return $current
}

function Get-NormalizedTextValue {
    param([object]$Value)

    if ($null -eq $Value) {
        return $null
    }

    $text = $Value.ToString().Trim()
    if ([string]::IsNullOrWhiteSpace($text)) {
        return $null
    }

    return $text
}

function Get-RunnerFingerprintSummary {
    param([object]$RunnerFingerprint)

    if ($null -eq $RunnerFingerprint) {
        return $null
    }

    return [ordered]@{
        runner = [ordered]@{
            os = Get-NormalizedTextValue -Value (Get-ObjectPropertyValue -InputObject $RunnerFingerprint -PropertyPath @("runner", "os"))
            arch = Get-NormalizedTextValue -Value (Get-ObjectPropertyValue -InputObject $RunnerFingerprint -PropertyPath @("runner", "arch"))
            imageOs = Get-NormalizedTextValue -Value (Get-ObjectPropertyValue -InputObject $RunnerFingerprint -PropertyPath @("runner", "imageOs"))
            imageVersion = Get-NormalizedTextValue -Value (Get-ObjectPropertyValue -InputObject $RunnerFingerprint -PropertyPath @("runner", "imageVersion"))
        }
        tools = [ordered]@{
            powershell = [ordered]@{
                version = Get-NormalizedTextValue -Value (Get-ObjectPropertyValue -InputObject $RunnerFingerprint -PropertyPath @("tools", "powershell", "version"))
            }
            cmake = [ordered]@{
                version = Get-NormalizedTextValue -Value (Get-ObjectPropertyValue -InputObject $RunnerFingerprint -PropertyPath @("tools", "cmake", "version"))
            }
            ninja = [ordered]@{
                version = Get-NormalizedTextValue -Value (Get-ObjectPropertyValue -InputObject $RunnerFingerprint -PropertyPath @("tools", "ninja", "version"))
            }
            python = [ordered]@{
                version = Get-NormalizedTextValue -Value (Get-ObjectPropertyValue -InputObject $RunnerFingerprint -PropertyPath @("tools", "python", "version"))
            }
            cc = [ordered]@{
                version = Get-NormalizedTextValue -Value (Get-ObjectPropertyValue -InputObject $RunnerFingerprint -PropertyPath @("tools", "cc", "version"))
            }
            cxx = [ordered]@{
                version = Get-NormalizedTextValue -Value (Get-ObjectPropertyValue -InputObject $RunnerFingerprint -PropertyPath @("tools", "cxx", "version"))
            }
            msvc = [ordered]@{
                version = Get-NormalizedTextValue -Value (Get-ObjectPropertyValue -InputObject $RunnerFingerprint -PropertyPath @("tools", "msvc", "version"))
            }
        }
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

function New-BaselineRegressionCheck {
    param(
        [string]$Id,
        [string]$Label,
        [object]$CurrentValue,
        [object]$BaselineValue,
        [double]$MinDeltaValue,
        [double]$MaxGrowthRatio,
        [string]$Unit,
        [string]$MissingMessage
    )

    if ($null -eq $CurrentValue -or $null -eq $BaselineValue) {
        return $null
    }

    $current = [double]$CurrentValue
    $baseline = [double]$BaselineValue
    $delta = [Math]::Round(($current - $baseline), 2)

    if ($baseline -le 0) {
        if ($current -ge $MinDeltaValue) {
            return New-CheckResult `
                -Id $Id `
                -Label $Label `
                -Status "warning" `
                -Message ("Observed {0:N2}{1}; baseline was {2:N2}{1}." -f $current, $Unit, $baseline) `
                -Actual $current `
                -Threshold ("baseline*{0:N2} + {1:N2}{2}" -f $MaxGrowthRatio, $MinDeltaValue, $Unit) `
                -Unit $Unit
        }

        return New-CheckResult `
            -Id $Id `
            -Label $Label `
            -Status "passed" `
            -Message ("Observed {0:N2}{1}; baseline was {2:N2}{1}." -f $current, $Unit, $baseline) `
            -Actual $current `
            -Threshold ("baseline*{0:N2} + {1:N2}{2}" -f $MaxGrowthRatio, $MinDeltaValue, $Unit) `
            -Unit $Unit
    }

    $ratio = $current / $baseline
    if ($delta -ge $MinDeltaValue -and $ratio -ge $MaxGrowthRatio) {
        return New-CheckResult `
            -Id $Id `
            -Label $Label `
            -Status "warning" `
            -Message ("Observed {0:N2}{1}; baseline {2:N2}{1}; delta {3:N2}{1}; ratio {4:N2}x." -f $current, $Unit, $baseline, $delta, $ratio) `
            -Actual $current `
            -Threshold ("baseline*{0:N2} + {1:N2}{2}" -f $MaxGrowthRatio, $MinDeltaValue, $Unit) `
            -Unit $Unit
    }

    return New-CheckResult `
        -Id $Id `
        -Label $Label `
        -Status "passed" `
        -Message ("Observed {0:N2}{1}; baseline {2:N2}{1}; delta {3:N2}{1}; ratio {4:N2}x." -f $current, $Unit, $baseline, $delta, $ratio) `
        -Actual $current `
        -Threshold ("baseline*{0:N2} + {1:N2}{2}" -f $MaxGrowthRatio, $MinDeltaValue, $Unit) `
        -Unit $Unit
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

function New-StringBaselineCheck {
    param(
        [string]$Id,
        [string]$Label,
        [object]$CurrentValue,
        [object]$BaselineValue
    )

    $currentText = Get-NormalizedTextValue -Value $CurrentValue
    $baselineText = Get-NormalizedTextValue -Value $BaselineValue

    if ($null -eq $currentText -and $null -eq $baselineText) {
        return $null
    }

    $displayCurrent = if ($null -ne $currentText) { $currentText } else { "unavailable" }
    $displayBaseline = if ($null -ne $baselineText) { $baselineText } else { "unavailable" }

    if ($displayCurrent -eq $displayBaseline) {
        return New-CheckResult `
            -Id $Id `
            -Label $Label `
            -Status "passed" `
            -Message ("Observed `{0}`; baseline `{1}`." -f $displayCurrent, $displayBaseline) `
            -Actual $displayCurrent `
            -Threshold $displayBaseline
    }

    return New-CheckResult `
        -Id $Id `
        -Label $Label `
        -Status "warning" `
        -Message ("Observed `{0}`; baseline `{1}`." -f $displayCurrent, $displayBaseline) `
        -Actual $displayCurrent `
        -Threshold $displayBaseline
}

function New-ExecutionStatusCheck {
    param(
        [string]$Label,
        [string]$ActualStatus
    )

    if ([string]::IsNullOrWhiteSpace($ActualStatus)) {
        return New-CheckResult `
            -Id "execution-status" `
            -Label $Label `
            -Status "missing" `
            -Message "execution-summary.json did not provide status." `
            -Threshold "succeeded"
    }

    if ($ActualStatus -eq "succeeded") {
        return New-CheckResult `
            -Id "execution-status" `
            -Label $Label `
            -Status "passed" `
            -Message "Execution summary status is succeeded." `
            -Actual $ActualStatus `
            -Threshold "succeeded"
    }

    if ($ActualStatus -eq "missing") {
        return New-CheckResult `
            -Id "execution-status" `
            -Label $Label `
            -Status "missing" `
            -Message "Execution summary status is missing; the workflow did not produce a complete stage result." `
            -Actual $ActualStatus `
            -Threshold "succeeded"
    }

    return New-CheckResult `
        -Id "execution-status" `
        -Label $Label `
        -Status "warning" `
        -Message ("Execution summary status is {0}." -f $ActualStatus) `
        -Actual $ActualStatus `
        -Threshold "succeeded"
}

function Get-ExecutionStep {
    param(
        [object]$ExecutionSummary,
        [string]$StepName
    )

    if (-not (Test-ObjectProperty -InputObject $ExecutionSummary -Name "steps")) {
        return $null
    }

    return @($ExecutionSummary.steps | Where-Object { $_.name -eq $StepName } | Select-Object -First 1)
}

function Get-ExecutionStepDurationSeconds {
    param(
        [object]$ExecutionSummary,
        [string]$StepName
    )

    $step = Get-ExecutionStep -ExecutionSummary $ExecutionSummary -StepName $StepName
    if (@($step).Count -eq 0) {
        return $null
    }

    if (-not (Test-ObjectProperty -InputObject $step[0] -Name "durationSeconds")) {
        return $null
    }

    return [double]$step[0].durationSeconds
}

function Get-WorkflowDataHashtable {
    param([object]$Workflow)

    $data = @{}
    if ($null -eq $Workflow) {
        return $data
    }

    foreach ($property in $Workflow.PSObject.Properties) {
        if ($property.Name -in @("name", "metadataRoot")) {
            continue
        }

        $data[$property.Name] = $property.Value
    }

    return $data
}

function ConvertTo-StageThreshold {
    param([string]$Spec)

    if ($Spec -notmatch '^(?<name>[A-Za-z0-9_-]+)=(?<threshold>[0-9]+(?:\.[0-9]+)?)$') {
        throw "Invalid stage threshold spec: $Spec. Expected format: stageName=seconds"
    }

    return [ordered]@{
        name = $matches.name
        thresholdSeconds = [double]$matches.threshold
    }
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
        [string]$Title,
        [string]$Status,
        [object[]]$Inputs,
        [object[]]$Checks
    )

    $warningChecks = @($Checks | Where-Object { $_.status -eq "warning" })
    $missingChecks = @($Checks | Where-Object { $_.status -eq "missing" })
    $loadedInputs = @($Inputs | Where-Object { $_.status -eq "loaded" })

    $lines = @(
        ("## {0}" -f $Title)
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
$runnerFingerprintPath = Join-Path $outputRootPath "runner-fingerprint.json"
$thresholdCheckJsonPath = Join-Path $outputRootPath "threshold-check.json"
$thresholdCheckMarkdownPath = Join-Path $outputRootPath "threshold-check.md"
$baselineRootPath = if ([string]::IsNullOrWhiteSpace($BaselineRoot)) {
    Join-Path $outputRootPath "baseline"
} elseif ([System.IO.Path]::IsPathRooted($BaselineRoot)) {
    [System.IO.Path]::GetFullPath($BaselineRoot)
} else {
    [System.IO.Path]::GetFullPath((Join-Path $outputRootPath $BaselineRoot))
}
$baselineExecutionSummaryPath = Join-Path $baselineRootPath "execution-summary.json"
$baselineRunnerFingerprintPath = Join-Path $baselineRootPath "runner-fingerprint.json"
$baselineFetchPath = Join-Path $outputRootPath "baseline-fetch.json"

$stageThresholdDefinitions = @(
    $StageThresholds |
        Where-Object { -not [string]::IsNullOrWhiteSpace($_) } |
        ForEach-Object { ConvertTo-StageThreshold -Spec $_ }
)

$executionSummaryInput = Get-JsonInput -Id "executionSummary" -Path $executionSummaryPath
$runnerFingerprintInput = Get-JsonInput -Id "runnerFingerprint" -Path $runnerFingerprintPath
$inputs = @(
    $executionSummaryInput.input
    $runnerFingerprintInput.input
)
$executionSummary = $executionSummaryInput.payload
$runnerFingerprint = $runnerFingerprintInput.payload
$workflowData = if ($null -ne $executionSummary) {
    Get-WorkflowDataHashtable -Workflow $executionSummary.workflow
} else {
    @{}
}

$checks = New-Object System.Collections.Generic.List[object]
$metrics = [ordered]@{}
$baselineThresholds = [ordered]@{
    durationRatio = 1.25
    overallMinDeltaSeconds = 60
    stageMinDeltaSeconds = 30
}

$executionStatus = $null
$executionDurationSeconds = $null
if ($null -ne $executionSummary) {
    if (Test-ObjectProperty -InputObject $executionSummary -Name "status") {
        $executionStatus = $executionSummary.status
    }

    if (Test-ObjectProperty -InputObject $executionSummary -Name "durationSeconds") {
        $executionDurationSeconds = [double]$executionSummary.durationSeconds
    }

    $stageDurations = [ordered]@{}
    foreach ($stageThreshold in $stageThresholdDefinitions) {
        $stageDurations[$stageThreshold.name] = Get-ExecutionStepDurationSeconds -ExecutionSummary $executionSummary -StepName $stageThreshold.name
    }

    $metrics.executionSummary = [ordered]@{
        status = $executionStatus
        durationSeconds = $executionDurationSeconds
        stageDurations = $stageDurations
    }
}

$runnerFingerprintSummary = Get-RunnerFingerprintSummary -RunnerFingerprint $runnerFingerprint
if ($null -ne $runnerFingerprintSummary) {
    $metrics.runnerFingerprint = $runnerFingerprintSummary
}

$checks.Add((New-ExecutionStatusCheck -Label "Execution summary status" -ActualStatus $executionStatus))

if ($OverallDurationThresholdSeconds -gt 0) {
    $checks.Add((New-MaxValueCheck `
            -Id "execution-total-duration" `
            -Label "Overall workflow duration" `
            -ActualValue $executionDurationSeconds `
            -ThresholdValue $OverallDurationThresholdSeconds `
            -Unit "s" `
            -MissingMessage "execution-summary.json did not provide durationSeconds."))
}

$baselineExecutionSummary = Get-OptionalJsonPayload -Path $baselineExecutionSummaryPath
$baselineRunnerFingerprint = Get-OptionalJsonPayload -Path $baselineRunnerFingerprintPath
$baselineFetch = Get-OptionalJsonPayload -Path $baselineFetchPath
$baselineRunnerFingerprintSummary = Get-RunnerFingerprintSummary -RunnerFingerprint $baselineRunnerFingerprint
$baselineAvailable = ($null -ne $baselineExecutionSummary -or $null -ne $baselineRunnerFingerprintSummary)

$metrics.baseline = [ordered]@{
    available = $baselineAvailable
    metadataRoot = $baselineRootPath
}

if ($null -ne $baselineExecutionSummary) {
    $baselineStageDurations = [ordered]@{}
    foreach ($stageThreshold in $stageThresholdDefinitions) {
        $baselineStageDurations[$stageThreshold.name] = Get-ExecutionStepDurationSeconds -ExecutionSummary $baselineExecutionSummary -StepName $stageThreshold.name
    }

    $metrics.baseline.executionSummary = [ordered]@{
        status = if (Test-ObjectProperty -InputObject $baselineExecutionSummary -Name "status") { $baselineExecutionSummary.status } else { $null }
        durationSeconds = if (Test-ObjectProperty -InputObject $baselineExecutionSummary -Name "durationSeconds") { [double]$baselineExecutionSummary.durationSeconds } else { $null }
        stageDurations = $baselineStageDurations
    }
}

if ($null -ne $baselineRunnerFingerprintSummary) {
    $metrics.baseline.runnerFingerprint = $baselineRunnerFingerprintSummary
}

if ($null -ne $baselineFetch -and (Test-ObjectProperty -InputObject $baselineFetch -Name "baseline")) {
    $metrics.baseline.source = $baselineFetch.baseline
}

if ($null -ne $baselineFetch -and (Test-ObjectProperty -InputObject $baselineFetch -Name "status")) {
    $metrics.baseline.fetchStatus = $baselineFetch.status
}

if ($null -ne $baselineExecutionSummary) {
    $baselineOverallDurationSeconds = if (Test-ObjectProperty -InputObject $baselineExecutionSummary -Name "durationSeconds") {
        [double]$baselineExecutionSummary.durationSeconds
    } else {
        $null
    }
    $baselineCheck = New-BaselineRegressionCheck `
        -Id "baseline-execution-total-duration" `
        -Label "Overall workflow duration vs baseline" `
        -CurrentValue $executionDurationSeconds `
        -BaselineValue $baselineOverallDurationSeconds `
        -MinDeltaValue $baselineThresholds.overallMinDeltaSeconds `
        -MaxGrowthRatio $baselineThresholds.durationRatio `
        -Unit "s" `
        -MissingMessage "Baseline execution summary did not provide durationSeconds."
    if ($null -ne $baselineCheck) {
        $checks.Add($baselineCheck)
    }
}

foreach ($stageThreshold in $stageThresholdDefinitions) {
    $step = if ($null -ne $executionSummary) {
        Get-ExecutionStep -ExecutionSummary $executionSummary -StepName $stageThreshold.name
    } else {
        $null
    }
    $stepLabel = if (@($step).Count -gt 0 -and (Test-ObjectProperty -InputObject $step[0] -Name "label")) {
        $step[0].label
    } else {
        $stageThreshold.name
    }
    $duration = if ($null -ne $executionSummary) {
        Get-ExecutionStepDurationSeconds -ExecutionSummary $executionSummary -StepName $stageThreshold.name
    } else {
        $null
    }

    $checks.Add((New-MaxValueCheck `
            -Id ("{0}-duration" -f $stageThreshold.name) `
            -Label ("{0} duration" -f $stepLabel) `
            -ActualValue $duration `
            -ThresholdValue $stageThreshold.thresholdSeconds `
            -Unit "s" `
            -MissingMessage ("Stage duration was not available for step `{0}`." -f $stageThreshold.name)))

    if ($null -ne $baselineExecutionSummary) {
        $baselineDuration = Get-ExecutionStepDurationSeconds -ExecutionSummary $baselineExecutionSummary -StepName $stageThreshold.name
        $baselineCheck = New-BaselineRegressionCheck `
            -Id ("baseline-{0}-duration" -f $stageThreshold.name) `
            -Label ("{0} duration vs baseline" -f $stepLabel) `
            -CurrentValue $duration `
            -BaselineValue $baselineDuration `
            -MinDeltaValue $baselineThresholds.stageMinDeltaSeconds `
            -MaxGrowthRatio $baselineThresholds.durationRatio `
            -Unit "s" `
            -MissingMessage ("Baseline stage duration was not available for step `{0}`." -f $stageThreshold.name)
        if ($null -ne $baselineCheck) {
            $checks.Add($baselineCheck)
        }
    }
}

if ($null -ne $runnerFingerprintSummary -and $null -ne $baselineRunnerFingerprintSummary) {
    foreach ($comparison in @(
            @{
                id = "baseline-runner-os"
                label = "Runner OS vs baseline"
                current = $runnerFingerprintSummary.runner.os
                baseline = $baselineRunnerFingerprintSummary.runner.os
            },
            @{
                id = "baseline-runner-arch"
                label = "Runner architecture vs baseline"
                current = $runnerFingerprintSummary.runner.arch
                baseline = $baselineRunnerFingerprintSummary.runner.arch
            },
            @{
                id = "baseline-runner-image-os"
                label = "Runner image OS vs baseline"
                current = $runnerFingerprintSummary.runner.imageOs
                baseline = $baselineRunnerFingerprintSummary.runner.imageOs
            },
            @{
                id = "baseline-runner-image-version"
                label = "Runner image version vs baseline"
                current = $runnerFingerprintSummary.runner.imageVersion
                baseline = $baselineRunnerFingerprintSummary.runner.imageVersion
            },
            @{
                id = "baseline-powershell-version"
                label = "PowerShell version vs baseline"
                current = $runnerFingerprintSummary.tools.powershell.version
                baseline = $baselineRunnerFingerprintSummary.tools.powershell.version
            },
            @{
                id = "baseline-cmake-version"
                label = "CMake version vs baseline"
                current = $runnerFingerprintSummary.tools.cmake.version
                baseline = $baselineRunnerFingerprintSummary.tools.cmake.version
            },
            @{
                id = "baseline-ninja-version"
                label = "Ninja version vs baseline"
                current = $runnerFingerprintSummary.tools.ninja.version
                baseline = $baselineRunnerFingerprintSummary.tools.ninja.version
            },
            @{
                id = "baseline-python-version"
                label = "Python version vs baseline"
                current = $runnerFingerprintSummary.tools.python.version
                baseline = $baselineRunnerFingerprintSummary.tools.python.version
            },
            @{
                id = "baseline-cc-version"
                label = "CC version vs baseline"
                current = $runnerFingerprintSummary.tools.cc.version
                baseline = $baselineRunnerFingerprintSummary.tools.cc.version
            },
            @{
                id = "baseline-cxx-version"
                label = "CXX version vs baseline"
                current = $runnerFingerprintSummary.tools.cxx.version
                baseline = $baselineRunnerFingerprintSummary.tools.cxx.version
            },
            @{
                id = "baseline-msvc-version"
                label = "MSVC version vs baseline"
                current = $runnerFingerprintSummary.tools.msvc.version
                baseline = $baselineRunnerFingerprintSummary.tools.msvc.version
            }
        )) {
        $baselineCheck = New-StringBaselineCheck `
            -Id $comparison.id `
            -Label $comparison.label `
            -CurrentValue $comparison.current `
            -BaselineValue $comparison.baseline
        if ($null -ne $baselineCheck) {
            $checks.Add($baselineCheck)
        }
    }
}

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
        name = $WorkflowName
        metadataRoot = $outputRootPath
    }
    mode = "warning-only"
    status = $guardStatus
    warningCount = @($warningChecks).Count
    missingCheckCount = @($missingChecks).Count
    passedCheckCount = @($passedChecks).Count
    inputIssueCount = @($inputIssues).Count
    inputs = $inputs
    thresholds = [ordered]@{
        executionTotalSeconds = if ($OverallDurationThresholdSeconds -gt 0) { $OverallDurationThresholdSeconds } else { $null }
        stages = $stageThresholdDefinitions
    }
    metrics = $metrics
    checks = $checkArray
    warnings = $warningChecks
}

foreach ($key in @($workflowData.Keys | Sort-Object)) {
    $payload.workflow[$key] = $workflowData[$key]
}

$summaryMarkdown = New-ThresholdSummaryMarkdown `
    -Title $SummaryTitle `
    -Status $guardStatus `
    -Inputs $inputs `
    -Checks $checkArray

$payload | ConvertTo-Json -Depth 8 | Set-Content -Path $thresholdCheckJsonPath
Set-Content -Path $thresholdCheckMarkdownPath -Value $summaryMarkdown

$metadataManifestPath = Update-MetadataManifest `
    -OutputRoot $outputRootPath `
    -WorkflowName $WorkflowName `
    -WorkflowData $workflowData `
    -Entries @(
        (New-MetadataManifestEntry -OutputRoot $outputRootPath -Id "thresholdCheck" -Path "threshold-check.json" -Format "json" -Role "threshold-check"),
        (New-MetadataManifestEntry -OutputRoot $outputRootPath -Id "thresholdCheckMarkdown" -Path "threshold-check.md" -Format "markdown" -Role "threshold-check")
    )

if (-not [string]::IsNullOrWhiteSpace($env:GITHUB_STEP_SUMMARY)) {
    Add-Content -Path $env:GITHUB_STEP_SUMMARY -Value $summaryMarkdown
}

Publish-GitHubWarnings -WorkflowName $WorkflowName -Checks $checkArray

Write-Host "Captured execution summary threshold guard:"
Write-Host "  workflow=$WorkflowName"
Write-Host "  status=$guardStatus"
Write-Host "  $thresholdCheckJsonPath"
Write-Host "  $thresholdCheckMarkdownPath"
Write-Host "  $metadataManifestPath"

exit 0
