#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build-macos"
DIST_DIR="${ROOT_DIR}/dist"
APP_NAME="Jingle Palette Pro"
APP_BUNDLE="${BUILD_DIR}/${APP_NAME}.app"

if [[ "$(uname -s)" != "Darwin" ]]; then
  echo "This script must be run on macOS."
  exit 1
fi

if ! command -v macdeployqt >/dev/null 2>&1; then
  echo "macdeployqt not found. Ensure Qt is installed and in PATH."
  exit 1
fi

mkdir -p "${DIST_DIR}"

cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE=Release
cmake --build "${BUILD_DIR}" --parallel

if [[ ! -d "${APP_BUNDLE}" ]]; then
  echo "App bundle not found: ${APP_BUNDLE}"
  exit 1
fi

# Ensure default audio files are available inside the bundle.
mkdir -p "${APP_BUNDLE}/Contents/Resources"
cp -f "${ROOT_DIR}/resources/Time_Announce.wav" "${APP_BUNDLE}/Contents/Resources/"
cp -f "${ROOT_DIR}/resources/West_Musette.wav" "${APP_BUNDLE}/Contents/Resources/"

# Keep compatibility with code paths that look near the executable.
cp -f "${ROOT_DIR}/resources/Time_Announce.wav" "${APP_BUNDLE}/Contents/MacOS/"
cp -f "${ROOT_DIR}/resources/West_Musette.wav" "${APP_BUNDLE}/Contents/MacOS/"

# Deploy Qt frameworks/plugins and produce DMG.
macdeployqt "${APP_BUNDLE}" -verbose=1 -always-overwrite -dmg

DMG_PATH="${BUILD_DIR}/${APP_NAME}.dmg"
if [[ ! -f "${DMG_PATH}" ]]; then
  echo "DMG not generated at ${DMG_PATH}"
  exit 1
fi

FINAL_DMG="${DIST_DIR}/Jingle_Palette_Pro-v0.3.0-macOS.dmg"
cp -f "${DMG_PATH}" "${FINAL_DMG}"

echo "DMG created: ${FINAL_DMG}"
