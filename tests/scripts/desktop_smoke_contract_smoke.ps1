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
        throw "Desktop smoke contract file was not found: $Path"
    }

    $content = Get-Content -Path $Path -Raw
    if ($content -notmatch $Pattern) {
        throw ("Desktop smoke contract drifted. Missing {0} in {1}" -f $Description, $Path)
    }
}

$checks = @(
    @{
        Path = Join-Path $repoRootPath "scripts/runSmokeTests.ps1"
        Pattern = '--target\s+TinaluxRunDesktopSmokeTests'
        Description = 'PowerShell desktop smoke target'
    },
    @{
        Path = Join-Path $repoRootPath "scripts/runSmokeTests.sh"
        Pattern = '--target\s+TinaluxRunDesktopSmokeTests'
        Description = 'shell desktop smoke target'
    },
    @{
        Path = Join-Path $repoRootPath "tests/CMakeLists.txt"
        Pattern = 'TinaluxAndroidStageScriptSmoke\s*PROPERTIES LABELS "android;android-scripts"'
        Description = 'Android stage script label contract'
    },
    @{
        Path = Join-Path $repoRootPath "tests/CMakeLists.txt"
        Pattern = 'TinaluxAndroidBuildValidateScriptSmoke\s*PROPERTIES LABELS "android;android-scripts"'
        Description = 'Android build validate script label contract'
    },
    @{
        Path = Join-Path $repoRootPath "tests/CMakeLists.txt"
        Pattern = 'add_custom_target\(TinaluxDesktopSmokeContract'
        Description = 'desktop smoke contract target'
    },
    @{
        Path = Join-Path $repoRootPath "tests/CMakeLists.txt"
        Pattern = 'list\(APPEND _tinalux_desktop_smoke_test_dependencies TinaluxDesktopSmokeContract\)'
        Description = 'desktop smoke contract dependency wiring'
    },
    @{
        Path = Join-Path $repoRootPath "tests/CMakeLists.txt"
        Pattern = 'add_custom_target\(TinaluxRunDesktopSmokeTests[\s\S]*?-LE android-scripts \$\{_tinalux_ctest_config_args\}'
        Description = 'desktop smoke ctest exclusion'
    },
    @{
        Path = Join-Path $repoRootPath ".github/workflows/windows-desktop-smoke.yml"
        Pattern = '"scripts/runSmokeTests\.sh"'
        Description = 'Windows smoke workflow shell runner trigger'
    },
    @{
        Path = Join-Path $repoRootPath ".github/workflows/windows-desktop-smoke.yml"
        Pattern = 'run:\s+\./tests/scripts/desktop_smoke_contract_smoke\.ps1\s+-RepoRoot\s+\.'
        Description = 'Windows smoke workflow contract step'
    }
)

foreach ($check in $checks) {
    Assert-FileContainsPattern -Path $check.Path -Pattern $check.Pattern -Description $check.Description
}

Write-Host "Validated desktop smoke contract:"
Write-Host "  scripts/runSmokeTests.ps1"
Write-Host "  scripts/runSmokeTests.sh"
Write-Host "  tests/CMakeLists.txt"
Write-Host "  .github/workflows/windows-desktop-smoke.yml"
