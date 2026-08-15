#!/usr/bin/env bash

set -euo pipefail

phase0_script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
phase0_repo="$(cd -- "$phase0_script_dir/../.." && pwd)"
phase0_package_dir="$phase0_repo/.tools/packages/phase0"
phase0_sysroot="$phase0_repo/.tools/phase0/sysroot"

source /etc/os-release
if [[ "${ID:-}" != "ubuntu" || "${VERSION_CODENAME:-}" != "noble" ]]; then
  echo "This local fallback is limited to Ubuntu 24.04 (noble)." >&2
  exit 2
fi
if ! command -v g++ >/dev/null 2>&1; then
  echo "The local fallback requires the base Ubuntu G++ runtime and headers." >&2
  exit 2
fi

phase0_packages=(
  cmake cmake-data libarchive13t64 libjsoncpp25 librhash0
  ninja-build
  clang-18 clangd-18 clang-format-18 clang-tidy-18
  libclang-cpp18 libllvm18 libclang-common-18-dev
  llvm-18-linker-tools libclang1-18 libobjc-13-dev
  libabsl20220623t64 libgrpc++1.51t64 libgrpc29t64 libprotobuf32t64
  gdb libbabeltrace1 libdebuginfod1t64 libipt2 libsource-highlight4t64
  pkg-config pkgconf pkgconf-bin libpkgconf3
  ccache libhiredis1.1.0
  mesa-utils mesa-utils-bin
  libgl1-mesa-dev libegl1-mesa-dev libgles2-mesa-dev mesa-common-dev
  libgl-dev libglvnd-dev libegl-dev libgles-dev libglx-dev
  libgl1 libglx0 libegl1 libgles1 libgles2 libglvnd0 libglx-mesa0
  libx11-dev libx11-6 libxau-dev libxdmcp-dev x11proto-dev xtrans-dev
  libxcb1-dev libpthread-stubs0-dev
  libxext-dev libxext6 libxrandr-dev libxrandr2 libxrender-dev libxrender1
  libxcursor-dev libxcursor1 libxfixes-dev libxfixes3 libxi-dev libxi6
  libxss-dev libxss1 libxtst-dev libxtst6 xorg-sgml-doctools
  libxkbcommon-dev libxkbcommon0
  libwayland-dev libwayland-bin libwayland-client0 libwayland-cursor0
  libwayland-egl1 libwayland-server0
  libdecor-0-dev libdecor-0-0 libffi-dev
  libdrm-dev libdrm2 libdrm-intel1 libdrm-radeon1 libdrm-nouveau2
  libdrm-amdgpu1 libpciaccess-dev libgbm-dev libgbm1
)

mkdir -p "$phase0_package_dir" "$phase0_sysroot"
(
  cd "$phase0_package_dir"
  apt download "${phase0_packages[@]}"
  sha256sum ./*.deb > phase0-packages.sha256
)

for phase0_package in "$phase0_package_dir"/*.deb; do
  dpkg-deb -x "$phase0_package" "$phase0_sysroot"
done

ln -sfn clang-18 "$phase0_sysroot/usr/bin/clang"
ln -sfn clang++-18 "$phase0_sysroot/usr/bin/clang++"
ln -sfn clangd-18 "$phase0_sysroot/usr/bin/clangd"
ln -sfn clang-format-18 "$phase0_sysroot/usr/bin/clang-format"
ln -sfn clang-tidy-18 "$phase0_sysroot/usr/bin/clang-tidy"

echo "Local Phase 0 toolchain extracted to $phase0_sysroot"
