param(
    [Parameter(Mandatory = $true)]
    [string]$RepoRoot
)

$ErrorActionPreference = "Stop"

function New-TempDirectory {
    param([string]$Prefix)

    $path = Join-Path ([System.IO.Path]::GetTempPath()) ($Prefix + [System.Guid]::NewGuid().ToString("N"))
    New-Item -ItemType Directory -Force -Path $path | Out-Null
    return $path
}

function New-FakeSdkModule {
    param([string]$SdkModuleRoot)

    New-Item -ItemType Directory -Force -Path (Join-Path $SdkModuleRoot "src/main") | Out-Null
    Set-Content -Path (Join-Path $SdkModuleRoot "build.gradle.kts") -Value "plugins {}"
    Set-Content -Path (Join-Path $SdkModuleRoot "src/main/AndroidManifest.xml") -Value "<manifest package=`"com.tinalux.runtime.test`" />"
}

$repoRootPath = [System.IO.Path]::GetFullPath((Resolve-Path $RepoRoot).Path)
$buildScript = Join-Path $repoRootPath "scripts/build_android_native.ps1"
if (-not (Test-Path $buildScript)) {
    throw "Build script not found: $buildScript"
}

$tempRoot = New-TempDirectory "tinalux-android-build-validate-smoke-"
try {
    $fakeNdkRoot = Join-Path $tempRoot "Android/Sdk/ndk/27.0.12077973"
    $toolchainDir = Join-Path $fakeNdkRoot "build/cmake"
    New-Item -ItemType Directory -Force -Path $toolchainDir | Out-Null
    Set-Content -Path (Join-Path $toolchainDir "android.toolchain.cmake") -Value "# fake toolchain"

    $sdkModuleRoot = Join-Path $tempRoot "android/tinalux-sdk"
    New-FakeSdkModule $sdkModuleRoot

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
} finally {
    if (Test-Path $tempRoot) {
        Remove-Item -Recurse -Force $tempRoot
    }
}
