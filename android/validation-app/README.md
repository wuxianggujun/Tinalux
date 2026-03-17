# Android Validation App

This is a minimal Android app skeleton for validating the prebuilt
`tinalux_native` library on device.

Current entry points:

- `MainActivity`: default OpenGL ES / EGL validation path
- `VulkanValidationActivity`: opt-in Vulkan validation path

How to use:

1. Build `libtinalux_native.so` for your target ABI with the Android toolchain.
2. Copy the library into:
   - `app/src/main/jniLibs/arm64-v8a/libtinalux_native.so`
   - or another ABI folder under `jniLibs`
3. Open `android/validation-app` in Android Studio.
4. Run `MainActivity` for the default Android OpenGL backend.
5. Launch `VulkanValidationActivity` manually when you want to validate the
   Vulkan path.

Current scope:

- Uses the shared host code from `../host/src/main/kotlin`
- Does not yet bundle Skia runtime data automatically
- Assumes the native shared library is provided as a prebuilt artifact
