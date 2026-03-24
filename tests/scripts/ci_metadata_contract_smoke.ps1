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
        Path = Join-Path $repoRootPath "tests/scripts/ci_metadata_validate.ps1"
        Pattern = 'metadata-validation\.json'
        Description = 'metadata validation json output'
    },
    @{
        Path = Join-Path $repoRootPath "tests/scripts/ci_metadata_validate.ps1"
        Pattern = 'Update-MetadataManifest'
        Description = 'metadata validation manifest update'
    },
    @{
        Path = Join-Path $repoRootPath "tests/scripts/ci_metadata_validate.ps1"
        Pattern = 'schemaVersion = 1'
        Description = 'metadata validation schema version'
    },
    @{
        Path = Join-Path $repoRootPath "tests/scripts/ci_metadata_validate.ps1"
        Pattern = '"thresholdCheck"'
        Description = 'metadata validation threshold check requirement'
    },
    @{
        Path = Join-Path $repoRootPath "tests/scripts/ci_metadata_validate.ps1"
        Pattern = '"thresholdCheckMarkdown"'
        Description = 'metadata validation threshold markdown requirement'
    },
    @{
        Path = Join-Path $repoRootPath "tests/scripts/ci_execution_summary_threshold_guard.ps1"
        Pattern = 'ci_metadata_manifest_helpers\.ps1'
        Description = 'execution summary threshold guard helper wiring'
    },
    @{
        Path = Join-Path $repoRootPath "tests/scripts/ci_execution_summary_threshold_guard.ps1"
        Pattern = 'schemaVersion = 1'
        Description = 'execution summary threshold guard schema version'
    },
    @{
        Path = Join-Path $repoRootPath "tests/scripts/ci_execution_summary_threshold_guard.ps1"
        Pattern = 'Join-Path \$outputRootPath "threshold-check\.json"'
        Description = 'execution summary threshold guard json filename'
    },
    @{
        Path = Join-Path $repoRootPath "tests/scripts/ci_execution_summary_threshold_guard.ps1"
        Pattern = 'Join-Path \$outputRootPath "threshold-check\.md"'
        Description = 'execution summary threshold guard markdown filename'
    },
    @{
        Path = Join-Path $repoRootPath "tests/scripts/ci_execution_summary_threshold_guard.ps1"
        Pattern = 'Update-MetadataManifest'
        Description = 'execution summary threshold guard manifest update'
    },
    @{
        Path = Join-Path $repoRootPath "tests/scripts/ci_execution_summary_threshold_guard.ps1"
        Pattern = 'mode = "warning-only"'
        Description = 'execution summary threshold guard warning-only mode'
    },
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
        Path = Join-Path $repoRootPath "tests/scripts/windows_desktop_smoke_threshold_guard.ps1"
        Pattern = 'ci_metadata_manifest_helpers\.ps1'
        Description = 'Windows threshold guard helper wiring'
    },
    @{
        Path = Join-Path $repoRootPath "tests/scripts/windows_desktop_smoke_threshold_guard.ps1"
        Pattern = 'schemaVersion = 1'
        Description = 'Windows threshold guard schema version'
    },
    @{
        Path = Join-Path $repoRootPath "tests/scripts/windows_desktop_smoke_threshold_guard.ps1"
        Pattern = 'Join-Path \$outputRootPath "threshold-check\.json"'
        Description = 'Windows threshold guard json filename'
    },
    @{
        Path = Join-Path $repoRootPath "tests/scripts/windows_desktop_smoke_threshold_guard.ps1"
        Pattern = 'Join-Path \$outputRootPath "threshold-check\.md"'
        Description = 'Windows threshold guard markdown filename'
    },
    @{
        Path = Join-Path $repoRootPath "tests/scripts/windows_desktop_smoke_threshold_guard.ps1"
        Pattern = 'Update-MetadataManifest'
        Description = 'Windows threshold guard manifest update'
    },
    @{
        Path = Join-Path $repoRootPath "tests/scripts/windows_desktop_smoke_threshold_guard.ps1"
        Pattern = 'mode = "warning-only"'
        Description = 'Windows threshold guard warning-only mode'
    },
    @{
        Path = Join-Path $repoRootPath "tests/scripts/windows_desktop_smoke_threshold_guard.ps1"
        Pattern = 'workflow = \[ordered\]@\{\s+name = "windows-desktop-smoke"\s+metadataRoot = \$outputRootPath'
        Description = 'Windows threshold guard workflow block'
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
        Path = Join-Path $repoRootPath ".github/workflows/windows-desktop-smoke.yml"
        Pattern = 'name:\s+Validate Windows Smoke Metadata'
        Description = 'Windows workflow metadata validation step'
    },
    @{
        Path = Join-Path $repoRootPath ".github/workflows/windows-desktop-smoke.yml"
        Pattern = 'name:\s+Capture Windows Smoke Threshold Warnings'
        Description = 'Windows workflow threshold guard step'
    },
    @{
        Path = Join-Path $repoRootPath ".github/workflows/android-build-scripts-smoke.yml"
        Pattern = 'run:\s+\./tests/scripts/ci_metadata_contract_smoke\.ps1\s+-RepoRoot\s+\.'
        Description = 'Android workflow metadata contract step'
    },
    @{
        Path = Join-Path $repoRootPath ".github/workflows/android-build-scripts-smoke.yml"
        Pattern = 'name:\s+Validate Android Smoke Metadata'
        Description = 'Android workflow metadata validation step'
    },
    @{
        Path = Join-Path $repoRootPath ".github/workflows/android-build-scripts-smoke.yml"
        Pattern = 'name:\s+Capture Android Smoke Threshold Warnings'
        Description = 'Android workflow threshold guard step'
    },
    @{
        Path = Join-Path $repoRootPath ".github/workflows/android-build-scripts-smoke.yml"
        Pattern = '"tests/scripts/ci_metadata_manifest_helpers\.ps1"'
        Description = 'Android workflow metadata helper trigger'
    },
    @{
        Path = Join-Path $repoRootPath ".github/workflows/android-build-scripts-smoke.yml"
        Pattern = '"tests/scripts/ci_metadata_validate\.ps1"'
        Description = 'Android workflow metadata validation trigger'
    },
    @{
        Path = Join-Path $repoRootPath ".github/workflows/android-build-scripts-smoke.yml"
        Pattern = '"tests/scripts/ci_execution_summary_threshold_guard\.ps1"'
        Description = 'Android workflow threshold trigger'
    },
    @{
        Path = Join-Path $repoRootPath ".github/workflows/linux-tina-glfw-x11.yml"
        Pattern = 'run:\s+\./tests/scripts/ci_metadata_contract_smoke\.ps1\s+-RepoRoot\s+\.'
        Description = 'Linux workflow metadata contract step'
    },
    @{
        Path = Join-Path $repoRootPath ".github/workflows/linux-tina-glfw-x11.yml"
        Pattern = 'name:\s+Validate Linux X11 Metadata'
        Description = 'Linux workflow metadata validation step'
    },
    @{
        Path = Join-Path $repoRootPath ".github/workflows/linux-tina-glfw-x11.yml"
        Pattern = 'name:\s+Capture Linux X11 Threshold Warnings'
        Description = 'Linux workflow threshold guard step'
    },
    @{
        Path = Join-Path $repoRootPath ".github/workflows/linux-tina-glfw-x11.yml"
        Pattern = '"tests/scripts/ci_metadata_manifest_helpers\.ps1"'
        Description = 'Linux workflow metadata helper trigger'
    },
    @{
        Path = Join-Path $repoRootPath ".github/workflows/linux-tina-glfw-x11.yml"
        Pattern = '"tests/scripts/ci_metadata_validate\.ps1"'
        Description = 'Linux workflow metadata validation trigger'
    },
    @{
        Path = Join-Path $repoRootPath ".github/workflows/linux-tina-glfw-x11.yml"
        Pattern = '"tests/scripts/ci_execution_summary_threshold_guard\.ps1"'
        Description = 'Linux workflow threshold trigger'
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
        Path = Join-Path $repoRootPath "docs/CI验证入口.md"
        Pattern = 'metadata-validation\.json'
        Description = 'CI docs validation mention'
    },
    @{
        Path = Join-Path $repoRootPath "docs/CI验证入口.md"
        Pattern = 'threshold-check\.json'
        Description = 'CI docs threshold guard mention'
    },
    @{
        Path = Join-Path $repoRootPath "docs/开发计划.md"
        Pattern = 'runner-fingerprint\.json` / `execution-summary\.json`'
        Description = 'plan docs schema alignment mention'
    },
    @{
        Path = Join-Path $repoRootPath "docs/开发计划.md"
        Pattern = 'metadata-validation\.json'
        Description = 'plan docs validation mention'
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
    },
    @{
        Path = Join-Path $repoRootPath "docs/开发计划.md"
        Pattern = 'threshold-check\.json'
        Description = 'plan docs threshold guard mention'
    }
)

foreach ($check in $checks) {
    Assert-FileContainsPattern -Path $check.Path -Pattern $check.Pattern -Description $check.Description
}

Write-Host "Validated CI metadata contract:"
Write-Host "  tests/scripts/ci_execution_summary_threshold_guard.ps1"
Write-Host "  tests/scripts/windows_desktop_smoke_ci_metadata.ps1"
Write-Host "  tests/scripts/android_build_scripts_smoke_ci_metadata.ps1"
Write-Host "  tests/scripts/linux_tina_glfw_x11_ci_metadata.ps1"
Write-Host "  tests/scripts/windows_desktop_smoke_stage_summary.ps1"
Write-Host "  tests/scripts/windows_desktop_smoke_test_timings.ps1"
Write-Host "  tests/scripts/windows_desktop_smoke_threshold_guard.ps1"
Write-Host "  tests/scripts/android_build_scripts_smoke_stage_summary.ps1"
Write-Host "  tests/scripts/linux_tina_glfw_x11_build_summary.ps1"
Write-Host "  .github/workflows/windows-desktop-smoke.yml"
Write-Host "  .github/workflows/android-build-scripts-smoke.yml"
Write-Host "  .github/workflows/linux-tina-glfw-x11.yml"
Write-Host "  docs/CI验证入口.md"
Write-Host "  docs/开发计划.md"
