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
        Path = Join-Path $repoRootPath ".github/workflows/linux-tina-glfw-x11.yml"
        Pattern = 'run:\s+\./tests/scripts/ci_metadata_contract_smoke\.ps1\s+-RepoRoot\s+\.'
        Description = 'Linux workflow metadata contract step'
    },
    @{
        Path = Join-Path $repoRootPath "docs/CI验证入口.md"
        Pattern = 'execution-summary\.json` 与 `execution-summary\.md'
        Description = 'CI docs execution summary naming'
    }
)

foreach ($check in $checks) {
    Assert-FileContainsPattern -Path $check.Path -Pattern $check.Pattern -Description $check.Description
}

Write-Host "Validated CI metadata contract:"
Write-Host "  tests/scripts/windows_desktop_smoke_stage_summary.ps1"
Write-Host "  tests/scripts/android_build_scripts_smoke_stage_summary.ps1"
Write-Host "  tests/scripts/linux_tina_glfw_x11_build_summary.ps1"
Write-Host "  .github/workflows/windows-desktop-smoke.yml"
Write-Host "  .github/workflows/android-build-scripts-smoke.yml"
Write-Host "  .github/workflows/linux-tina-glfw-x11.yml"
Write-Host "  docs/CI验证入口.md"
