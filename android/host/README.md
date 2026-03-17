# Android Host Skeleton

This folder contains a minimal Android host-side scaffold for the native
`tinalux_native` library.

Current contents:

- `TinaluxNativeBridge.kt`: direct JNI bindings
- `TinaluxRendererHost.kt`: lifecycle-safe runtime owner
- `TinaluxSurfaceView.kt`: `SurfaceView` wrapper that drives native frames
- `TinaluxInputConnection.kt`: IME bridge for text input / composition
- `TinaluxActivity.kt`: minimal host `Activity` that wires the surface lifecycle
- touch input is translated into the current mouse-style UI event model
- system clipboard is mirrored between Android and the native window state
- `TinaluxSurfaceView` exposes `onHostPause()` / `onHostResume()` helpers for host lifecycle wiring
- runtime suspend/resume state is mirrored into the native layer
- suspend/reattach keeps the native UI session alive and rebuilds render state on demand

Default backend:

- `AndroidRuntimeConfig` defaults to `Backend::OpenGL`
- Vulkan remains opt-in until Android device-side validation is in place

Suggested integration steps:

1. Copy the Kotlin sources into an Android app module.
2. Package `libtinalux_native.so` with your APK.
3. Create a screen that hosts `TinaluxSurfaceView`.
4. Replace the demo-scene installation with your real UI bootstrap.

Current limitations:

- Multi-touch is collapsed to a single primary pointer
- `EditorInfo` is still a generic text configuration, not widget-specific
- No pause/resume GPU resource recovery yet
- No Vulkan Android surface path yet
