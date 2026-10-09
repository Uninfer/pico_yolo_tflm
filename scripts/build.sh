#!/usr/bin/env bash
set -euo pipefail
project_dir="$(cd "$(dirname "$0")/.." && pwd)"
build_dir="${BUILD_DIR:-${project_dir}/build}"
cmake -S "${project_dir}" -B "${build_dir}" -G Ninja -DCMAKE_BUILD_TYPE=Release "$@"
cmake --build "${build_dir}" --parallel
