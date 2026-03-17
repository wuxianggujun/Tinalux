# Android Validation App

This is a minimal Android app skeleton for validating the prebuilt
`tinalux_native` library on device.

Current entry points:

- `MainActivity`: default OpenGL ES / EGL validation path
- `VulkanValidationActivity`: opt-in Vulkan validation path

How to use:

1. Build `libtinalux_native.so` for your target ABI with the Android toolchain.
2. Stage the artifact with:
   - `powershell -ExecutionPolicy Bypass -File scripts/stage_android_validation_artifacts.ps1 -SourceLibrary <path-to-libtinalux_native.so> -Abi arm64-v8a`
3. Open `android/validation-app` in Android Studio.
4. Run `MainActivity` for the default Android OpenGL backend.
5. Launch `VulkanValidationActivity` manually when you want to validate the
   Vulkan path.

Current scope:

- Uses the shared host code from `../host/src/main/kotlin`
- Can stage optional runtime data into `app/src/main/assets`
- Assumes the native shared library is provided as a prebuilt artifact
