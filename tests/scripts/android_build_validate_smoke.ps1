param(
    [Parameter(Mandatory = $true)]
    [string]$RepoRoot
)

$ErrorActionPreference = "Stop"

$repoRootPath = [System.IO.Path]::GetFullPath((Resolve-Path $RepoRoot).Path)
$helperScript = Join-Path $repoRootPath "tests/scripts/android_smoke_test_helpers.ps1"
if (-not (Test-Path $helperScript)) {
    throw "Android smoke helper script not found: $helperScript"
}
. $helperScript

Assert-RepoSdkModuleContract -RepoRootPath $repoRootPath

$buildScript = Join-Path $repoRootPath "scripts/build_android_native.ps1"
if (-not (Test-Path $buildScript)) {
    throw "Build script not found: $buildScript"
}

$keepTempRoot = $env:TINALUX_KEEP_TEST_TEMP_ROOT -eq "1"
$tempRoot = New-TempDirectory "tinalux-android-build-validate-smoke-"
try {
    $fakeNdkRoot = Join-Path $tempRoot "Android/Sdk/ndk/27.0.12077973"
    $toolchainDir = Join-Path $fakeNdkRoot "build/cmake"
    New-Item -ItemType Directory -Force -Path $toolchainDir | Out-Null
    Set-Content -Path (Join-Path $toolchainDir "android.toolchain.cmake") -Value "# fake toolchain"

    $sdkModuleRoot = Join-Path $tempRoot "android/tinalux-sdk"
    Copy-RepoSdkModuleContractSnapshot -RepoRootPath $repoRootPath -SdkModuleRoot $sdkModuleRoot

    $artifactRoot = Join-Path $tempRoot "artifacts"
    New-Item -ItemType Directory -Force -Path $artifactRoot | Out-Null
    $sourceIcuData = Join-Path $artifactRoot "icudtl.dat"
    [System.IO.File]::WriteAllBytes($sourceIcuData, [byte[]](7, 8, 9))

    $cmakePath = (Get-Command cmake -ErrorAction Stop).Path

    & $buildScript `
        -Abi arm64-v8a `
        -ApiLevel 26 `
        -BuildType Release `
        -NdkPath $fakeNdkRoot `
        -CmakePath $cmakePath `
        -Generator Ninja `
        -NinjaPath $cmakePath `
        -StageToSdk `
        -ValidateOnly `
        -SdkModuleRoot $sdkModuleRoot `
        -SourceIcuData $sourceIcuData

    $stagedLibrary = Join-Path $sdkModuleRoot "src/main/jniLibs/arm64-v8a/libtinalux_native.so"
    if (Test-Path $stagedLibrary) {
        throw "ValidateOnly should not stage a native library, but found $stagedLibrary"
    }

    & $buildScript `
        -Abi arm64-v8a `
        -ApiLevel 26 `
        -BuildType Release `
        -NdkPath $fakeNdkRoot `
        -CmakePath $cmakePath `
        -Generator Ninja `
        -NinjaPath $cmakePath `
        -StageToSdk `
        -ValidateOnly `
        -SourceIcuData $sourceIcuData
} finally {
    if (Test-Path $tempRoot) {
        if ($keepTempRoot) {
            Write-Host "Preserved temp root:"
            Write-Host "  $tempRoot"
        } else {
            Remove-Item -Recurse -Force $tempRoot
        }
    }
}
