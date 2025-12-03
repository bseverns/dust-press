#!/usr/bin/env bash

set -euo pipefail

JUCE_TAG="7.0.12"
JUCE_SRC_DIR="native/.juce-src"
JUCE_BUILD_DIR="native/.juce-build"

echo "[dust-press] Bootstrapping JUCE ${JUCE_TAG} into ${JUCE_BUILD_DIR}" >&2

if [ ! -d "${JUCE_SRC_DIR}" ]; then
  echo "[dust-press] Cloning JUCE → ${JUCE_SRC_DIR}" >&2
  git clone --branch "${JUCE_TAG}" --depth 1 https://github.com/juce-framework/JUCE.git "${JUCE_SRC_DIR}"
else
  echo "[dust-press] JUCE source already present at ${JUCE_SRC_DIR}; skipping clone" >&2
fi

echo "[dust-press] Configuring JUCE CMake build → ${JUCE_BUILD_DIR}" >&2
cmake -S "${JUCE_SRC_DIR}" -B "${JUCE_BUILD_DIR}"

cat <<EOF >&2

[dust-press] Done. Point your plugin configure at ${JUCE_BUILD_DIR}:
  cmake -S native -B native/build \\
    -DDUSTPRESS_BUILD_PLUGIN=ON \\
    -DDUSTPRESS_FETCH_JUCE=OFF \\
    -DCMAKE_PREFIX_PATH=${JUCE_BUILD_DIR}

Re-run this script if you blow away the JUCE build tree or want a new tag (edit JUCE_TAG up top).
EOF
