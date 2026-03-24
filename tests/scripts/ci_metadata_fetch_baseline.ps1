param(
    [Parameter(Mandatory = $true)]
    [string]$OutputRoot,
    [Parameter(Mandatory = $true)]
    [string]$WorkflowName,
    [Parameter(Mandatory = $true)]
    [string]$WorkflowFile,
    [Parameter(Mandatory = $true)]
    [string]$ArtifactName,
    [string]$Branch = "",
    [string]$FallbackBranch = "",
    [int]$MaxRunsToScan = 20
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

function Get-ExistingWorkflowDataHashtable {
    param([string]$ManifestPath)

    $data = @{}
    if (-not (Test-Path $ManifestPath)) {
        return $data
    }

    try {
        $manifest = Get-Content -Path $ManifestPath -Raw | ConvertFrom-Json
    } catch {
        return $data
    }

    if (-not (Test-ObjectProperty -InputObject $manifest -Name "workflow") -or $null -eq $manifest.workflow) {
        return $data
    }

    foreach ($property in $manifest.workflow.PSObject.Properties) {
        if ($property.Name -in @("name", "metadataRoot")) {
            continue
        }

        $data[$property.Name] = $property.Value
    }

    return $data
}

function New-ApiHeaders {
    param([string]$Token)

    return @{
        Accept = "application/vnd.github+json"
        Authorization = "Bearer $Token"
        "X-GitHub-Api-Version" = "2022-11-28"
    }
}

function Invoke-GitHubGet {
    param(
        [string]$Uri,
        [hashtable]$Headers
    )

    return Invoke-RestMethod -Method Get -Uri $Uri -Headers $Headers
}

function Get-CandidateBranches {
    param(
        [string]$PrimaryBranch,
        [string]$SecondaryBranch
    )

    $branches = New-Object System.Collections.Generic.List[string]

    foreach ($branch in @($PrimaryBranch, $SecondaryBranch)) {
        if ([string]::IsNullOrWhiteSpace($branch)) {
            continue
        }

        if (-not $branches.Contains($branch)) {
            $branches.Add($branch)
        }
    }

    return @($branches.ToArray())
}

function Resolve-BaselineMetadataDirectory {
    param([string]$ExtractRoot)

    $candidates = @(
        Get-ChildItem -Path $ExtractRoot -Recurse -Filter "metadata-manifest.json" -File -ErrorAction SilentlyContinue |
            Sort-Object -Property @{ Expression = { $_.FullName.Length } }, @{ Expression = { $_.FullName } }
    )

    if (@($candidates).Count -eq 0) {
        return $null
    }

    return (Split-Path $candidates[0].FullName -Parent)
}

function New-BaselineFetchSummaryMarkdown {
    param(
        [string]$WorkflowName,
        [string]$ArtifactName,
        [string]$Status,
        [string[]]$AttemptedBranches,
        [object]$Baseline
    )

    $lines = @(
        "## CI Baseline Fetch"
        ""
        ('- Workflow：`{0}`' -f $WorkflowName)
        ('- Artifact：`{0}`' -f $ArtifactName)
        ('- 状态：`{0}`' -f $Status)
        ('- 尝试分支：`{0}`' -f (($AttemptedBranches -join ", ")))
    )

    if ($null -ne $Baseline -and $Baseline.available) {
        $lines += ('- 基线分支：`{0}`' -f $Baseline.branch)
        $lines += ('- 基线 runId：`{0}`' -f $Baseline.runId)
        $lines += ('- 基线 artifactId：`{0}`' -f $Baseline.artifactId)
        $lines += ('- 基线 metadataRoot：`{0}`' -f $Baseline.metadataRoot)
    } elseif ($null -ne $Baseline -and (Test-ObjectProperty -InputObject $Baseline -Name "message")) {
        $lines += ('- 说明：{0}' -f $Baseline.message)
    }

    return ($lines -join [Environment]::NewLine)
}

$outputRootPath = [System.IO.Path]::GetFullPath($OutputRoot)
New-Item -ItemType Directory -Force -Path $outputRootPath | Out-Null

$manifestPath = Join-Path $outputRootPath "metadata-manifest.json"
$workflowData = Get-ExistingWorkflowDataHashtable -ManifestPath $manifestPath

$baselineRootPath = Join-Path $outputRootPath "baseline"
$baselineFetchJsonPath = Join-Path $outputRootPath "baseline-fetch.json"
$baselineFetchMarkdownPath = Join-Path $outputRootPath "baseline-fetch.md"

$repository = $env:GITHUB_REPOSITORY
$token = $env:GITHUB_TOKEN
$currentRunId = $env:GITHUB_RUN_ID
$attemptedBranches = Get-CandidateBranches -PrimaryBranch $Branch -SecondaryBranch $FallbackBranch

$status = "missing"
$baseline = [ordered]@{
    available = $false
    branch = $null
    runId = $null
    artifactId = $null
    metadataRoot = $baselineRootPath
}

if ([string]::IsNullOrWhiteSpace($repository)) {
    $baseline.message = "GITHUB_REPOSITORY was not provided."
} elseif ([string]::IsNullOrWhiteSpace($token)) {
    $baseline.message = "GITHUB_TOKEN was not provided."
} elseif (@($attemptedBranches).Count -eq 0) {
    $baseline.message = "No candidate branches were provided."
} else {
    $repositoryParts = $repository.Split("/", 2)
    if ($repositoryParts.Length -ne 2) {
        $baseline.message = ("Invalid GITHUB_REPOSITORY value: {0}" -f $repository)
    } else {
        $owner = $repositoryParts[0]
        $repo = $repositoryParts[1]
        $headers = New-ApiHeaders -Token $token
        $escapedWorkflowFile = [uri]::EscapeDataString($WorkflowFile)
        $resolved = $false

        foreach ($candidateBranch in $attemptedBranches) {
            if ($resolved) {
                break
            }

            $runsUri = "https://api.github.com/repos/$owner/$repo/actions/workflows/$escapedWorkflowFile/runs?branch=$([uri]::EscapeDataString($candidateBranch))&status=success&per_page=$MaxRunsToScan"

            try {
                $runsResponse = Invoke-GitHubGet -Uri $runsUri -Headers $headers
            } catch {
                $status = "failed"
                $baseline.message = ("Workflow run query failed for branch `{0}`: {1}" -f $candidateBranch, $_.Exception.Message)
                break
            }

            foreach ($run in @($runsResponse.workflow_runs)) {
                if (-not [string]::IsNullOrWhiteSpace($currentRunId) -and [string]$run.id -eq [string]$currentRunId) {
                    continue
                }

                $artifactsUri = "https://api.github.com/repos/$owner/$repo/actions/runs/$($run.id)/artifacts?per_page=100"
                try {
                    $artifactsResponse = Invoke-GitHubGet -Uri $artifactsUri -Headers $headers
                } catch {
                    $status = "failed"
                    $baseline.message = ("Artifact query failed for run `{0}`: {1}" -f $run.id, $_.Exception.Message)
                    $resolved = $true
                    break
                }

                $artifact = @(
                    $artifactsResponse.artifacts |
                        Where-Object { $_.name -eq $ArtifactName -and -not $_.expired } |
                        Select-Object -First 1
                )

                if (@($artifact).Count -eq 0) {
                    continue
                }

                $tempRoot = Join-Path ([System.IO.Path]::GetTempPath()) ("tinalux-baseline-fetch-" + [guid]::NewGuid().ToString("N"))
                $zipPath = Join-Path $tempRoot "artifact.zip"
                $extractRoot = Join-Path $tempRoot "extract"
                New-Item -ItemType Directory -Force -Path $extractRoot | Out-Null

                try {
                    Invoke-WebRequest -Uri $artifact[0].archive_download_url -Headers $headers -OutFile $zipPath
                    Expand-Archive -Path $zipPath -DestinationPath $extractRoot -Force
                    $resolvedBaselineMetadataRoot = Resolve-BaselineMetadataDirectory -ExtractRoot $extractRoot
                    if ($null -eq $resolvedBaselineMetadataRoot) {
                        $status = "failed"
                        $baseline.message = ("Artifact `{0}` from run `{1}` did not contain metadata-manifest.json." -f $ArtifactName, $run.id)
                        $resolved = $true
                        break
                    }

                    New-Item -ItemType Directory -Force -Path $baselineRootPath | Out-Null
                    Copy-Item -Path (Join-Path $resolvedBaselineMetadataRoot "*") -Destination $baselineRootPath -Recurse -Force

                    $status = "captured"
                    $baseline.available = $true
                    $baseline.branch = $candidateBranch
                    $baseline.runId = $run.id
                    $baseline.headSha = $run.head_sha
                    $baseline.artifactId = $artifact[0].id
                    $baseline.artifactName = $artifact[0].name
                    $baseline.createdAt = $artifact[0].created_at
                    $baseline.updatedAt = $artifact[0].updated_at
                    $baseline.metadataRoot = $baselineRootPath
                    $resolved = $true
                    break
                } catch {
                    $status = "failed"
                    $baseline.message = ("Artifact download failed for run `{0}`: {1}" -f $run.id, $_.Exception.Message)
                    $resolved = $true
                    break
                }
            }
        }

        if (-not $resolved -and -not $baseline.available -and $status -ne "failed") {
            $status = "missing"
            $baseline.message = ("No successful baseline artifact named `{0}` was found." -f $ArtifactName)
        }
    }
}

$payload = [ordered]@{
    schemaVersion = 1
    generatedAtUtc = (Get-Date).ToUniversalTime().ToString("o")
    workflow = [ordered]@{
        name = $WorkflowName
        metadataRoot = $outputRootPath
    }
    status = $status
    source = [ordered]@{
        repository = $repository
        workflowFile = $WorkflowFile
        artifactName = $ArtifactName
        branch = $Branch
        fallbackBranch = $FallbackBranch
        attemptedBranches = $attemptedBranches
        currentRunId = $currentRunId
    }
    baseline = $baseline
}

foreach ($key in @($workflowData.Keys | Sort-Object)) {
    $payload.workflow[$key] = $workflowData[$key]
}

$summaryMarkdown = New-BaselineFetchSummaryMarkdown `
    -WorkflowName $WorkflowName `
    -ArtifactName $ArtifactName `
    -Status $status `
    -AttemptedBranches $attemptedBranches `
    -Baseline $baseline

$payload | ConvertTo-Json -Depth 8 | Set-Content -Path $baselineFetchJsonPath
Set-Content -Path $baselineFetchMarkdownPath -Value $summaryMarkdown

$metadataManifestPath = Update-MetadataManifest `
    -OutputRoot $outputRootPath `
    -WorkflowName $WorkflowName `
    -WorkflowData $workflowData `
    -Entries @(
        (New-MetadataManifestEntry -OutputRoot $outputRootPath -Id "baselineFetch" -Path "baseline-fetch.json" -Format "json" -Role "baseline-fetch"),
        (New-MetadataManifestEntry -OutputRoot $outputRootPath -Id "baselineFetchMarkdown" -Path "baseline-fetch.md" -Format "markdown" -Role "baseline-fetch")
    )

if (-not [string]::IsNullOrWhiteSpace($env:GITHUB_STEP_SUMMARY)) {
    Add-Content -Path $env:GITHUB_STEP_SUMMARY -Value $summaryMarkdown
}

Write-Host "Captured CI baseline fetch summary:"
Write-Host "  workflow=$WorkflowName"
Write-Host "  status=$status"
Write-Host "  $baselineFetchJsonPath"
Write-Host "  $baselineFetchMarkdownPath"
Write-Host "  $metadataManifestPath"

exit 0
