#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"
APPDIR="${ROOT_DIR}/AppDir"
DIST_DIR="${ROOT_DIR}/dist"
TOOLS_DIR="${ROOT_DIR}/.tools"
APP_NAME="jingle-palette-pro"
APP_BIN_NAME="Jingle Palette Pro"
VERSION="0.3.0"

mkdir -p "${BUILD_DIR}" "${DIST_DIR}" "${TOOLS_DIR}"

cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE=Release
cmake --build "${BUILD_DIR}" --parallel

rm -rf "${APPDIR}"
mkdir -p "${APPDIR}/usr/bin"
mkdir -p "${APPDIR}/usr/lib/gstreamer-1.0"
mkdir -p "${APPDIR}/usr/libexec/gstreamer-1.0"
mkdir -p "${APPDIR}/usr/share/applications"
mkdir -p "${APPDIR}/usr/share/icons/hicolor/256x256/apps"
mkdir -p "${APPDIR}/usr/share/jinglepalette"

cp "${BUILD_DIR}/${APP_BIN_NAME}" "${APPDIR}/usr/bin/${APP_BIN_NAME}"
cp "${ROOT_DIR}/logo.png" "${APPDIR}/usr/share/icons/hicolor/256x256/apps/${APP_NAME}.png"
cp "${ROOT_DIR}/logo.png" "${APPDIR}/${APP_NAME}.png"

for f in Time_Announce.wav West_Musette.wav; do
  if [[ -f "${ROOT_DIR}/resources/${f}" ]]; then
    cp "${ROOT_DIR}/resources/${f}" "${APPDIR}/usr/share/jinglepalette/"
  fi
done

cat > "${APPDIR}/usr/share/applications/${APP_NAME}.desktop" << EOF
[Desktop Entry]
Type=Application
Name=Jingle Palette Pro
Exec=${APP_NAME}
Icon=${APP_NAME}
Categories=AudioVideo;Audio;Music;
Comment=Instant jingle player for radio broadcasting
Terminal=false
StartupNotify=true
EOF

cat > "${APPDIR}/AppRun" << 'EOF'
#!/usr/bin/env bash
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
export LD_LIBRARY_PATH="${HERE}/usr/lib:${LD_LIBRARY_PATH:-}"
export QT_PLUGIN_PATH="${HERE}/usr/plugins:${HERE}/usr/lib/qt6/plugins:${QT_PLUGIN_PATH:-}"
export QT_QPA_PLATFORM_PLUGIN_PATH="${HERE}/usr/plugins/platforms:${HERE}/usr/lib/qt6/plugins/platforms:${QT_QPA_PLATFORM_PLUGIN_PATH:-}"
export GST_PLUGIN_PATH="${HERE}/usr/lib/gstreamer-1.0"
export GST_PLUGIN_SYSTEM_PATH_1_0=""
if [[ -x "${HERE}/usr/libexec/gstreamer-1.0/gst-plugin-scanner" ]]; then
  export GST_PLUGIN_SCANNER="${HERE}/usr/libexec/gstreamer-1.0/gst-plugin-scanner"
fi
exec "${HERE}/usr/bin/Jingle Palette Pro" "$@"
EOF
chmod +x "${APPDIR}/AppRun"

GST_PLUGIN_DIR=$(pkg-config --variable=pluginsdir gstreamer-1.0 2>/dev/null || echo "/usr/lib/x86_64-linux-gnu/gstreamer-1.0")
if [[ -d "${GST_PLUGIN_DIR}" ]]; then
  cp -L "${GST_PLUGIN_DIR}"/*.so "${APPDIR}/usr/lib/gstreamer-1.0/" 2>/dev/null || true
fi
for scanner in \
  /usr/libexec/gstreamer-1.0/gst-plugin-scanner \
  /usr/lib/x86_64-linux-gnu/gstreamer1.0/gstreamer-1.0/gst-plugin-scanner; do
  if [[ -f "${scanner}" ]]; then
    cp -L "${scanner}" "${APPDIR}/usr/libexec/gstreamer-1.0/" 2>/dev/null || true
    chmod +x "${APPDIR}/usr/libexec/gstreamer-1.0/gst-plugin-scanner" || true
    break
  fi
done

LINUXDEPLOY="${TOOLS_DIR}/linuxdeploy-x86_64.AppImage"
QT_PLUGIN="${TOOLS_DIR}/linuxdeploy-plugin-qt-x86_64.AppImage"
if [[ ! -f "${LINUXDEPLOY}" ]]; then
  curl -L "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage" -o "${LINUXDEPLOY}"
fi
if [[ ! -f "${QT_PLUGIN}" ]]; then
  curl -L "https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage" -o "${QT_PLUGIN}"
fi
chmod +x "${LINUXDEPLOY}" "${QT_PLUGIN}"

export VERSION="${VERSION}"
export LINUXDEPLOY_PLUGIN_QT_APPIMAGE="${QT_PLUGIN}"

"${LINUXDEPLOY}" \
  --appdir "${APPDIR}" \
  -e "${APPDIR}/usr/bin/${APP_BIN_NAME}" \
  -d "${APPDIR}/usr/share/applications/${APP_NAME}.desktop" \
  -i "${APPDIR}/${APP_NAME}.png" \
  --plugin qt \
  --output appimage

mv -f "${ROOT_DIR}"/*.AppImage "${DIST_DIR}/"

echo "AppImage created in ${DIST_DIR}"
