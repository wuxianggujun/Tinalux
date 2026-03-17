# Tinalux Android SDK Module

This module packages the reusable Android host/runtime layer for Tinalux as an
Android library.

Current responsibilities:

- exposes the host integration classes from `../host/src/main/kotlin`
- packages prebuilt native libraries from `src/main/jniLibs/<abi>`
- packages optional runtime assets from `src/main/assets`

Current public entry points:

- `com.tinalux.runtime.TinaluxActivity`
- `com.tinalux.runtime.TinaluxSurfaceView`
- `com.tinalux.runtime.TinaluxBackend`
- `com.tinalux.runtime.TinaluxVulkanValidationActivity`

How to use:

1. Build and stage the Android shared library:
   - `powershell -ExecutionPolicy Bypass -File ../../scripts/build_android_native.ps1 -Abi arm64-v8a -StageToSdk`
2. Add this module as an Android library dependency.
3. Start from `TinaluxActivity` or embed `TinaluxSurfaceView` directly.

Gradle-native workflow:

- opt in to automatic native build/staging with:
  - `./gradlew :tinalux-sdk:assembleDebug -Ptinalux.autoBuildNative=true`
- configurable properties:
  - `-Ptinalux.ndkPath=...`
  - `-Ptinalux.nativeAbi=arm64-v8a`
  - `-Ptinalux.nativeBuildType=Release`
  - `-Ptinalux.androidApi=26`
  - `-Ptinalux.nativeIcuData=...`

Build guard:

- `preBuild` verifies that `src/main/jniLibs/<abi>/libtinalux_native.so` exists
- if the library is missing, the Gradle build fails with an actionable message

Publishing:

- this module now supports `maven-publish`
- publish locally with:
  - `./gradlew :tinalux-sdk:publishTinaluxSdkToMavenLocal`
- default coordinates:
  - `groupId = com.tinalux`
  - `artifactId = tinalux-android-sdk`
  - `version = 0.1.0-SNAPSHOT`

Current limitation:

- native library production is still external to Gradle
