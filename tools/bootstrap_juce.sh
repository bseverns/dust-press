#!/usr/bin/env bash

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
JUCE_TAG="${JUCE_TAG:-8.0.11}"
JUCE_SRC_DIR="${JUCE_SRC_DIR:-${REPO_ROOT}/native/.juce-src}"
JUCE_BUILD_DIR="${JUCE_BUILD_DIR:-${REPO_ROOT}/native/.juce-build}"
JUCE_INSTALL_DIR="${JUCE_INSTALL_DIR:-${REPO_ROOT}/native/.juce-kit}"
JUCE_TOOLS_BUILD_DIR="${JUCE_BUILD_DIR}/tools"

require_bin() {
  if ! command -v "$1" >/dev/null 2>&1; then
    cat <<EOF >&2
[dust-press] Missing dependency: $1
  Install it before bootstrapping JUCE. On Debian/Ubuntu: sudo apt-get install -y $1
  On macOS with Homebrew: brew install $1
EOF
    exit 1
  fi
}

require_bin git
require_bin cmake

# Xcode and other multi-config generators need an explicit configuration.
# Let users override it (e.g. `JUCE_BUILD_CONFIG=Debug ./tools/bootstrap_juce.sh`)
# but default to Release so we mirror JUCE's own guidance.
JUCE_BUILD_CONFIG="${JUCE_BUILD_CONFIG:-Release}"

echo "[dust-press] Bootstrapping JUCE ${JUCE_TAG} into ${JUCE_BUILD_DIR}" >&2

if [ ! -d "${JUCE_SRC_DIR}" ]; then
  echo "[dust-press] Cloning JUCE → ${JUCE_SRC_DIR}" >&2
  git clone --branch "${JUCE_TAG}" --depth 1 https://github.com/juce-framework/JUCE.git "${JUCE_SRC_DIR}"
else
  echo "[dust-press] JUCE source already present at ${JUCE_SRC_DIR}; skipping clone" >&2
fi

if [ -f "${JUCE_BUILD_DIR}/CMakeCache.txt" ]; then
  cache_home=$(grep -E '^CMAKE_HOME_DIRECTORY:INTERNAL=' "${JUCE_BUILD_DIR}/CMakeCache.txt" | cut -d= -f2- || true)
  if [ "${cache_home}" != "${JUCE_SRC_DIR}" ]; then
    echo "[dust-press] Nuking stale JUCE build cache (it points at ${cache_home:-unknown}) → ${JUCE_BUILD_DIR}" >&2
    rm -rf "${JUCE_BUILD_DIR}"
  fi
fi

echo "[dust-press] Configuring JUCE CMake build → ${JUCE_BUILD_DIR}" >&2
cmake -S "${JUCE_SRC_DIR}" -B "${JUCE_BUILD_DIR}" \
  -DCMAKE_INSTALL_PREFIX="${JUCE_INSTALL_DIR}" \
  -DJUCE_BUILD_EXTRAS=ON

echo "[dust-press] Building JUCE helper targets + installing CMake package → ${JUCE_INSTALL_DIR} (config=${JUCE_BUILD_CONFIG})" >&2

# JUCE's juceaide target *usually* sits in the top-level build graph, but on
# some generators (notably Makefiles on macOS), the target can be tucked under
# tools/ or extras/Build instead. Try a short list of likely build dirs before
# we give up so the script works on more default setups.
juce_helpers_built=0
juce_helper_targets=(
  juceaide
  juce_lv2_helper
)
juceaide_build_dirs=(
  "${JUCE_BUILD_DIR}"
  "${JUCE_TOOLS_BUILD_DIR}"
  "${JUCE_BUILD_DIR}/extras/Build"
)

for build_dir in "${juceaide_build_dirs[@]}"; do
  if [ -d "${build_dir}" ]; then
    for target in "${juce_helper_targets[@]}"; do
      echo "[dust-press] Trying ${target} build from ${build_dir}" >&2
      if ! cmake --build "${build_dir}" --target "${target}" --config "${JUCE_BUILD_CONFIG}"; then
        echo "[dust-press] ${target} not available in ${build_dir}" >&2
        continue 2
      fi
    done
    juce_helpers_built=1
    break
  fi
done

if [ "${juce_helpers_built}" -ne 1 ]; then
  cat <<EOF >&2
[dust-press] Uh-oh: couldn't build the JUCE helper targets (juceaide + juce_lv2_helper).
  We tried these build dirs:
    - ${JUCE_BUILD_DIR}
    - ${JUCE_TOOLS_BUILD_DIR}
    - ${JUCE_BUILD_DIR}/extras/Build

  If your JUCE build lives somewhere else (for example: native/Build),
  export JUCE_BUILD_DIR before running this script, or wipe ${JUCE_BUILD_DIR}
  and re-run so the configure step can generate the extras/tools targets.
EOF
  exit 1
fi

cmake --install "${JUCE_BUILD_DIR}" --config "${JUCE_BUILD_CONFIG}"

cat <<EOF >&2

[dust-press] Done. Point your plugin configure at ${JUCE_INSTALL_DIR}:
  cmake -S native -B native/build \\
    -DDUSTPRESS_BUILD_PLUGIN=ON \\
    -DDUSTPRESS_FETCH_JUCE=OFF \\
    -DCMAKE_PREFIX_PATH=${JUCE_INSTALL_DIR}

Re-run this script if you blow away the JUCE build tree or want a new tag (edit JUCE_TAG up top).
EOF
