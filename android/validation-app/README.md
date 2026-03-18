# Android Validation App

This is a minimal Android app skeleton for validating the prebuilt
`tinalux_native` library on device.

Current entry points:

- `MainActivity`: default OpenGL ES / EGL validation path
- `VulkanValidationActivity`: opt-in Vulkan validation path
- `:tinalux-sdk`: reusable Android library module consumed by this app

How to use:

1. Build and stage the Android shared library with:
   - `powershell -ExecutionPolicy Bypass -File scripts/build_android_native.ps1 -Abi arm64-v8a -StageToSdk`
   - `powershell -ExecutionPolicy Bypass -File scripts/build_android_native.ps1 -Abi arm64-v8a -StageToSdk -ValidateOnly`
   - or from Gradle:
     `gradle :tinalux-sdk:assembleDebug -Ptinalux.autoBuildNative=true`
2. Open `android/validation-app` in Android Studio.
3. Sync Gradle.
4. Run `MainActivity` for the default Android OpenGL backend.
5. Launch `VulkanValidationActivity` manually when you want to validate the
   Vulkan path.

Command-line Gradle note:

- this sample does not commit a Gradle wrapper yet
- use Android Studio, or invoke `gradle` from a local Gradle installation

Current scope:

- Consumes the reusable `../tinalux-sdk` Android library module
- The SDK module reuses the shared host code from `../host/src/main/kotlin`
- Can stage optional runtime data into `../tinalux-sdk/src/main/assets`
- Assumes the native shared library is provided as a prebuilt artifact
- The same SDK module can now be published as an AAR via Maven Local
