# Linux X11 构建验证

这条验证链只覆盖 `3rdparty/tina_glfw` 的 Linux X11 构建，
目标是尽快确认 IME preedit callbacks 所依赖的 X11 分支可以独立编过。

## 覆盖范围

- 使用仓库内固定的本地 `3rdparty/tina_glfw`
- 强制开启 `GLFW_BUILD_X11`
- 强制关闭 `GLFW_BUILD_WAYLAND`
- 不编 examples、tests、docs
- 当前不包含 Skia 和 Tinalux 整库 Linux 链接验证

## Linux 本地运行

先安装基础依赖：

```bash
sudo apt-get update
sudo apt-get install -y \
  build-essential \
  cmake \
  ninja-build \
  pkg-config \
  xorg-dev \
  libgl1-mesa-dev
```

然后执行：

```bash
bash ./scripts/validateTinaGlfwX11Linux.sh
```

默认输出目录：

```text
build/tina_glfw-linux-x11
```

可选环境变量：

```bash
BUILD_DIR=/tmp/tina_glfw-x11 BUILD_TYPE=Release \
  bash ./scripts/validateTinaGlfwX11Linux.sh
```

## CI 自动验证

仓库新增工作流：

```text
.github/workflows/linux-tina-glfw-x11.yml
```

触发条件：

- `3rdparty/tina_glfw/**`
- `src/platform/glfw/**`
- `src/platform/CMakeLists.txt`
- 根目录 `CMakeLists.txt`
- 验证脚本和工作流自身

CI 在 `ubuntu-24.04` 上分别用 `gcc` 和 `clang` 执行同一条
X11 构建验证命令。

## 备注

这一步的目的不是替代 Linux 真机输入法联调，
而是先把最基础的编译闭环补齐，尽早暴露 X11 分支的源码或依赖问题。
