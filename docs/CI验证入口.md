# Tinalux CI 验证入口

> 更新时间：2026-03-23
> 目标：明确当前每条 GitHub Actions workflow 的职责、触发范围和查看顺序

## 当前结论

- 当前仓库已有 `3` 条可用 CI 验证链路
- 它们分别覆盖 Linux X11 构建、Windows 桌面 smoke、Android 脚本级 smoke
- 三条 workflow 现已统一启用并发取消策略：
  `concurrency: ${{ github.workflow }}-${{ github.ref }}`
- 三条 workflow 现已具备显式运行超时，失败产物统一保留 `7` 天
- `P1` 真机验证仍未自动化，继续按 backlog / TODO 管理

## Workflow 选择

### 1. Linux X11 构建验证

- workflow：`.github/workflows/linux-tina-glfw-x11.yml`
- 主要目的：确认 `3rdparty/tina_glfw` 的 Linux/X11 分支可在 CI 上稳定编译
- 触发改动：
  - `.github/workflows/linux-tina-glfw-x11.yml`
  - `scripts/validateTinaGlfwX11Linux.sh`
  - `3rdparty/tina_glfw/**`
- 当前说明：
  - 该链路只直接构建 `3rdparty/tina_glfw`
  - 因此不会因主工程 `src/platform/**` 或根 `CMakeLists.txt` 变更而触发
- 运行内容：
  - 安装 Ubuntu X11 构建依赖
  - 分别用 `gcc` / `clang` 执行 `scripts/validateTinaGlfwX11Linux.sh`
- 运行边界：
  - job 超时 `30` 分钟
- 失败回溯：
  - 上传 `build/tina_glfw-linux-x11/**` 构建目录，产物保留 `7` 天
- 适合查看的场景：
  - `tina_glfw` 上游分支或本地补丁改动
  - Linux X11 构建链本身的编译性回归

### 2. Windows 桌面 Smoke

- workflow：`.github/workflows/windows-desktop-smoke.yml`
- 主要目的：确认 Windows 桌面主链路可以完成 `syncSkia -> configure -> build -> ctest`
- 触发改动：
  - `.github/workflows/windows-desktop-smoke.yml`
  - `CMakeLists.txt`
  - `cmake/**`
  - `include/**`
  - `src/**`
  - `tests/**`
  - `scripts/runSmokeTests.ps1`
  - `scripts/runSmokeTests.sh`
  - `syncSkia.bat`
  - `main.cpp`
- 当前排除：
  - `tests/scripts/android_stage_script_smoke.ps1`
  - `tests/scripts/android_build_validate_smoke.ps1`
  - `tests/scripts/android_smoke_test_helpers.ps1`
  - 这类 Android PowerShell smoke 变更改由 `android-build-scripts-smoke` 单独兜底
- 当前执行边界：
  - `scripts/runSmokeTests.ps1` / `scripts/runSmokeTests.sh` 现在都只构建 `TinaluxRunDesktopSmokeTests`
  - `TinaluxRunDesktopSmokeTests` 在有 `pwsh` 的环境下会先执行 `tests/scripts/desktop_smoke_contract_smoke.ps1`
  - 该 target 会通过 `ctest -LE android-scripts` 排除 Android PowerShell smoke
  - workflow 也会前置执行同一契约脚本，尽早暴露入口或 label 漂移
  - 因此 Windows 桌面 smoke 已同时完成“触发解耦”、“执行解耦”和“边界契约校验”
- 运行内容：
  - `./tests/scripts/desktop_smoke_contract_smoke.ps1 -RepoRoot .`
  - `syncSkia.bat`
  - `cmake -S . -B build-ci -G Ninja -DCMAKE_BUILD_TYPE=Debug`
  - `./scripts/runSmokeTests.ps1 -BuildDir build-ci -Config Debug`
- 当前附加能力：
  - 缓存 `3rdparty/skia`
  - 缓存 `build-ci/skia` Debug 构建产物
  - 前置校验桌面 smoke 入口和 `android-scripts` 过滤契约
  - 失败时上传 `CTest` 日志，产物保留 `7` 天
  - 同时保留 `CMakeCache.txt`、`CMakeConfigureLog.yaml`、`build.ninja`、`.ninja_log`
  - 仅 `syncSkia.bat` 变更会触发该链路，避免 `syncSkia.sh` 的无效触发
- 适合查看的场景：
  - 核心 C++ 源码改动
  - CMake / Skia 集成改动
  - smoke tests 变更

### 3. Android Build Scripts Smoke

- workflow：`.github/workflows/android-build-scripts-smoke.yml`
- 主要目的：单独守护 Android PowerShell 脚本链路，不依赖整套桌面 smoke
- 触发改动：
  - `.github/workflows/android-build-scripts-smoke.yml`
  - `scripts/build_android_native.ps1`
  - `scripts/stage_android_validation_artifacts.ps1`
  - `tests/scripts/android_smoke_test_helpers.ps1`
  - `tests/scripts/android_stage_script_smoke.ps1`
  - `tests/scripts/android_build_validate_smoke.ps1`
  - `android/tinalux-sdk/build.gradle.kts`
  - `android/tinalux-sdk/consumer-rules.pro`
  - `android/tinalux-sdk/src/main/AndroidManifest.xml`
- 当前说明：
  - 该链路校验 PowerShell 脚本和已提交的 SDK 模块 staging 约定
  - 也会校验 SDK 模块仍引用 `android/host` Kotlin 源码，并保持 `android:hasCode="true"`
  - 也会校验 SDK 模块的 `consumer-rules.pro` 仍与 `build.gradle.kts` 对齐
  - 同时覆盖默认 `SdkModuleRoot` 解析和自定义 SDK 模块快照路径
  - 默认 `ValidateOnly` 路径也会校验对仓库内已 staged SDK 产物无副作用
  - 当前不覆盖 `android/host/**` 和 `android/validation-app/**` 的 Gradle / 运行时行为
- 运行内容：
  - `./tests/scripts/android_stage_script_smoke.ps1 -RepoRoot .`
  - `./tests/scripts/android_build_validate_smoke.ps1 -RepoRoot .`
- 失败回溯：
  - 保留 smoke 临时目录
  - 上传 `ci-artifacts/android-build-scripts-smoke/**`，产物保留 `7` 天
- 适合查看的场景：
  - Android 打包脚本改动
  - SDK module staging 约定改动
  - validate-only 流程改动

## 手动触发建议

- 改 GLFW / X11：优先看 `linux-tina-glfw-x11`
- 改 C++ 核心、CMake、桌面 smoke 测试：优先看 `windows-desktop-smoke`
- 改 Android 构建脚本或 SDK staging：优先看 `android-build-scripts-smoke`
- 改动跨多个域时：允许同时手动触发多条 workflow

## 当前未覆盖范围

- Android 真机编译、安装、渲染、输入、生命周期闭环
- Linux/X11 真机运行时与 IME 差异验证
- macOS / Metal 桌面验证
- 更完整的平台矩阵、更细粒度的缓存策略，以及对主工程桌面链路和平台链路继续精炼的触发规则
