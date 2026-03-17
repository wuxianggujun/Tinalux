# Android Host Skeleton

This folder contains a minimal Android host-side scaffold for the native
`tinalux_native` library.

Current contents:

- `TinaluxNativeBridge.kt`: direct JNI bindings
- `TinaluxRendererHost.kt`: lifecycle-safe runtime owner
- `TinaluxSurfaceView.kt`: `SurfaceView` wrapper that drives native frames
- touch input is translated into the current mouse-style UI event model

Suggested integration steps:

1. Copy the Kotlin sources into an Android app module.
2. Package `libtinalux_native.so` with your APK.
3. Create a screen that hosts `TinaluxSurfaceView`.
4. Replace the demo-scene installation with your real UI bootstrap.

Current limitations:

- No IME bridge yet
- Multi-touch is collapsed to a single primary pointer
- No pause/resume GPU resource recovery yet
- No Vulkan Android surface path yet
