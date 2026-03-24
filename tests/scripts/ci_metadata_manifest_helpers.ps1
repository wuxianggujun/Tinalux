function New-MetadataManifestEntry {
    param(
        [Parameter(Mandatory = $true)]
        [string]$OutputRoot,
        [Parameter(Mandatory = $true)]
        [string]$Id,
        [Parameter(Mandatory = $true)]
        [string]$Path,
        [Parameter(Mandatory = $true)]
        [string]$Format,
        [Parameter(Mandatory = $true)]
        [string]$Role,
        [int]$SchemaVersion = 1
    )

    $outputRootPath = [System.IO.Path]::GetFullPath($OutputRoot)
    $resolvedPath = Join-Path $outputRootPath $Path

    return [ordered]@{
        id = $Id
        path = $Path
        format = $Format
        role = $Role
        exists = (Test-Path $resolvedPath)
        schemaVersion = $SchemaVersion
    }
}

function Update-MetadataManifest {
    param(
        [Parameter(Mandatory = $true)]
        [string]$OutputRoot,
        [Parameter(Mandatory = $true)]
        [string]$WorkflowName,
        [hashtable]$WorkflowData = @{},
        [object[]]$Entries = @()
    )

    $outputRootPath = [System.IO.Path]::GetFullPath($OutputRoot)
    New-Item -ItemType Directory -Force -Path $outputRootPath | Out-Null

    $manifestPath = Join-Path $outputRootPath "metadata-manifest.json"

    $workflow = [ordered]@{
        name = $WorkflowName
        metadataRoot = $outputRootPath
    }

    foreach ($key in @($WorkflowData.Keys | Sort-Object)) {
        $workflow[$key] = $WorkflowData[$key]
    }

    $existingEntries = [ordered]@{}
    if (Test-Path $manifestPath) {
        $existingManifest = Get-Content -Path $manifestPath -Raw | ConvertFrom-Json -AsHashtable
        foreach ($entry in @($existingManifest.files)) {
            $existingEntries[$entry.id] = [ordered]@{
                id = $entry.id
                path = $entry.path
                format = $entry.format
                role = $entry.role
                exists = $entry.exists
                schemaVersion = $entry.schemaVersion
            }
        }
    }

    foreach ($entry in @($Entries)) {
        $existingEntries[$entry.id] = $entry
    }

    $existingEntries["metadataManifest"] = New-MetadataManifestEntry `
        -OutputRoot $outputRootPath `
        -Id "metadataManifest" `
        -Path "metadata-manifest.json" `
        -Format "json" `
        -Role "metadata-manifest"

    $files = @(
        $existingEntries.GetEnumerator() |
            Sort-Object -Property Name |
            ForEach-Object {
                $entry = [ordered]@{
                    id = $_.Value.id
                    path = $_.Value.path
                    format = $_.Value.format
                    role = $_.Value.role
                    exists = if ($_.Value.id -eq "metadataManifest") {
                        $true
                    } else {
                        Test-Path (Join-Path $outputRootPath $_.Value.path)
                    }
                    schemaVersion = $_.Value.schemaVersion
                }

                [pscustomobject]$entry
            }
    )

    $manifest = [ordered]@{
        schemaVersion = 1
        generatedAtUtc = (Get-Date).ToUniversalTime().ToString("o")
        workflow = $workflow
        files = $files
    }

    $manifest | ConvertTo-Json -Depth 8 | Set-Content -Path $manifestPath
    return $manifestPath
}
