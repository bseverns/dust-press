#!/usr/bin/env bash

set -euo pipefail

JUCE_TAG="7.0.12"
JUCE_SRC_DIR="native/.juce-src"
JUCE_BUILD_DIR="native/.juce-build"
JUCE_INSTALL_DIR="native/.juce-kit"
# The JUCE extras (Projucer + juceaide) live in a nested build directory.
JUCE_TOOLS_BUILD_DIR="${JUCE_BUILD_DIR}/tools"
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

echo "[dust-press] Configuring JUCE CMake build → ${JUCE_BUILD_DIR}" >&2
cmake -S "${JUCE_SRC_DIR}" -B "${JUCE_BUILD_DIR}" \
  -DCMAKE_INSTALL_PREFIX="${JUCE_INSTALL_DIR}" \
  -DJUCE_BUILD_EXTRAS=ON

echo "[dust-press] Building juceaide + installing CMake package → ${JUCE_INSTALL_DIR} (config=${JUCE_BUILD_CONFIG})" >&2
cmake --build "${JUCE_BUILD_DIR}" --target juceaide --config "${JUCE_BUILD_CONFIG}"
cmake --install "${JUCE_BUILD_DIR}" --config "${JUCE_BUILD_CONFIG}"

cat <<EOF >&2

[dust-press] Done. Point your plugin configure at ${JUCE_INSTALL_DIR}:
  cmake -S native -B native/build \\
    -DDUSTPRESS_BUILD_PLUGIN=ON \\
    -DDUSTPRESS_FETCH_JUCE=OFF \\
    -DCMAKE_PREFIX_PATH=${JUCE_INSTALL_DIR}

Re-run this script if you blow away the JUCE build tree or want a new tag (edit JUCE_TAG up top).
EOF
