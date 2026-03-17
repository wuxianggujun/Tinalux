param(
    [string]$Abi = "arm64-v8a",
    [int]$ApiLevel = 26,
    [ValidateSet("Debug", "Release")]
    [string]$BuildType = "Release",
    [string]$NdkPath = "",
    [string]$Generator = "Ninja",
    [string]$BuildRoot = "",
    [switch]$StageToSdk,
    [string]$SdkModuleRoot = "",
    [string]$SourceIcuData = ""
)

$ErrorActionPreference = "Stop"

function Resolve-FullPath {
    param([string]$PathValue)
    if (-not (Test-Path $PathValue)) {
        throw "Path not found: $PathValue"
    }
    return [System.IO.Path]::GetFullPath((Resolve-Path $PathValue).Path)
}

$repoRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
if ([string]::IsNullOrWhiteSpace($BuildRoot)) {
    $BuildRoot = Join-Path $repoRoot ("build-android-{0}-{1}" -f $Abi, $BuildType.ToLowerInvariant())
}
$BuildRoot = [System.IO.Path]::GetFullPath($BuildRoot)

if ([string]::IsNullOrWhiteSpace($NdkPath)) {
    if (-not [string]::IsNullOrWhiteSpace($env:ANDROID_NDK_ROOT)) {
        $NdkPath = $env:ANDROID_NDK_ROOT
    } elseif (-not [string]::IsNullOrWhiteSpace($env:ANDROID_NDK_HOME)) {
        $NdkPath = $env:ANDROID_NDK_HOME
    }
}

if ([string]::IsNullOrWhiteSpace($NdkPath)) {
    throw "ANDROID_NDK_ROOT / ANDROID_NDK_HOME is not set. Pass -NdkPath explicitly."
}

$NdkPath = Resolve-FullPath $NdkPath
$toolchainFile = Join-Path $NdkPath "build\cmake\android.toolchain.cmake"
if (-not (Test-Path $toolchainFile)) {
    throw "Android toolchain file not found: $toolchainFile"
}

$configureArgs = @(
    "-S", $repoRoot,
    "-B", $BuildRoot,
    "-G", $Generator,
    "-DANDROID=ON",
    "-DCMAKE_TOOLCHAIN_FILE=$toolchainFile",
    "-DANDROID_ABI=$Abi",
    "-DANDROID_PLATFORM=android-$ApiLevel",
    "-DCMAKE_BUILD_TYPE=$BuildType",
    "-DANDROID_STL=c++_static",
    "-DTINALUX_BUILD_TESTS=OFF"
)

Write-Host "Configuring Android native build:"
Write-Host "  Repo:       $repoRoot"
Write-Host "  BuildRoot:  $BuildRoot"
Write-Host "  ABI:        $Abi"
Write-Host "  API Level:  android-$ApiLevel"
Write-Host "  BuildType:  $BuildType"
Write-Host "  NDK:        $NdkPath"
Write-Host ""

& cmake @configureArgs
if ($LASTEXITCODE -ne 0) {
    throw "CMake configure failed."
}

& cmake --build $BuildRoot --config $BuildType --target TinaluxAndroidNative
if ($LASTEXITCODE -ne 0) {
    throw "CMake build failed."
}

$library = Get-ChildItem -Path $BuildRoot -Filter "libtinalux_native.so" -Recurse -File |
    Sort-Object LastWriteTimeUtc -Descending |
    Select-Object -First 1

if ($null -eq $library) {
    throw "Failed to locate libtinalux_native.so under $BuildRoot"
}

Write-Host ""
Write-Host "Built native library:"
Write-Host "  $($library.FullName)"

if ($StageToSdk.IsPresent) {
    $stageScript = Join-Path $PSScriptRoot "stage_android_validation_artifacts.ps1"
    $stageArgs = @(
        "-ExecutionPolicy", "Bypass",
        "-File", $stageScript,
        "-SourceLibrary", $library.FullName,
        "-Abi", $Abi
    )
    if (-not [string]::IsNullOrWhiteSpace($SdkModuleRoot)) {
        $stageArgs += @("-SdkModuleRoot", $SdkModuleRoot)
    }
    if (-not [string]::IsNullOrWhiteSpace($SourceIcuData)) {
        $stageArgs += @("-SourceIcuData", $SourceIcuData)
    }

    Write-Host ""
    Write-Host "Staging Android SDK artifacts..."
    & powershell @stageArgs
    if ($LASTEXITCODE -ne 0) {
        throw "Artifact staging failed."
    }
}

Write-Host ""
Write-Host "Done."
Write-Host "  Native output: $($library.FullName)"
if (-not $StageToSdk.IsPresent) {
    Write-Host "  To stage manually, run scripts/stage_android_validation_artifacts.ps1"
}
