param(
    [string]$BuildDir = "build",
    [string]$Config = "Debug",
    [ValidateSet("all", "build", "test")]
    [string]$Phase = "all"
)

$ErrorActionPreference = "Stop"

switch ($Phase) {
    "all" {
        Write-Host "Running Tinalux smoke tests from '$BuildDir' with config '$Config'..."
        & cmake --build $BuildDir --config $Config --target TinaluxRunDesktopSmokeTests
    }
    "build" {
        Write-Host "Building Tinalux smoke tests from '$BuildDir' with config '$Config'..."
        & cmake --build $BuildDir --config $Config --target TinaluxBuildSmokeTests
    }
    "test" {
        Write-Host "Executing Tinalux desktop smoke tests from '$BuildDir' with config '$Config'..."
        & ctest --test-dir $BuildDir --output-on-failure --timeout 60 -LE android-scripts -C $Config
    }
    default {
        throw "Unsupported smoke phase: $Phase"
    }
}

if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}
