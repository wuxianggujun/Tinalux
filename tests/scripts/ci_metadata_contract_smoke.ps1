param(
    [Parameter(Mandatory = $true)]
    [string]$RepoRoot
)

$ErrorActionPreference = "Stop"

$repoRootPath = [System.IO.Path]::GetFullPath((Resolve-Path $RepoRoot).Path)

function Assert-FileContainsPattern {
    param(
        [string]$Path,
        [string]$Pattern,
        [string]$Description
    )

    if (-not (Test-Path $Path)) {
        throw "CI metadata contract file was not found: $Path"
    }

    $content = Get-Content -Path $Path -Raw
    if ($content -notmatch $Pattern) {
        throw ("CI metadata contract drifted. Missing {0} in {1}" -f $Description, $Path)
    }
}

$checks = @(
    @{
        Path = Join-Path $repoRootPath "tests/scripts/windows_desktop_smoke_ci_metadata.ps1"
        Pattern = 'ci_metadata_manifest_helpers\.ps1'
        Description = 'Windows metadata helper wiring'
    },
    @{
        Path = Join-Path $repoRootPath "tests/scripts/windows_desktop_smoke_ci_metadata.ps1"
        Pattern = 'schemaVersion = 1'
        Description = 'Windows runner fingerprint schema version'
    },
    @{
        Path = Join-Path $repoRootPath "tests/scripts/windows_desktop_smoke_ci_metadata.ps1"
        Pattern = 'name = "windows-desktop-smoke"'
        Description = 'Windows runner fingerprint workflow name'
    },
    @{
        Path = Join-Path $repoRootPath "tests/scripts/windows_desktop_smoke_ci_metadata.ps1"
        Pattern = 'metadataRoot = \$outputRootPath'
        Description = 'Windows runner fingerprint metadata root'
    },
    @{
        Path = Join-Path $repoRootPath "tests/scripts/windows_desktop_smoke_ci_metadata.ps1"
        Pattern = 'Update-MetadataManifest'
        Description = 'Windows metadata manifest update'
    },
    @{
        Path = Join-Path $repoRootPath "tests/scripts/windows_desktop_smoke_test_timings.ps1"
        Pattern = 'ci_metadata_manifest_helpers\.ps1'
        Description = 'Windows test timings helper wiring'
    },
    @{
        Path = Join-Path $repoRootPath "tests/scripts/windows_desktop_smoke_test_timings.ps1"
        Pattern = 'schemaVersion = 1'
        Description = 'Windows test timings schema version'
    },
    @{
        Path = Join-Path $repoRootPath "tests/scripts/windows_desktop_smoke_test_timings.ps1"
        Pattern = 'Update-MetadataManifest'
        Description = 'Windows test timings manifest update'
    },
    @{
        Path = Join-Path $repoRootPath "tests/scripts/windows_desktop_smoke_test_timings.ps1"
        Pattern = 'workflow = \[ordered\]@\{\s+name = "windows-desktop-smoke"\s+metadataRoot = \$outputRootPath'
        Description = 'Windows test timings workflow block'
    },
    @{
        Path = Join-Path $repoRootPath "tests/scripts/android_build_scripts_smoke_ci_metadata.ps1"
        Pattern = 'ci_metadata_manifest_helpers\.ps1'
        Description = 'Android metadata helper wiring'
    },
    @{
        Path = Join-Path $repoRootPath "tests/scripts/android_build_scripts_smoke_ci_metadata.ps1"
        Pattern = 'schemaVersion = 1'
        Description = 'Android runner fingerprint schema version'
    },
    @{
        Path = Join-Path $repoRootPath "tests/scripts/android_build_scripts_smoke_ci_metadata.ps1"
        Pattern = 'name = "android-build-scripts-smoke"'
        Description = 'Android runner fingerprint workflow name'
    },
    @{
        Path = Join-Path $repoRootPath "tests/scripts/android_build_scripts_smoke_ci_metadata.ps1"
        Pattern = 'metadataRoot = \$outputRootPath'
        Description = 'Android runner fingerprint metadata root'
    },
    @{
        Path = Join-Path $repoRootPath "tests/scripts/android_build_scripts_smoke_ci_metadata.ps1"
        Pattern = 'Update-MetadataManifest'
        Description = 'Android metadata manifest update'
    },
    @{
        Path = Join-Path $repoRootPath "tests/scripts/android_build_scripts_smoke_stage_summary.ps1"
        Pattern = 'ci_metadata_manifest_helpers\.ps1'
        Description = 'Android execution summary helper wiring'
    },
    @{
        Path = Join-Path $repoRootPath "tests/scripts/linux_tina_glfw_x11_ci_metadata.ps1"
        Pattern = 'schemaVersion = 1'
        Description = 'Linux runner fingerprint schema version'
    },
    @{
        Path = Join-Path $repoRootPath "tests/scripts/linux_tina_glfw_x11_ci_metadata.ps1"
        Pattern = 'ci_metadata_manifest_helpers\.ps1'
        Description = 'Linux metadata helper wiring'
    },
    @{
        Path = Join-Path $repoRootPath "tests/scripts/linux_tina_glfw_x11_ci_metadata.ps1"
        Pattern = 'name = "linux-tina-glfw-x11"'
        Description = 'Linux runner fingerprint workflow name'
    },
    @{
        Path = Join-Path $repoRootPath "tests/scripts/linux_tina_glfw_x11_ci_metadata.ps1"
        Pattern = 'metadataRoot = \$outputRootPath'
        Description = 'Linux runner fingerprint metadata root'
    },
    @{
        Path = Join-Path $repoRootPath "tests/scripts/linux_tina_glfw_x11_ci_metadata.ps1"
        Pattern = 'Update-MetadataManifest'
        Description = 'Linux metadata manifest update'
    },
    @{
        Path = Join-Path $repoRootPath "tests/scripts/windows_desktop_smoke_stage_summary.ps1"
        Pattern = 'schemaVersion = 1'
        Description = 'Windows execution summary schema version'
    },
    @{
        Path = Join-Path $repoRootPath "tests/scripts/windows_desktop_smoke_stage_summary.ps1"
        Pattern = 'Join-Path \$outputRootPath "execution-summary\.json"'
        Description = 'Windows execution summary json filename'
    },
    @{
        Path = Join-Path $repoRootPath "tests/scripts/windows_desktop_smoke_stage_summary.ps1"
        Pattern = 'Join-Path \$outputRootPath "execution-summary\.md"'
        Description = 'Windows execution summary markdown filename'
    },
    @{
        Path = Join-Path $repoRootPath "tests/scripts/windows_desktop_smoke_stage_summary.ps1"
        Pattern = 'status = \$executionStatus'
        Description = 'Windows execution summary status field'
    },
    @{
        Path = Join-Path $repoRootPath "tests/scripts/windows_desktop_smoke_stage_summary.ps1"
        Pattern = 'steps = \$steps'
        Description = 'Windows execution summary steps field'
    },
    @{
        Path = Join-Path $repoRootPath "tests/scripts/windows_desktop_smoke_stage_summary.ps1"
        Pattern = 'Update-MetadataManifest'
        Description = 'Windows execution summary manifest update'
    },
    @{
        Path = Join-Path $repoRootPath "tests/scripts/android_build_scripts_smoke_stage_summary.ps1"
        Pattern = 'schemaVersion = 1'
        Description = 'Android execution summary schema version'
    },
    @{
        Path = Join-Path $repoRootPath "tests/scripts/android_build_scripts_smoke_stage_summary.ps1"
        Pattern = 'Join-Path \$outputRootPath "execution-summary\.json"'
        Description = 'Android execution summary json filename'
    },
    @{
        Path = Join-Path $repoRootPath "tests/scripts/android_build_scripts_smoke_stage_summary.ps1"
        Pattern = 'Join-Path \$outputRootPath "execution-summary\.md"'
        Description = 'Android execution summary markdown filename'
    },
    @{
        Path = Join-Path $repoRootPath "tests/scripts/android_build_scripts_smoke_stage_summary.ps1"
        Pattern = 'status = \$executionStatus'
        Description = 'Android execution summary status field'
    },
    @{
        Path = Join-Path $repoRootPath "tests/scripts/android_build_scripts_smoke_stage_summary.ps1"
        Pattern = 'steps = \$steps'
        Description = 'Android execution summary steps field'
    },
    @{
        Path = Join-Path $repoRootPath "tests/scripts/android_build_scripts_smoke_stage_summary.ps1"
        Pattern = 'Update-MetadataManifest'
        Description = 'Android execution summary manifest update'
    },
    @{
        Path = Join-Path $repoRootPath "tests/scripts/linux_tina_glfw_x11_build_summary.ps1"
        Pattern = 'schemaVersion = 1'
        Description = 'Linux execution summary schema version'
    },
    @{
        Path = Join-Path $repoRootPath "tests/scripts/linux_tina_glfw_x11_build_summary.ps1"
        Pattern = 'ci_metadata_manifest_helpers\.ps1'
        Description = 'Linux execution summary helper wiring'
    },
    @{
        Path = Join-Path $repoRootPath "tests/scripts/linux_tina_glfw_x11_build_summary.ps1"
        Pattern = 'Join-Path \$outputRootPath "execution-summary\.json"'
        Description = 'Linux execution summary json filename'
    },
    @{
        Path = Join-Path $repoRootPath "tests/scripts/linux_tina_glfw_x11_build_summary.ps1"
        Pattern = 'Join-Path \$outputRootPath "execution-summary\.md"'
        Description = 'Linux execution summary markdown filename'
    },
    @{
        Path = Join-Path $repoRootPath "tests/scripts/linux_tina_glfw_x11_build_summary.ps1"
        Pattern = 'status = \$executionStatus'
        Description = 'Linux execution summary status field'
    },
    @{
        Path = Join-Path $repoRootPath "tests/scripts/linux_tina_glfw_x11_build_summary.ps1"
        Pattern = 'steps = \$steps'
        Description = 'Linux execution summary steps field'
    },
    @{
        Path = Join-Path $repoRootPath "tests/scripts/linux_tina_glfw_x11_build_summary.ps1"
        Pattern = 'Update-MetadataManifest'
        Description = 'Linux execution summary manifest update'
    },
    @{
        Path = Join-Path $repoRootPath "tests/scripts/windows_desktop_smoke_stage_summary.ps1"
        Pattern = 'workflow = \[ordered\]@\{\s+name = "windows-desktop-smoke"\s+metadataRoot = \$outputRootPath'
        Description = 'Windows execution summary workflow block'
    },
    @{
        Path = Join-Path $repoRootPath "tests/scripts/android_build_scripts_smoke_stage_summary.ps1"
        Pattern = 'workflow = \[ordered\]@\{\s+name = "android-build-scripts-smoke"\s+metadataRoot = \$outputRootPath'
        Description = 'Android execution summary workflow block'
    },
    @{
        Path = Join-Path $repoRootPath "tests/scripts/linux_tina_glfw_x11_build_summary.ps1"
        Pattern = 'workflow = \[ordered\]@\{\s+name = "linux-tina-glfw-x11"\s+metadataRoot = \$outputRootPath'
        Description = 'Linux execution summary workflow block'
    },
    @{
        Path = Join-Path $repoRootPath ".github/workflows/windows-desktop-smoke.yml"
        Pattern = 'run:\s+\./tests/scripts/ci_metadata_contract_smoke\.ps1\s+-RepoRoot\s+\.'
        Description = 'Windows workflow metadata contract step'
    },
    @{
        Path = Join-Path $repoRootPath ".github/workflows/android-build-scripts-smoke.yml"
        Pattern = 'run:\s+\./tests/scripts/ci_metadata_contract_smoke\.ps1\s+-RepoRoot\s+\.'
        Description = 'Android workflow metadata contract step'
    },
    @{
        Path = Join-Path $repoRootPath ".github/workflows/android-build-scripts-smoke.yml"
        Pattern = '"tests/scripts/ci_metadata_manifest_helpers\.ps1"'
        Description = 'Android workflow metadata helper trigger'
    },
    @{
        Path = Join-Path $repoRootPath ".github/workflows/linux-tina-glfw-x11.yml"
        Pattern = 'run:\s+\./tests/scripts/ci_metadata_contract_smoke\.ps1\s+-RepoRoot\s+\.'
        Description = 'Linux workflow metadata contract step'
    },
    @{
        Path = Join-Path $repoRootPath ".github/workflows/linux-tina-glfw-x11.yml"
        Pattern = '"tests/scripts/ci_metadata_manifest_helpers\.ps1"'
        Description = 'Linux workflow metadata helper trigger'
    },
    @{
        Path = Join-Path $repoRootPath "docs/CI验证入口.md"
        Pattern = 'execution-summary\.json` 与 `execution-summary\.md'
        Description = 'CI docs execution summary naming'
    },
    @{
        Path = Join-Path $repoRootPath "docs/CI验证入口.md"
        Pattern = 'runner-fingerprint\.json` / `execution-summary\.json`'
        Description = 'CI docs schema alignment mention'
    },
    @{
        Path = Join-Path $repoRootPath "docs/CI验证入口.md"
        Pattern = 'metadata-manifest\.json'
        Description = 'CI docs manifest mention'
    },
    @{
        Path = Join-Path $repoRootPath "docs/开发计划.md"
        Pattern = 'runner-fingerprint\.json` / `execution-summary\.json`'
        Description = 'plan docs schema alignment mention'
    },
    @{
        Path = Join-Path $repoRootPath "docs/CI验证入口.md"
        Pattern = 'schemaVersion = 1'
        Description = 'CI docs schema version mention'
    },
    @{
        Path = Join-Path $repoRootPath "docs/开发计划.md"
        Pattern = 'schemaVersion = 1'
        Description = 'plan docs schema version mention'
    }
)

foreach ($check in $checks) {
    Assert-FileContainsPattern -Path $check.Path -Pattern $check.Pattern -Description $check.Description
}

Write-Host "Validated CI metadata contract:"
Write-Host "  tests/scripts/windows_desktop_smoke_ci_metadata.ps1"
Write-Host "  tests/scripts/android_build_scripts_smoke_ci_metadata.ps1"
Write-Host "  tests/scripts/linux_tina_glfw_x11_ci_metadata.ps1"
Write-Host "  tests/scripts/windows_desktop_smoke_stage_summary.ps1"
Write-Host "  tests/scripts/windows_desktop_smoke_test_timings.ps1"
Write-Host "  tests/scripts/android_build_scripts_smoke_stage_summary.ps1"
Write-Host "  tests/scripts/linux_tina_glfw_x11_build_summary.ps1"
Write-Host "  .github/workflows/windows-desktop-smoke.yml"
Write-Host "  .github/workflows/android-build-scripts-smoke.yml"
Write-Host "  .github/workflows/linux-tina-glfw-x11.yml"
Write-Host "  docs/CI验证入口.md"
Write-Host "  docs/开发计划.md"
