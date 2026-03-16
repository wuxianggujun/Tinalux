#!/usr/bin/env bash
set -euo pipefail

build_dir="${1:-build}"
config="${2:-Debug}"

echo "Running Tinalux smoke tests from '${build_dir}' with config '${config}'..."
cmake --build "${build_dir}" --config "${config}" --target TinaluxRunSmokeTests
