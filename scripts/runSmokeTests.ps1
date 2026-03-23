param(
    [string]$BuildDir = "build",
    [string]$Config = "Debug"
)

$ErrorActionPreference = "Stop"

Write-Host "Running Tinalux smoke tests from '$BuildDir' with config '$Config'..."
& cmake --build $BuildDir --config $Config --target TinaluxRunDesktopSmokeTests
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}
