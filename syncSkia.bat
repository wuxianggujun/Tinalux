@echo off
setlocal
if not exist "3rdparty/skia" (
    git clone https://skia.googlesource.com/skia.git --depth 1 3rdparty/skia
    if errorlevel 1 exit /b 1
    pushd 3rdparty\skia
    if errorlevel 1 exit /b 1
) else (
    pushd 3rdparty\skia
    if errorlevel 1 exit /b 1
    git pull --depth=1 https://skia.googlesource.com/skia.git main --allow-unrelated-histories
    if errorlevel 1 (
        popd
        exit /b 1
    )
)
set PYTHONUTF8=1
set GIT_SYNC_DEPS_SKIP_EMSDK=1
set "_TINALUX_PYTHON="
where py >nul 2>nul && set "_TINALUX_PYTHON=py -3"
if not defined _TINALUX_PYTHON (
    where python3 >nul 2>nul && set "_TINALUX_PYTHON=python3"
)
if not defined _TINALUX_PYTHON (
    where python >nul 2>nul && set "_TINALUX_PYTHON=python"
)
if not defined _TINALUX_PYTHON (
    echo Failed to locate a Python interpreter. Install py, python3, or python.
    popd
    exit /b 1
)
%_TINALUX_PYTHON% tools/git-sync-deps -v
if errorlevel 1 (
    popd
    exit /b 1
)
popd
endlocal
