#!/usr/bin/env bash

set -euo pipefail

phase0_script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
phase0_repo="$(cd -- "$phase0_script_dir/../.." && pwd)"
phase0_sysroot="$phase0_repo/.tools/phase0/sysroot"
phase0_sdl_prefix="${WIDE_EYE_PHASE0_SDL_PREFIX:-$phase0_repo/.tools/phase0/install-x11}"
phase0_build="${WIDE_EYE_PHASE0_BUILD_DIR:-$phase0_repo/.tools/phase0/build/context-smoke}"

phase0_cmake="$(command -v cmake || true)"
phase0_ninja="$(command -v ninja || true)"
phase0_clang="$(command -v clang-18 || true)"
phase0_clangxx="$(command -v clang++-18 || true)"
phase0_toolchain_args=()

if [[ (-z "$phase0_cmake" || -z "$phase0_ninja" ||
       -z "$phase0_clang" || -z "$phase0_clangxx") &&
      -x "$phase0_sysroot/usr/bin/cmake" ]]; then
  phase0_cmake="$phase0_sysroot/usr/bin/cmake"
  phase0_ninja="$phase0_sysroot/usr/bin/ninja"
  phase0_clang="$phase0_sysroot/usr/bin/clang-18"
  phase0_clangxx="$phase0_sysroot/usr/bin/clang++-18"
  phase0_runtime_libs="$phase0_sysroot/usr/lib/x86_64-linux-gnu:$phase0_sysroot/usr/lib/llvm-18/lib"
  export LD_LIBRARY_PATH="$phase0_runtime_libs${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
  export PKG_CONFIG_SYSROOT_DIR="$phase0_sysroot"
  export PKG_CONFIG_LIBDIR="$phase0_sysroot/usr/lib/x86_64-linux-gnu/pkgconfig:$phase0_sysroot/usr/share/pkgconfig"
  phase0_toolchain_args=(
    -DCMAKE_INCLUDE_PATH="$phase0_sysroot/usr/include"
    -DCMAKE_LIBRARY_PATH="$phase0_sysroot/usr/lib/x86_64-linux-gnu"
  )
fi

if [[ -z "$phase0_cmake" || -z "$phase0_ninja" ||
      -z "$phase0_clang" || -z "$phase0_clangxx" ]]; then
  echo "Phase 0 toolchain is missing; see docs/setup/UBUNTU_24_04.md." >&2
  exit 2
fi

phase0_prefix_args=()
if [[ -f "$phase0_sdl_prefix/lib/cmake/SDL3/SDL3Config.cmake" ]]; then
  phase0_prefix_path="$phase0_sdl_prefix"
  if [[ -d "$phase0_sysroot/usr" ]]; then
    phase0_prefix_path="$phase0_prefix_path;$phase0_sysroot/usr"
  fi
  phase0_prefix_args=(-DCMAKE_PREFIX_PATH="$phase0_prefix_path")
elif [[ -d "$phase0_sysroot/usr" ]]; then
  phase0_prefix_args=(-DCMAKE_PREFIX_PATH="$phase0_sysroot/usr")
fi

"$phase0_cmake" \
  -S "$phase0_script_dir/context-smoke" \
  -B "$phase0_build" \
  -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_MAKE_PROGRAM="$phase0_ninja" \
  -DCMAKE_C_COMPILER="$phase0_clang" \
  -DCMAKE_CXX_COMPILER="$phase0_clangxx" \
  "${phase0_toolchain_args[@]}" \
  "${phase0_prefix_args[@]}"
"$phase0_cmake" --build "$phase0_build" --parallel

phase0_video_driver="${WIDE_EYE_PHASE0_VIDEO_DRIVER:-x11}"
env SDL_VIDEO_DRIVER="$phase0_video_driver" \
  "$phase0_build/wide-eye-context-smoke" "$@"
