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

function New-ValidationIssue {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Code,
        [Parameter(Mandatory = $true)]
        [string]$Message,
        [string]$FileId = ""
    )

    $issue = [ordered]@{
        code = $Code
        message = $Message
    }

    if (-not [string]::IsNullOrWhiteSpace($FileId)) {
        $issue.fileId = $FileId
    }

    return [pscustomobject]$issue
}

function Get-RequiredManifestFileIds {
    param([string]$WorkflowName)

    switch ($WorkflowName) {
        "windows-desktop-smoke" {
            return @(
                "cacheSummary",
                "executionSummary",
                "executionSummaryMarkdown",
                "metadataManifest",
                "runnerFingerprint",
                "testTimings",
                "testTimingsSummary",
                "thresholdCheck",
                "thresholdCheckMarkdown",
                "workflowSummary"
            )
        }
        "android-build-scripts-smoke" {
            return @(
                "executionSummary",
                "executionSummaryMarkdown",
                "metadataManifest",
                "runnerFingerprint",
                "workflowSummary"
            )
        }
        "linux-tina-glfw-x11" {
            return @(
                "executionSummary",
                "executionSummaryMarkdown",
                "metadataManifest",
                "runnerFingerprint",
                "workflowSummary"
            )
        }
        default {
            return @("metadataManifest")
        }
    }
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

function New-ValidationSummaryMarkdown {
    param(
        [string]$WorkflowName,
        [string]$Status,
        [int]$DeclaredFileCount,
        [int]$ValidatedFileCount,
        [int]$MissingFileCount,
        [int]$InvalidFileCount,
        [object[]]$MissingRequiredIds,
        [object[]]$Issues
    )

    $lines = @(
        "## CI Metadata 验收摘要"
        ""
        ('- Workflow：`{0}`' -f $WorkflowName)
        ('- 状态：`{0}`' -f $Status)
        ('- 已声明文件数：`{0}`' -f $DeclaredFileCount)
        ('- 验证通过文件数：`{0}`' -f $ValidatedFileCount)
        ('- 缺失文件数：`{0}`' -f $MissingFileCount)
        ('- 无效文件数：`{0}`' -f $InvalidFileCount)
        ('- 缺失必需文件数：`{0}`' -f @($MissingRequiredIds).Count)
    )

    if (@($MissingRequiredIds).Count -gt 0) {
        $lines += ""
        $lines += "缺失的必需文件："
        foreach ($fileId in $MissingRequiredIds) {
            $lines += ('- `{0}`' -f $fileId)
        }
    }

    if (@($Issues).Count -gt 0) {
        $lines += ""
        $lines += "问题列表："
        foreach ($issue in $Issues) {
            $prefix = if (Test-ObjectProperty -InputObject $issue -Name "fileId") {
                ('[{0}] ' -f $issue.fileId)
            } else {
                ""
            }

            $lines += ('- `{0}` {1}{2}' -f $issue.code, $prefix, $issue.message)
        }
    }

    return ($lines -join [Environment]::NewLine)
}

$outputRootPath = [System.IO.Path]::GetFullPath($OutputRoot)
New-Item -ItemType Directory -Force -Path $outputRootPath | Out-Null

$manifestPath = Join-Path $outputRootPath "metadata-manifest.json"
$validationJsonPath = Join-Path $outputRootPath "metadata-validation.json"
$validationMarkdownPath = Join-Path $outputRootPath "metadata-validation.md"

$issues = New-Object System.Collections.Generic.List[object]
$fileResults = New-Object System.Collections.Generic.List[object]
$workflowName = "unknown"
$workflowData = @{}
$validationStatus = "failed"

if (-not (Test-Path $manifestPath)) {
    $issues.Add((New-ValidationIssue -Code "missing-manifest" -Message "metadata-manifest.json was not found."))
} else {
    try {
        $manifest = Get-Content -Path $manifestPath -Raw | ConvertFrom-Json
    } catch {
        $manifest = $null
        $issues.Add((New-ValidationIssue -Code "invalid-manifest" -Message ("metadata-manifest.json could not be parsed: {0}" -f $_.Exception.Message)))
    }

    if ($null -ne $manifest) {
        if (-not (Test-ObjectProperty -InputObject $manifest -Name "schemaVersion") -or [int]$manifest.schemaVersion -ne 1) {
            $issues.Add((New-ValidationIssue -Code "manifest-schema-version" -Message "metadata-manifest.json schemaVersion must be 1."))
        }

        if (-not (Test-ObjectProperty -InputObject $manifest -Name "workflow") -or $null -eq $manifest.workflow) {
            $issues.Add((New-ValidationIssue -Code "manifest-workflow" -Message "metadata-manifest.json is missing the workflow block."))
        } else {
            if (Test-ObjectProperty -InputObject $manifest.workflow -Name "name") {
                $workflowName = $manifest.workflow.name
            }

            $workflowData = Get-WorkflowDataHashtable -Workflow $manifest.workflow

            if (-not (Test-ObjectProperty -InputObject $manifest.workflow -Name "metadataRoot") -or `
                [string]::IsNullOrWhiteSpace($manifest.workflow.metadataRoot)) {
                $issues.Add((New-ValidationIssue -Code "manifest-metadata-root" -Message "metadata-manifest.json workflow.metadataRoot is missing."))
            } elseif ([System.IO.Path]::GetFullPath($manifest.workflow.metadataRoot) -ne $outputRootPath) {
                $issues.Add((New-ValidationIssue -Code "manifest-metadata-root" -Message ("workflow.metadataRoot does not match OutputRoot: {0}" -f $manifest.workflow.metadataRoot)))
            }
        }

        $requiredIds = Get-RequiredManifestFileIds -WorkflowName $workflowName
        $manifestEntries = @()
        if (Test-ObjectProperty -InputObject $manifest -Name "files" -and $null -ne $manifest.files) {
            $manifestEntries = @($manifest.files)
        } else {
            $issues.Add((New-ValidationIssue -Code "manifest-files" -Message "metadata-manifest.json is missing the files array."))
        }

        $entriesById = @{}
        foreach ($entry in $manifestEntries) {
            $entriesById[$entry.id] = $entry
        }

        $missingRequiredIds = @(
            $requiredIds |
                Where-Object { -not $entriesById.ContainsKey($_) }
        )

        foreach ($fileId in $missingRequiredIds) {
            $issues.Add((New-ValidationIssue -Code "missing-required-file" -Message ("Required manifest entry was not found: {0}" -f $fileId) -FileId $fileId))
        }

        foreach ($entry in $manifestEntries) {
            $entryStatus = "validated"
            $messages = New-Object System.Collections.Generic.List[string]
            $resolvedFilePath = Join-Path $outputRootPath $entry.path

            if (-not (Test-Path $resolvedFilePath)) {
                $entryStatus = "missing"
                $messages.Add("Declared file does not exist on disk.")
                $issues.Add((New-ValidationIssue -Code "missing-file" -Message ("Declared file does not exist: {0}" -f $entry.path) -FileId $entry.id))
            } elseif ($entry.format -eq "json") {
                try {
                    $payload = Get-Content -Path $resolvedFilePath -Raw | ConvertFrom-Json
                } catch {
                    $payload = $null
                    $entryStatus = "invalid"
                    $messages.Add("JSON could not be parsed.")
                    $issues.Add((New-ValidationIssue -Code "invalid-json" -Message ("JSON parse failed: {0}" -f $_.Exception.Message) -FileId $entry.id))
                }

                if ($null -ne $payload) {
                    if (-not (Test-ObjectProperty -InputObject $payload -Name "schemaVersion")) {
                        $entryStatus = "invalid"
                        $messages.Add("schemaVersion is missing.")
                        $issues.Add((New-ValidationIssue -Code "missing-schema-version" -Message "JSON payload is missing schemaVersion." -FileId $entry.id))
                    } elseif ([int]$payload.schemaVersion -ne [int]$entry.schemaVersion) {
                        $entryStatus = "invalid"
                        $messages.Add(("schemaVersion mismatch: declared={0}, actual={1}" -f $entry.schemaVersion, $payload.schemaVersion))
                        $issues.Add((New-ValidationIssue -Code "schema-version-mismatch" -Message ("Declared schemaVersion {0} does not match payload schemaVersion {1}." -f $entry.schemaVersion, $payload.schemaVersion) -FileId $entry.id))
                    }

                    if ((Test-ObjectProperty -InputObject $payload -Name "workflow") -and $null -ne $payload.workflow -and `
                        (Test-ObjectProperty -InputObject $payload.workflow -Name "name") -and `
                        -not [string]::IsNullOrWhiteSpace($workflowName) -and `
                        $workflowName -ne "unknown" -and `
                        $payload.workflow.name -ne $workflowName) {
                        $entryStatus = "invalid"
                        $messages.Add(("workflow.name mismatch: expected={0}, actual={1}" -f $workflowName, $payload.workflow.name))
                        $issues.Add((New-ValidationIssue -Code "workflow-name-mismatch" -Message ("Payload workflow.name {0} does not match manifest workflow.name {1}." -f $payload.workflow.name, $workflowName) -FileId $entry.id))
                    }
                }
            }

            $result = [ordered]@{
                id = $entry.id
                path = $entry.path
                format = $entry.format
                role = $entry.role
                required = ($requiredIds -contains $entry.id)
                status = $entryStatus
            }

            if ($messages.Count -gt 0) {
                $result.messages = @($messages)
            }

            $fileResults.Add([pscustomobject]$result)
        }

        $declaredFileCount = @($manifestEntries).Count
        $validatedFileCount = @($fileResults | Where-Object { $_.status -eq "validated" }).Count
        $missingFileCount = @($fileResults | Where-Object { $_.status -eq "missing" }).Count
        $invalidFileCount = @($fileResults | Where-Object { $_.status -eq "invalid" }).Count

        $validationStatus = if ($issues.Count -eq 0) { "succeeded" } else { "failed" }
    }
}

if ($null -eq $manifest) {
    $requiredIds = @()
    $missingRequiredIds = @()
    $declaredFileCount = 0
    $validatedFileCount = 0
    $missingFileCount = 0
    $invalidFileCount = 0
}

$fileResultArray = if ($fileResults.Count -gt 0) {
    @($fileResults.ToArray())
} else {
    @()
}

$issueArray = if ($issues.Count -gt 0) {
    @($issues.ToArray())
} else {
    @()
}

$validationPayload = [ordered]@{
    schemaVersion = 1
    generatedAtUtc = (Get-Date).ToUniversalTime().ToString("o")
    workflow = [ordered]@{
        name = $workflowName
        metadataRoot = $outputRootPath
    }
    status = $validationStatus
    requiredFileIds = $requiredIds
    missingRequiredIds = $missingRequiredIds
    declaredFileCount = $declaredFileCount
    validatedFileCount = $validatedFileCount
    missingFileCount = $missingFileCount
    invalidFileCount = $invalidFileCount
    files = $fileResultArray
    issues = $issueArray
}

foreach ($key in @($workflowData.Keys | Sort-Object)) {
    $validationPayload.workflow[$key] = $workflowData[$key]
}

$summaryMarkdown = New-ValidationSummaryMarkdown `
    -WorkflowName $workflowName `
    -Status $validationStatus `
    -DeclaredFileCount $declaredFileCount `
    -ValidatedFileCount $validatedFileCount `
    -MissingFileCount $missingFileCount `
    -InvalidFileCount $invalidFileCount `
    -MissingRequiredIds $missingRequiredIds `
    -Issues $issueArray

$validationPayload | ConvertTo-Json -Depth 8 | Set-Content -Path $validationJsonPath
Set-Content -Path $validationMarkdownPath -Value $summaryMarkdown

if ($null -ne $manifest) {
    $metadataManifestPath = Update-MetadataManifest `
        -OutputRoot $outputRootPath `
        -WorkflowName $workflowName `
        -WorkflowData $workflowData `
        -Entries @(
            (New-MetadataManifestEntry -OutputRoot $outputRootPath -Id "metadataValidation" -Path "metadata-validation.json" -Format "json" -Role "metadata-validation"),
            (New-MetadataManifestEntry -OutputRoot $outputRootPath -Id "metadataValidationMarkdown" -Path "metadata-validation.md" -Format "markdown" -Role "metadata-validation")
        )
} else {
    $metadataManifestPath = $manifestPath
}

if (-not [string]::IsNullOrWhiteSpace($env:GITHUB_STEP_SUMMARY)) {
    Add-Content -Path $env:GITHUB_STEP_SUMMARY -Value $summaryMarkdown
}

Write-Host "Validated CI metadata:"
Write-Host "  status=$validationStatus"
Write-Host "  $validationJsonPath"
Write-Host "  $validationMarkdownPath"
Write-Host "  $metadataManifestPath"

if ($validationStatus -ne "succeeded") {
    $issueMessages = @($issueArray | ForEach-Object { $_.message })
    Write-Error ($issueMessages -join [Environment]::NewLine)
    exit 1
}
