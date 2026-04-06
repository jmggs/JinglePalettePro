#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
APPDIR="${ROOT_DIR}/AppDir"
PKGROOT="${ROOT_DIR}/deb_full_build"
PKGNAME="jingle-palette-pro"
VERSION="0.3.0"
ARCH="amd64"
INSTALL_DIR="/opt/jingle-palette-pro"
APP_BIN_NAME="Jingle Palette Pro"
OUT_DEB="${ROOT_DIR}/${PKGNAME}_${VERSION}_${ARCH}.deb"

if [[ ! -d "${APPDIR}/usr" ]]; then
  echo "AppDir not found. Run build_scripts/linux/build_appimage.sh first."
  exit 1
fi

rm -rf "${PKGROOT}"
mkdir -p "${PKGROOT}/DEBIAN"
mkdir -p "${PKGROOT}${INSTALL_DIR}"
mkdir -p "${PKGROOT}/usr/bin"
mkdir -p "${PKGROOT}/usr/share/applications"
mkdir -p "${PKGROOT}/usr/share/icons/hicolor/256x256/apps"

cp -a "${APPDIR}/usr" "${PKGROOT}${INSTALL_DIR}/"
cp -a "${APPDIR}/AppRun" "${PKGROOT}${INSTALL_DIR}/AppRun"
chmod +x "${PKGROOT}${INSTALL_DIR}/AppRun"

cat > "${PKGROOT}/usr/bin/jingle-palette-pro" << 'EOF'
#!/usr/bin/env bash
exec /opt/jingle-palette-pro/AppRun "$@"
EOF
chmod +x "${PKGROOT}/usr/bin/jingle-palette-pro"

if [[ -f "${APPDIR}/usr/share/applications/jingle-palette-pro.desktop" ]]; then
  cp "${APPDIR}/usr/share/applications/jingle-palette-pro.desktop" "${PKGROOT}/usr/share/applications/"
fi
if [[ -f "${APPDIR}/usr/share/icons/hicolor/256x256/apps/jingle-palette-pro.png" ]]; then
  cp "${APPDIR}/usr/share/icons/hicolor/256x256/apps/jingle-palette-pro.png" \
     "${PKGROOT}/usr/share/icons/hicolor/256x256/apps/"
fi

INSTALLED_SIZE=$(du -sk "${PKGROOT}" | cut -f1)
cat > "${PKGROOT}/DEBIAN/control" << EOF
Package: ${PKGNAME}
Version: ${VERSION}
Architecture: ${ARCH}
Maintainer: jomi <jomi@jsworkstation>
Installed-Size: ${INSTALLED_SIZE}
Depends: libc6, libstdc++6
Provides: jingle-palette-pro-full
Conflicts: jingle-palette-pro-full
Replaces: jingle-palette-pro-full
Section: sound
Priority: optional
Homepage: https://github.com/jmggs/JinglePalettePro
Description: Jingle Palette Pro (full self-contained bundle)
 Full runtime bundle generated from linuxdeploy AppDir with Qt and multimedia dependencies.
EOF

cat > "${PKGROOT}/DEBIAN/postinst" << 'EOF'
#!/usr/bin/env bash
set -e
command -v update-desktop-database >/dev/null 2>&1 && update-desktop-database /usr/share/applications || true
command -v gtk-update-icon-cache >/dev/null 2>&1 && gtk-update-icon-cache -f -t /usr/share/icons/hicolor || true
EOF
chmod 755 "${PKGROOT}/DEBIAN/postinst"

find "${PKGROOT}" -type d -exec chmod 755 {} \;
find "${PKGROOT}" -type f -exec chmod 644 {} \;
chmod 755 "${PKGROOT}/usr/bin/jingle-palette-pro" "${PKGROOT}${INSTALL_DIR}/AppRun" "${PKGROOT}/DEBIAN/postinst"
chmod 755 "${PKGROOT}${INSTALL_DIR}/usr/bin/${APP_BIN_NAME}" || true
find "${PKGROOT}${INSTALL_DIR}" -type f \( -name "*.so" -o -name "*.so.*" \) -exec chmod 755 {} \; || true
find "${PKGROOT}${INSTALL_DIR}" -type f -path "*/plugins/*" -exec chmod 755 {} \; || true
find "${PKGROOT}${INSTALL_DIR}" -type f -path "*/libexec/*" -exec chmod 755 {} \; || true

rm -f "${OUT_DEB}"
dpkg-deb --build --root-owner-group "${PKGROOT}" "${OUT_DEB}"

echo "Created: ${OUT_DEB}"
du -sh "${OUT_DEB}"
