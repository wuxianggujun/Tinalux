# Tinalux CI 验证入口

> 更新时间：2026-03-24
> 目标：明确当前每条 GitHub Actions workflow 的职责、触发范围和查看顺序

## 当前结论

- 当前仓库已有 `3` 条可用 CI 验证链路
- 它们分别覆盖 Linux X11 构建、Windows 桌面 smoke、Android 脚本级 smoke
- 三条 workflow 现已统一启用并发取消策略：
  `concurrency: ${{ github.workflow }}-${{ github.ref }}`
- 三条 workflow 现已具备显式运行超时，失败产物统一保留 `1` 天
- 三条 workflow 的 `runner-fingerprint.json` / `execution-summary.json` 已对齐顶层 schema，并固定 `schemaVersion = 1`，便于横向比对状态、耗时和步骤统计
- 三条 workflow 的 metadata 目录现已统一提供 `metadata-manifest.json` 作为索引入口，便于脚本或团队成员快速定位各类 JSON / Markdown 输出
- 三条 workflow 现已在上传 artifact 前执行 metadata 自动验收，并产出 `metadata-validation.json` / `metadata-validation.md`
- 三条 workflow 现已额外产出 `baseline-fetch.json` / `baseline-fetch.md` 与 `threshold-check.json` / `threshold-check.md`；当前会优先尝试拉取上一条成功 run 的 metadata 作为基线，继续保持 `warning-only`，不直接拦截 CI；命中阈值时会在 GitHub Actions 日志里发出 `warning` 注解，并额外标记 runner image / tool 版本漂移
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
  - 采集 runner / compiler / buildDir 元数据
  - 分别用 `gcc` / `clang` 执行 `scripts/validateTinaGlfwX11Linux.sh`
  - 汇总单阶段构建耗时和构建目录体积
- 运行边界：
  - job 超时 `30` 分钟
- 当前附加能力：
  - workflow summary 会输出 runner 指纹、编译器版本、单阶段构建耗时和构建目录体积
  - 会额外记录 `runner-fingerprint.json`、`build-run.json`、`execution-summary.json`、`execution-summary.md`、`baseline-fetch.json`、`baseline-fetch.md`、`threshold-check.json`、`threshold-check.md`、`metadata-manifest.json` 与 `metadata-validation.json`
  - `baseline-fetch.json` / `baseline-fetch.md` 当前会尽量拉取上一条成功 Linux X11 metadata artifact，供本次 run 做趋势对比
  - `threshold-check.json` / `threshold-check.md` 当前会基于 execution summary 状态、构建耗时、runner / tool 版本漂移和可用 baseline 输出 `warning-only` 告警摘要
  - metadata 目录会作为独立 artifact 在成功和失败场景都上传，便于下载比对 runner、compiler 和构建结果
- 失败回溯：
  - 上传 `build/tina_glfw-linux-x11/<compiler>/**` 构建目录，产物保留 `1` 天
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
  - Windows CI 为了保留 configure / build / test 三阶段耗时，已拆为 CI 专用分阶段脚本执行；本地 smoke 入口仍保持不变
  - 因此 Windows 桌面 smoke 已同时完成“触发解耦”、“执行解耦”、“边界契约校验”和“阶段耗时观测”
- 运行内容：
  - `./tests/scripts/desktop_smoke_contract_smoke.ps1 -RepoRoot .`
  - `syncSkia.bat`
  - `./tests/scripts/windows_desktop_smoke_stage_run.ps1 -Stage configure -BuildDir build-ci -Config Debug`
  - `./tests/scripts/windows_desktop_smoke_stage_run.ps1 -Stage build -BuildDir build-ci -Config Debug`
  - `./tests/scripts/windows_desktop_smoke_stage_run.ps1 -Stage test -BuildDir build-ci -Config Debug`
- 当前附加能力：
  - 缓存 `3rdparty/skia`
  - 缓存 `build-ci/skia` 以及 Debug / Release 的 Skia 签名文件
  - build cache 允许按同一 `Skia revision` 前缀回退 restore，减小配置哈希变更时的冷启动成本
  - `cmake/SkiaConfig.cmake` 的签名文件现在只在内容变化时改写，避免每次 configure 都重复触发 `gn gen`
  - workflow summary 会输出 source/build cache 命中状态、matched key 和 save 策略摘要
  - workflow summary 还会输出 configure / build / test 三阶段耗时摘要
  - workflow summary 还会输出本次桌面 smoke 的最慢测试 Top 列表和累计测试耗时
  - 会额外记录 `runner-fingerprint.json`、`cache-summary.json` 与 `metadata-manifest.json`，失败时随 artifact 一并保留
  - 会额外记录 `stage-configure.json`、`stage-build.json`、`stage-test.json`、`execution-summary.json` 与 `execution-summary.md`
  - 会额外记录 `test-timings.json` 与 `test-timings-summary.md`，即使测试失败也尽量保留，且它们已纳入同一份 metadata manifest
  - 会额外记录 `baseline-fetch.json` 与 `baseline-fetch.md`，尽量拉取上一条成功 Windows smoke metadata artifact，供本次 run 做趋势对比
  - 会额外记录 `threshold-check.json` 与 `threshold-check.md`，基于阶段耗时、慢测试、cache 状态、runner / tool 版本漂移和可用 baseline 生成 `warning-only` 告警摘要
  - 会额外记录 `metadata-validation.json` 与 `metadata-validation.md`，用于自动检查 manifest、schemaVersion 和关键 JSON/Markdown 是否齐全
  - metadata 目录会作为独立 artifact 在成功和失败场景都上传，便于团队下载比对 cache / runner / timing 结果
  - 前置校验桌面 smoke 入口和 `android-scripts` 过滤契约
  - 失败时上传 `CTest` 日志，产物保留 `1` 天
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
  - `./tests/scripts/android_build_scripts_smoke_ci_metadata.ps1 -OutputRoot ... -ArtifactsRoot ...`
  - `./tests/scripts/android_build_scripts_smoke_stage_run.ps1 -Stage stage -RepoRoot . -OutputRoot ...`
  - `./tests/scripts/android_build_scripts_smoke_stage_run.ps1 -Stage validate -RepoRoot . -OutputRoot ...`
- 当前附加能力：
  - workflow summary 会输出 runner 指纹，以及 stage / validate 两阶段耗时摘要
  - 会额外记录 `runner-fingerprint.json`、`script-stage.json`、`script-validate.json`、`execution-summary.json`、`execution-summary.md`、`baseline-fetch.json`、`baseline-fetch.md`、`threshold-check.json`、`threshold-check.md`、`metadata-manifest.json` 与 `metadata-validation.json`
  - `baseline-fetch.json` / `baseline-fetch.md` 当前会尽量拉取上一条成功 Android smoke metadata artifact，供本次 run 做趋势对比
  - `threshold-check.json` / `threshold-check.md` 当前会基于 stage / validate 两阶段耗时、execution summary 状态、runner / tool 版本漂移和可用 baseline 输出 `warning-only` 告警摘要
  - metadata 目录会作为独立 artifact 在成功和失败场景都上传，便于下载比对 runner、耗时和保留的 smoke 临时目录
- 失败回溯：
  - 保留 smoke 临时目录
  - 上传 `ci-artifacts/android-build-scripts-smoke/**`，产物保留 `1` 天
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
