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

function Assert-RepoSdkModuleContract {
    param([string]$RepoRootPath)

    $sdkModuleRoot = Join-Path $RepoRootPath "android/tinalux-sdk"
    $buildFile = Join-Path $sdkModuleRoot "build.gradle.kts"
    $manifestFile = Join-Path $sdkModuleRoot "src/main/AndroidManifest.xml"
    $consumerRulesFile = Join-Path $sdkModuleRoot "consumer-rules.pro"

    foreach ($requiredPath in @($buildFile, $manifestFile, $consumerRulesFile)) {
        if (-not (Test-Path $requiredPath)) {
            throw "Expected Android SDK module contract file was not found: $requiredPath"
        }
    }

    $buildFileContent = Get-Content -Path $buildFile -Raw
    $expectedPatterns = @(
        @{
            Pattern = 'java\.srcDir\(projectDir\.resolve\("\.\./host/src/main/kotlin"\)\)'
            Description = 'host Kotlin source path'
        },
        @{
            Pattern = 'manifest\.srcFile\("src/main/AndroidManifest\.xml"\)'
            Description = 'main manifest source path'
        },
        @{
            Pattern = 'jniLibs\.srcDirs\("src/main/jniLibs"\)'
            Description = 'jniLib staging path'
        },
        @{
            Pattern = 'assets\.srcDirs\("src/main/assets"\)'
            Description = 'runtime asset staging path'
        },
        @{
            Pattern = 'consumerProguardFiles\("consumer-rules\.pro"\)'
            Description = 'consumer ProGuard rules path'
        }
    )

    foreach ($expectedPattern in $expectedPatterns) {
        if ($buildFileContent -notmatch $expectedPattern.Pattern) {
            throw ("Android SDK module contract drifted. Missing {0} declaration in {1}" -f $expectedPattern.Description, $buildFile)
        }
    }

    $manifestContent = Get-Content -Path $manifestFile -Raw
    if ($manifestContent -notmatch 'android:hasCode="true"') {
        throw ("Android SDK module manifest drifted. Missing android:hasCode=""true"" in {0}" -f $manifestFile)
    }
}

function Copy-RepoSdkModuleContractSnapshot {
    param(
        [string]$RepoRootPath,
        [string]$SdkModuleRoot
    )

    $sourceSdkModuleRoot = Join-Path $RepoRootPath "android/tinalux-sdk"
    $filesToCopy = @(
        "build.gradle.kts",
        "consumer-rules.pro",
        "src/main/AndroidManifest.xml"
    )

    foreach ($relativePath in $filesToCopy) {
        $sourcePath = Join-Path $sourceSdkModuleRoot $relativePath
        if (-not (Test-Path $sourcePath)) {
            throw "SDK module snapshot source file not found: $sourcePath"
        }

        $destinationPath = Join-Path $SdkModuleRoot $relativePath
        $destinationDirectory = Split-Path -Parent $destinationPath
        New-Item -ItemType Directory -Force -Path $destinationDirectory | Out-Null
        Copy-Item -Force $sourcePath $destinationPath
    }
}

function Get-SdkModuleArtifactSnapshot {
    param([string]$SdkModuleRoot)

    $snapshot = @()
    $artifactRoots = @(
        (Join-Path $SdkModuleRoot "src/main/jniLibs"),
        (Join-Path $SdkModuleRoot "src/main/assets")
    )

    foreach ($artifactRoot in $artifactRoots) {
        if (-not (Test-Path $artifactRoot)) {
            continue
        }

        $files = Get-ChildItem -Path $artifactRoot -File -Recurse | Sort-Object FullName
        foreach ($file in $files) {
            $relativePath = $file.FullName.Substring($SdkModuleRoot.Length).TrimStart('\', '/')
            $hash = (Get-FileHash -Path $file.FullName -Algorithm SHA256).Hash
            $snapshot += "{0}|{1}|{2}" -f $relativePath, $file.Length, $hash
        }
    }

    return $snapshot
}

function Assert-SdkModuleArtifactSnapshotUnchanged {
    param(
        [string[]]$Before,
        [string[]]$After,
        [string]$Description
    )

    $beforeValue = @($Before)
    $afterValue = @($After)
    if (Compare-Object -ReferenceObject $beforeValue -DifferenceObject $afterValue) {
        throw "SDK module artifacts changed unexpectedly during $Description."
    }
}
