param(
    [ValidateSet("armeabi-v7a", "arm64-v8a", "x86", "x86_64")]
    [string]$Abi = "arm64-v8a",
    [ValidateRange(21, 1000)]
    [int]$ApiLevel = 26,
    [ValidateSet("Debug", "Release")]
    [string]$BuildType = "Release",
    [string]$NdkPath = "",
    [string]$Generator = "Ninja",
    [string]$BuildRoot = "",
    [switch]$StageToSdk,
    [switch]$ValidateOnly,
    [string]$SdkModuleRoot = "",
    [string]$SourceIcuData = "",
    [string]$CmakePath = "",
    [string]$NinjaPath = "",
    [Version]$MinimumCmakeVersion = ([Version]"3.31.0")
)

$ErrorActionPreference = "Stop"

function Resolve-FullPath {
    param([string]$PathValue)
    if (-not (Test-Path $PathValue)) {
        throw "Path not found: $PathValue"
    }
    return [System.IO.Path]::GetFullPath((Resolve-Path $PathValue).Path)
}

function Find-FirstExistingPath {
    param([string[]]$Candidates)

    foreach ($candidate in $Candidates) {
        if (-not [string]::IsNullOrWhiteSpace($candidate) -and (Test-Path $candidate)) {
            return [System.IO.Path]::GetFullPath((Resolve-Path $candidate).Path)
        }
    }

    return $null
}

function Get-CmakeVersion {
    param([string]$ExecutablePath)

    try {
        $versionOutput = & $ExecutablePath --version 2>$null
        if ($LASTEXITCODE -ne 0 -or -not $versionOutput) {
            return $null
        }

        $firstLine = ($versionOutput | Select-Object -First 1)
        if ($firstLine -match 'cmake version ([0-9]+\.[0-9]+\.[0-9]+)') {
            return [Version]$Matches[1]
        }
    } catch {
        return $null
    }

    return $null
}

function Select-CompatibleCmake {
    param(
        [string[]]$Candidates,
        [Version]$MinimumVersion
    )

    foreach ($candidate in $Candidates) {
        if ([string]::IsNullOrWhiteSpace($candidate)) {
            continue
        }

        $resolvedCandidate = $candidate
        if ($candidate -ne "cmake" -and -not (Test-Path $candidate)) {
            continue
        }
        if ($candidate -ne "cmake") {
            $resolvedCandidate = [System.IO.Path]::GetFullPath((Resolve-Path $candidate).Path)
        }

        $detectedVersion = Get-CmakeVersion $resolvedCandidate
        if ($null -ne $detectedVersion -and $detectedVersion -ge $MinimumVersion) {
            return @{
                Path = $resolvedCandidate
                Version = $detectedVersion
            }
        }
    }

    return $null
}

$repoRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
if ([string]::IsNullOrWhiteSpace($BuildRoot)) {
    $BuildRoot = Join-Path $repoRoot ("build-android-{0}-{1}" -f $Abi, $BuildType.ToLowerInvariant())
}
$BuildRoot = [System.IO.Path]::GetFullPath($BuildRoot)

$resolvedIcuData = ""
if (-not [string]::IsNullOrWhiteSpace($SourceIcuData)) {
    $resolvedIcuData = Resolve-FullPath $SourceIcuData
}

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

$sdkRoot = [System.IO.Path]::GetFullPath((Join-Path $NdkPath "..\.."))
$sdkCmakeRoot = Join-Path $sdkRoot "cmake"
$sdkCmakeVersionDir = $null
if (Test-Path $sdkCmakeRoot) {
    $sdkCmakeVersionDir = Get-ChildItem $sdkCmakeRoot -Directory |
        Sort-Object Name -Descending |
        Select-Object -First 1
}

if ([string]::IsNullOrWhiteSpace($CmakePath)) {
    $selectedCmake = Select-CompatibleCmake -Candidates @(
        "cmake",
        $(if ($sdkCmakeVersionDir) { Join-Path $sdkCmakeVersionDir.FullName "bin\cmake.exe" })
    ) -MinimumVersion $MinimumCmakeVersion
    if ($null -eq $selectedCmake) {
        throw "Failed to locate a compatible cmake.exe (minimum version $MinimumCmakeVersion). Pass -CmakePath explicitly."
    }
    $CmakePath = $selectedCmake.Path
    $detectedCmakeVersion = $selectedCmake.Version
} else {
    if ($CmakePath -ne "cmake") {
        $CmakePath = Resolve-FullPath $CmakePath
    }
    $detectedCmakeVersion = Get-CmakeVersion $CmakePath
    if ($null -eq $detectedCmakeVersion -or $detectedCmakeVersion -lt $MinimumCmakeVersion) {
        throw "Configured CMake '$CmakePath' does not satisfy the minimum version $MinimumCmakeVersion."
    }
}

if ([string]::IsNullOrWhiteSpace($NinjaPath) -and $Generator -eq "Ninja") {
    $NinjaPath = Find-FirstExistingPath @(
        $(if ($sdkCmakeVersionDir) { Join-Path $sdkCmakeVersionDir.FullName "bin\ninja.exe" }),
        "ninja"
    )
}
if ($Generator -eq "Ninja" -and [string]::IsNullOrWhiteSpace($NinjaPath)) {
    throw "Failed to locate ninja.exe for the Ninja generator. Pass -NinjaPath explicitly."
}
if (-not [string]::IsNullOrWhiteSpace($NinjaPath) -and $NinjaPath -ne "ninja") {
    $NinjaPath = Resolve-FullPath $NinjaPath
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
if ($Generator -eq "Ninja" -and -not [string]::IsNullOrWhiteSpace($NinjaPath)) {
    $configureArgs += "-DCMAKE_MAKE_PROGRAM=$NinjaPath"
}

Write-Host "Configuring Android native build:"
Write-Host "  Repo:       $repoRoot"
Write-Host "  BuildRoot:  $BuildRoot"
Write-Host "  ABI:        $Abi"
Write-Host "  API Level:  android-$ApiLevel"
Write-Host "  BuildType:  $BuildType"
Write-Host "  NDK:        $NdkPath"
Write-Host "  CMake:      $CmakePath ($detectedCmakeVersion)"
Write-Host "  Mode:       $(if ($ValidateOnly.IsPresent) { "ValidateOnly" } else { "Build" })"
if ($Generator -eq "Ninja") {
    Write-Host "  Ninja:      $NinjaPath"
}
if (-not [string]::IsNullOrWhiteSpace($resolvedIcuData)) {
    Write-Host "  ICU Data:   $resolvedIcuData"
}
if ($StageToSdk.IsPresent) {
    $resolvedSdkModuleRoot = if ([string]::IsNullOrWhiteSpace($SdkModuleRoot)) {
        [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot "..\android\tinalux-sdk"))
    } else {
        [System.IO.Path]::GetFullPath($SdkModuleRoot)
    }
    Write-Host "  Stage SDK:  $resolvedSdkModuleRoot"
}
Write-Host ""

if ($ValidateOnly.IsPresent) {
    if ($StageToSdk.IsPresent) {
        $stageScript = Join-Path $PSScriptRoot "stage_android_validation_artifacts.ps1"
        $stageArgs = @{
            Abi = $Abi
            ValidateOnly = $true
        }
        if (-not [string]::IsNullOrWhiteSpace($SdkModuleRoot)) {
            $stageArgs.SdkModuleRoot = $SdkModuleRoot
        }
        if (-not [string]::IsNullOrWhiteSpace($resolvedIcuData)) {
            $stageArgs.SourceIcuData = $resolvedIcuData
        }

        & $stageScript @stageArgs
    }

    Write-Host "Validation complete."
    Write-Host "  Environment is ready for an Android native configure/build run."
    return
}

& $CmakePath @configureArgs
if ($LASTEXITCODE -ne 0) {
    throw "CMake configure failed."
}

& $CmakePath --build $BuildRoot --config $BuildType --target TinaluxAndroidNative
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
    $stageArgs = @{
        SourceLibrary = $library.FullName
        Abi = $Abi
    }
    if (-not [string]::IsNullOrWhiteSpace($SdkModuleRoot)) {
        $stageArgs.SdkModuleRoot = $SdkModuleRoot
    }
    if (-not [string]::IsNullOrWhiteSpace($resolvedIcuData)) {
        $stageArgs.SourceIcuData = $resolvedIcuData
    }

    Write-Host ""
    Write-Host "Staging Android SDK artifacts..."
    & $stageScript @stageArgs
}

Write-Host ""
Write-Host "Done."
Write-Host "  Native output: $($library.FullName)"
if (-not $StageToSdk.IsPresent) {
    Write-Host "  To stage manually, run scripts/stage_android_validation_artifacts.ps1"
}
