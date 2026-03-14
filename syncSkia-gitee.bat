@echo off
if not exist "3rdparty/skia" (
    git clone https://skia.googlesource.com/skia.git --depth 1 3rdparty/skia
    pushd 3rdparty\skia
) else (
    pushd 3rdparty\skia
    git pull --depth=1 https://skia.googlesource.com/skia.git main --allow-unrelated-histories
)
set PYTHONUTF8=1
set GIT_SYNC_DEPS_SKIP_EMSDK=1
python3 tools/git-sync-deps -v
popd
