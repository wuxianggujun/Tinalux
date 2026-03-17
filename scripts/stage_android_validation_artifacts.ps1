param(
    [Parameter(Mandatory = $true)]
    [string]$SourceLibrary,

    [Parameter(Mandatory = $true)]
    [string]$Abi,

    [string]$SourceIcuData = "",

    [string]$ValidationAppRoot = ""
)

$ErrorActionPreference = "Stop"

function Resolve-NormalizedPath {
    param([string]$PathValue)
    return [System.IO.Path]::GetFullPath((Resolve-Path $PathValue).Path)
}

if ([string]::IsNullOrWhiteSpace($ValidationAppRoot)) {
    $ValidationAppRoot = Join-Path $PSScriptRoot "..\\android\\validation-app"
}

$validationAppPath = [System.IO.Path]::GetFullPath($ValidationAppRoot)
if (-not (Test-Path $validationAppPath)) {
    throw "Validation app root not found: $validationAppPath"
}

$sourceLibraryPath = Resolve-NormalizedPath $SourceLibrary
if (-not (Test-Path $sourceLibraryPath)) {
    throw "Source library not found: $sourceLibraryPath"
}

$destinationLibDir = Join-Path $validationAppPath "app\\src\\main\\jniLibs\\$Abi"
New-Item -ItemType Directory -Force -Path $destinationLibDir | Out-Null

$destinationLibPath = Join-Path $destinationLibDir "libtinalux_native.so"
Copy-Item -Force $sourceLibraryPath $destinationLibPath

Write-Host "Staged native library:"
Write-Host "  $destinationLibPath"

if (-not [string]::IsNullOrWhiteSpace($SourceIcuData)) {
    $sourceIcuDataPath = Resolve-NormalizedPath $SourceIcuData
    if (-not (Test-Path $sourceIcuDataPath)) {
        throw "Source ICU data not found: $sourceIcuDataPath"
    }

    $destinationAssetsDir = Join-Path $validationAppPath "app\\src\\main\\assets"
    New-Item -ItemType Directory -Force -Path $destinationAssetsDir | Out-Null

    $destinationIcuDataPath = Join-Path $destinationAssetsDir "icudtl.dat"
    Copy-Item -Force $sourceIcuDataPath $destinationIcuDataPath

    Write-Host "Staged optional ICU data:"
    Write-Host "  $destinationIcuDataPath"
}

Write-Host ""
Write-Host "Next steps:"
Write-Host "  1. Open android/validation-app in Android Studio"
Write-Host "  2. Sync Gradle"
Write-Host "  3. Run MainActivity or VulkanValidationActivity on device"
