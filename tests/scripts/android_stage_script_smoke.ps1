param(
    [Parameter(Mandatory = $true)]
    [string]$RepoRoot
)

$ErrorActionPreference = "Stop"

function New-TempDirectory {
    param([string]$Prefix)

    $parentRoot = if (-not [string]::IsNullOrWhiteSpace($env:TINALUX_TEST_TEMP_PARENT)) {
        New-Item -ItemType Directory -Force -Path $env:TINALUX_TEST_TEMP_PARENT | Out-Null
        [System.IO.Path]::GetFullPath($env:TINALUX_TEST_TEMP_PARENT)
    } else {
        [System.IO.Path]::GetTempPath()
    }

    $path = Join-Path $parentRoot ($Prefix + [System.Guid]::NewGuid().ToString("N"))
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
$stageScript = Join-Path $repoRootPath "scripts/stage_android_validation_artifacts.ps1"
if (-not (Test-Path $stageScript)) {
    throw "Stage script not found: $stageScript"
}

$keepTempRoot = $env:TINALUX_KEEP_TEST_TEMP_ROOT -eq "1"
$tempRoot = New-TempDirectory "tinalux-android-stage-smoke-"
try {
    $sdkModuleRoot = Join-Path $tempRoot "android/tinalux-sdk"
    New-FakeSdkModule $sdkModuleRoot

    $artifactRoot = Join-Path $tempRoot "artifacts"
    New-Item -ItemType Directory -Force -Path $artifactRoot | Out-Null

    $sourceLibrary = Join-Path $artifactRoot "libtinalux_native.so"
    [System.IO.File]::WriteAllBytes($sourceLibrary, [byte[]](1, 2, 3, 4))

    $sourceIcuData = Join-Path $artifactRoot "icudtl.dat"
    [System.IO.File]::WriteAllBytes($sourceIcuData, [byte[]](5, 6))

    & $stageScript -SourceLibrary $sourceLibrary -Abi arm64-v8a -SourceIcuData $sourceIcuData -SdkModuleRoot $sdkModuleRoot

    $stagedLibrary = Join-Path $sdkModuleRoot "src/main/jniLibs/arm64-v8a/libtinalux_native.so"
    if (-not (Test-Path $stagedLibrary)) {
        throw "Expected staged native library at $stagedLibrary"
    }
    if ((Get-Item $stagedLibrary).Length -ne 4) {
        throw "Unexpected staged native library size at $stagedLibrary"
    }

    $stagedIcuData = Join-Path $sdkModuleRoot "src/main/assets/icudtl.dat"
    if (-not (Test-Path $stagedIcuData)) {
        throw "Expected staged ICU data at $stagedIcuData"
    }

    & $stageScript -Abi arm64-v8a -SdkModuleRoot $sdkModuleRoot -SourceIcuData $sourceIcuData -ValidateOnly
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
