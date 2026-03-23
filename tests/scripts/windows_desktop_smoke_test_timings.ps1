param(
    [Parameter(Mandatory = $true)]
    [string]$BuildDir,
    [Parameter(Mandatory = $true)]
    [string]$OutputRoot,
    [int]$MaxTests = 10
)

$ErrorActionPreference = "Stop"

function New-TimingSummaryMarkdown {
    param(
        [string]$BuildDirLabel,
        [string]$LastTestLogPath,
        [object[]]$Entries,
        [object[]]$TopEntries,
        [int]$MaxTestsToShow
    )

    $totalTests = @($Entries).Count
    $totalDuration = if ($totalTests -gt 0) {
        [Math]::Round((@($Entries) | Measure-Object -Property seconds -Sum).Sum, 2)
    } else {
        0
    }
    $slowTestsOverOneSecond = @($Entries | Where-Object { $_.seconds -ge 1.0 }).Count

    $lines = @(
        "## Windows Desktop Smoke 耗时摘要"
        ""
        ('- 构建目录：`{0}`' -f $BuildDirLabel)
        ('- 数据源：`{0}`' -f $LastTestLogPath)
        ('- 已解析测试数：`{0}`' -f $totalTests)
        ('- 测试累计耗时：`{0} sec`' -f $totalDuration)
        ('- 耗时 >= 1s 的测试数：`{0}`' -f $slowTestsOverOneSecond)
        ""
        ('Top {0} 慢测试：' -f [Math]::Min($MaxTestsToShow, @($TopEntries).Count))
    )

    if (@($TopEntries).Count -eq 0) {
        $lines += "- 未解析到测试时长数据。"
    } else {
        foreach ($entry in $TopEntries) {
            $lines += ('- `#{0} {1}`: `{2:N2}s` [{3}]' -f $entry.index, $entry.name, $entry.seconds, $entry.status)
        }
    }

    return ($lines -join [Environment]::NewLine)
}

$buildDirPath = [System.IO.Path]::GetFullPath((Resolve-Path $BuildDir).Path)
$outputRootPath = [System.IO.Path]::GetFullPath($OutputRoot)
New-Item -ItemType Directory -Force -Path $outputRootPath | Out-Null

$lastTestLogPath = Join-Path $buildDirPath "Testing/Temporary/LastTest.log"
$timingsJsonPath = Join-Path $outputRootPath "test-timings.json"
$timingsSummaryPath = Join-Path $outputRootPath "test-timings-summary.md"

if (-not (Test-Path $lastTestLogPath)) {
    $missingPayload = [ordered]@{
        buildDir = $buildDirPath
        lastTestLog = $lastTestLogPath
        status = "missing"
        message = "CTest LastTest.log was not found. Desktop smoke likely failed before entering the ctest phase."
        entries = @()
    }
    $missingPayload | ConvertTo-Json -Depth 5 | Set-Content -Path $timingsJsonPath

    $missingMarkdown = @(
        "## Windows Desktop Smoke 耗时摘要"
        ""
        ('- 构建目录：`{0}`' -f $buildDirPath)
        ('- 数据源缺失：`{0}`' -f $lastTestLogPath)
        '- 说明：桌面 smoke 可能在进入 `ctest` 前就失败了，因此没有可解析的测试时长。'
    ) -join [Environment]::NewLine

    Set-Content -Path $timingsSummaryPath -Value $missingMarkdown
    if (-not [string]::IsNullOrWhiteSpace($env:GITHUB_STEP_SUMMARY)) {
        Add-Content -Path $env:GITHUB_STEP_SUMMARY -Value $missingMarkdown
    }

    Write-Host "No LastTest.log found for desktop smoke timings:"
    Write-Host "  $lastTestLogPath"
    exit 0
}

$entries = New-Object System.Collections.Generic.List[object]
$currentIndex = $null
$currentTotal = $null
$currentName = $null
$currentSeconds = $null
$currentStatus = $null

function Add-CurrentTimingEntry {
    if ($null -eq $currentName -or $null -eq $currentSeconds) {
        return
    }

    $entries.Add([pscustomobject]@{
        index = $currentIndex
        total = $currentTotal
        name = $currentName
        seconds = [double]$currentSeconds
        status = if ([string]::IsNullOrWhiteSpace($currentStatus)) { "Unknown" } else { $currentStatus }
    })
}

foreach ($line in Get-Content -Path $lastTestLogPath) {
    if ($line -match '^(?<index>\d+)\/(?<total>\d+)\s+Test:\s+(?<name>.+)$') {
        Add-CurrentTimingEntry
        $currentIndex = [int]$matches.index
        $currentTotal = [int]$matches.total
        $currentName = $matches.name.Trim()
        $currentSeconds = $null
        $currentStatus = $null
        continue
    }

    if ($null -ne $currentName -and $line -match '^Test time =\s+(?<seconds>[0-9.]+)\s+sec$') {
        $currentSeconds = [double]$matches.seconds
        continue
    }

    if ($null -ne $currentName -and $null -ne $currentSeconds -and $line -match '^(?<status>Test Passed\.|.*Failed.*|.*Timeout.*|.*Not Run.*|\*{3}.*)$') {
        $currentStatus = $matches.status.Trim()
        Add-CurrentTimingEntry
        $currentIndex = $null
        $currentTotal = $null
        $currentName = $null
        $currentSeconds = $null
        $currentStatus = $null
    }
}

Add-CurrentTimingEntry

$parsedEntries = $entries.ToArray()
$topEntries = @(
    $parsedEntries |
        Sort-Object -Property @{ Expression = 'seconds'; Descending = $true }, @{ Expression = 'index'; Descending = $false } |
        Select-Object -First $MaxTests
)

$payload = [ordered]@{
    buildDir = $buildDirPath
    lastTestLog = $lastTestLogPath
    status = "parsed"
    maxTests = $MaxTests
    totalTests = $parsedEntries.Count
    totalDurationSeconds = if ($parsedEntries.Count -gt 0) {
        [Math]::Round(($parsedEntries | Measure-Object -Property seconds -Sum).Sum, 4)
    } else {
        0
    }
    slowTestsOverOneSecond = @($parsedEntries | Where-Object { $_.seconds -ge 1.0 }).Count
    topEntries = $topEntries
    entries = $parsedEntries
}

$payload | ConvertTo-Json -Depth 6 | Set-Content -Path $timingsJsonPath

$summaryMarkdown = New-TimingSummaryMarkdown `
    -BuildDirLabel $buildDirPath `
    -LastTestLogPath $lastTestLogPath `
    -Entries $parsedEntries `
    -TopEntries $topEntries `
    -MaxTestsToShow $MaxTests

Set-Content -Path $timingsSummaryPath -Value $summaryMarkdown
if (-not [string]::IsNullOrWhiteSpace($env:GITHUB_STEP_SUMMARY)) {
    Add-Content -Path $env:GITHUB_STEP_SUMMARY -Value $summaryMarkdown
}

Write-Host "Captured Windows desktop smoke timing summary:"
Write-Host "  $timingsJsonPath"
Write-Host "  $timingsSummaryPath"
