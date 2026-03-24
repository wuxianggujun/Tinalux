#!/usr/bin/env bash
set -euo pipefail

build_dir="${1:-build}"
config="${2:-Debug}"
phase="${3:-all}"

case "${phase}" in
  all)
    echo "Running Tinalux smoke tests from '${build_dir}' with config '${config}'..."
    cmake --build "${build_dir}" --config "${config}" --target TinaluxRunDesktopSmokeTests
    ;;
  build)
    echo "Building Tinalux smoke tests from '${build_dir}' with config '${config}'..."
    cmake --build "${build_dir}" --config "${config}" --target TinaluxBuildSmokeTests
    ;;
  test)
    echo "Executing Tinalux desktop smoke tests from '${build_dir}' with config '${config}'..."
    ctest --test-dir "${build_dir}" --output-on-failure --timeout 60 -LE android-scripts -C "${config}"
    ;;
  *)
    echo "Unsupported smoke phase: ${phase}" >&2
    exit 1
    ;;
esac
