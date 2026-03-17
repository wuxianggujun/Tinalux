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

1. Stage `libtinalux_native.so` into `src/main/jniLibs/<abi>`.
2. Add this module as an Android library dependency.
3. Start from `TinaluxActivity` or embed `TinaluxSurfaceView` directly.

Current limitation:

- native library production is still external to Gradle
