#!/bin/sh -e
if [ ! -d "3rdparty/skia" ]; then
    git clone "https://skia.googlesource.com/skia.git" --depth 1 3rdparty/skia
    cd 3rdparty/skia
else
    cd 3rdparty/skia
    git pull --depth=1 "https://skia.googlesource.com/skia.git" main --allow-unrelated-histories
fi
export PYTHONUTF8=1
export GIT_SYNC_DEPS_SKIP_EMSDK=1
if command -v python3 >/dev/null 2>&1; then
    _tinalux_python=python3
elif command -v python >/dev/null 2>&1; then
    _tinalux_python=python
else
    echo "error: python3/python not found" >&2
    exit 1
fi
"${_tinalux_python}" tools/git-sync-deps -v
cd ../..
