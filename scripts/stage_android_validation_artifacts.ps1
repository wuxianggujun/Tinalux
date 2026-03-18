param(
    [string]$SourceLibrary = "",

    [Parameter(Mandatory = $true)]
    [ValidateSet("armeabi-v7a", "arm64-v8a", "x86", "x86_64")]
    [string]$Abi,

    [string]$SourceIcuData = "",

    [string]$SdkModuleRoot = "",

    [switch]$ValidateOnly
)

$ErrorActionPreference = "Stop"

function Resolve-NormalizedPath {
    param([string]$PathValue)
    if (-not (Test-Path $PathValue)) {
        throw "Path not found: $PathValue"
    }
    return [System.IO.Path]::GetFullPath((Resolve-Path $PathValue).Path)
}

function Assert-SdkModuleLayout {
    param([string]$SdkModulePath)

    $requiredPaths = @(
        "build.gradle.kts",
        "src\main\AndroidManifest.xml"
    )

    foreach ($relativePath in $requiredPaths) {
        $candidatePath = Join-Path $SdkModulePath $relativePath
        if (-not (Test-Path $candidatePath)) {
            throw "SDK module layout is incomplete. Missing required file: $candidatePath"
        }
    }
}

function Resolve-ValidatedLibraryPath {
    param([string]$LibraryPath)

    if ([string]::IsNullOrWhiteSpace($LibraryPath)) {
        throw "SourceLibrary is required unless -ValidateOnly is specified."
    }

    $resolvedLibraryPath = Resolve-NormalizedPath $LibraryPath
    $libraryInfo = Get-Item $resolvedLibraryPath
    if ($libraryInfo.Length -le 0) {
        throw "Source library is empty: $resolvedLibraryPath"
    }

    return $resolvedLibraryPath
}

function Get-StagedAbiNames {
    param([string]$SdkModulePath)

    $jniLibRoot = Join-Path $SdkModulePath "src\main\jniLibs"
    if (-not (Test-Path $jniLibRoot)) {
        return @()
    }

    return Get-ChildItem -Path $jniLibRoot -Directory |
        Where-Object { Test-Path (Join-Path $_.FullName "libtinalux_native.so") } |
        Sort-Object Name |
        ForEach-Object { $_.Name }
}

if ([string]::IsNullOrWhiteSpace($SdkModuleRoot)) {
    $SdkModuleRoot = Join-Path $PSScriptRoot "..\\android\\tinalux-sdk"
}

$sdkModulePath = [System.IO.Path]::GetFullPath($SdkModuleRoot)
if (-not (Test-Path $sdkModulePath)) {
    throw "SDK module root not found: $sdkModulePath"
}
Assert-SdkModuleLayout $sdkModulePath

$resolvedIcuDataPath = ""
if (-not [string]::IsNullOrWhiteSpace($SourceIcuData)) {
    $resolvedIcuDataPath = Resolve-NormalizedPath $SourceIcuData
}

if ($ValidateOnly.IsPresent) {
    Write-Host "Validated Android SDK module root:"
    Write-Host "  $sdkModulePath"

    if (-not [string]::IsNullOrWhiteSpace($SourceLibrary)) {
        $validatedLibraryPath = Resolve-ValidatedLibraryPath $SourceLibrary
        Write-Host "Validated source library:"
        Write-Host "  $validatedLibraryPath"
    }

    if (-not [string]::IsNullOrWhiteSpace($resolvedIcuDataPath)) {
        Write-Host "Validated optional ICU data:"
        Write-Host "  $resolvedIcuDataPath"
    }

    $stagedAbis = Get-StagedAbiNames $sdkModulePath
    Write-Host "Currently staged ABIs:"
    Write-Host "  $(if ($stagedAbis.Count -gt 0) { $stagedAbis -join ', ' } else { '(none)' })"
    return
}

$sourceLibraryPath = Resolve-ValidatedLibraryPath $SourceLibrary
$destinationLibDir = Join-Path $sdkModulePath "src\\main\\jniLibs\\$Abi"
New-Item -ItemType Directory -Force -Path $destinationLibDir | Out-Null

$destinationLibPath = Join-Path $destinationLibDir "libtinalux_native.so"
Copy-Item -Force $sourceLibraryPath $destinationLibPath

Write-Host "Staged native library:"
Write-Host "  $destinationLibPath"

if (-not [string]::IsNullOrWhiteSpace($resolvedIcuDataPath)) {
    $destinationAssetsDir = Join-Path $sdkModulePath "src\\main\\assets"
    New-Item -ItemType Directory -Force -Path $destinationAssetsDir | Out-Null

    $destinationIcuDataPath = Join-Path $destinationAssetsDir "icudtl.dat"
    Copy-Item -Force $resolvedIcuDataPath $destinationIcuDataPath

    Write-Host "Staged optional ICU data:"
    Write-Host "  $destinationIcuDataPath"
}

$stagedAbis = Get-StagedAbiNames $sdkModulePath
Write-Host "Staged ABIs:"
Write-Host "  $($stagedAbis -join ', ')"

Write-Host ""
Write-Host "Next steps:"
Write-Host "  1. Open android/validation-app in Android Studio"
Write-Host "  2. Sync Gradle"
Write-Host "  3. validation-app will pick up the staged tinalux-sdk artifacts"
Write-Host "  4. Run MainActivity or VulkanValidationActivity on device"
