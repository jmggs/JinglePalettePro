#!/bin/bash
# =============================================================================
# Jingle Palette Pro — .deb autossuficiente
# Qt libs incluídas | GStreamer do sistema | Ícone + menu de aplicações
# Uso: ./make_deb.sh
# =============================================================================

set -e

APP_NAME="jingle-palette-pro"
APP_BINARY="Jingle Palette Pro"
DISPLAY_NAME="Jingle Palette Pro"
VERSION="0.1"
ARCH="amd64"
MAINTAINER="jomi <jomi@jsworkstation>"
DEB_NAME="${APP_NAME}_${VERSION}_${ARCH}.deb"
INSTALL_DIR="/opt/${APP_NAME}"
PKG_DIR="$(pwd)/deb_build/${APP_NAME}"

GREEN='\033[0;32m'; YELLOW='\033[1;33m'; RED='\033[0;31m'; BLUE='\033[0;34m'; NC='\033[0m'
info()  { echo -e "${GREEN}[✓]${NC} $1"; }
warn()  { echo -e "${YELLOW}[!]${NC} $1"; }
error() { echo -e "${RED}[✗]${NC} $1"; exit 1; }
step()  { echo -e "\n${BLUE}──── $1${NC}"; }

echo ""
echo -e "${BLUE}=================================================${NC}"
echo -e "${BLUE}   Jingle Palette Pro — .deb Builder${NC}"
echo -e "${BLUE}=================================================${NC}"

# ─── 1. Verifica ferramentas ─────────────────────────────────────────────────
step "A verificar ferramentas"
for tool in cmake g++ dpkg-deb; do
    if ! command -v $tool &>/dev/null; then
        warn "$tool não encontrado — a instalar..."
        sudo apt install -y $tool
    fi
done
info "Ferramentas OK"

# ─── 2. Compila ──────────────────────────────────────────────────────────────
step "Compilação"
if [ ! -f "build/${APP_BINARY}" ]; then
    warn "A compilar..."
    mkdir -p build && cd build
    cmake .. -DCMAKE_BUILD_TYPE=Release
    cmake --build . --parallel
    cd ..
fi
info "Binário: build/${APP_BINARY}"

# ─── 3. Localiza Qt ──────────────────────────────────────────────────────────
step "A localizar Qt"
QMAKE_BIN=$(which qmake6 2>/dev/null || which qmake 2>/dev/null || echo "")
[ -z "$QMAKE_BIN" ] && error "qmake não encontrado. Instala o Qt6."

QT_PREFIX=$($QMAKE_BIN -query QT_INSTALL_PREFIX)
QT_LIBS=$($QMAKE_BIN -query QT_INSTALL_LIBS)
QT_PLUGINS=$($QMAKE_BIN -query QT_INSTALL_PLUGINS)
info "Qt: ${QT_PREFIX}"

# ─── 4. Estrutura de diretórios ──────────────────────────────────────────────
step "A criar estrutura"
rm -rf deb_build
mkdir -p "${PKG_DIR}${INSTALL_DIR}/bin"
mkdir -p "${PKG_DIR}${INSTALL_DIR}/lib"
mkdir -p "${PKG_DIR}${INSTALL_DIR}/plugins/platforms"
mkdir -p "${PKG_DIR}${INSTALL_DIR}/plugins/audio"
mkdir -p "${PKG_DIR}${INSTALL_DIR}/plugins/multimedia"
mkdir -p "${PKG_DIR}${INSTALL_DIR}/plugins/imageformats"
mkdir -p "${PKG_DIR}${INSTALL_DIR}/data"
mkdir -p "${PKG_DIR}/usr/bin"
mkdir -p "${PKG_DIR}/usr/share/applications"
mkdir -p "${PKG_DIR}/usr/share/icons/hicolor/256x256/apps"
mkdir -p "${PKG_DIR}/usr/share/icons/hicolor/128x128/apps"
mkdir -p "${PKG_DIR}/usr/share/icons/hicolor/64x64/apps"
mkdir -p "${PKG_DIR}/usr/share/icons/hicolor/scalable/apps"
mkdir -p "${PKG_DIR}/usr/share/doc/${APP_NAME}"
mkdir -p "${PKG_DIR}/DEBIAN"
info "Estrutura criada"

# ─── 5. Binário ──────────────────────────────────────────────────────────────
step "A copiar binário"
cp "build/${APP_BINARY}" "${PKG_DIR}${INSTALL_DIR}/bin/"
chmod 755 "${PKG_DIR}${INSTALL_DIR}/bin/${APP_BINARY}"
info "OK"

# ─── 6. Qt libs ──────────────────────────────────────────────────────────────
step "A copiar Qt libs"
copy_lib() {
    local name=$1
    local found=$(find "${QT_LIBS}" /usr/lib/x86_64-linux-gnu /usr/lib \
                  -name "${name}" -not -path "*/debug/*" 2>/dev/null | head -1)
    if [ -n "$found" ]; then
        cp -L "$found" "${PKG_DIR}${INSTALL_DIR}/lib/" 2>/dev/null && \
            echo -e "  ${GREEN}✓${NC} $name" || \
            echo -e "  ${YELLOW}!${NC} $name (falhou)"
    else
        echo -e "  ${YELLOW}-${NC} $name (não encontrado)"
    fi
}

# Qt core libs
for lib in libQt6Core.so.6 libQt6Gui.so.6 libQt6Widgets.so.6 \
           libQt6Multimedia.so.6 libQt6MultimediaWidgets.so.6 \
           libQt6Network.so.6 libQt6DBus.so.6 libQt6OpenGL.so.6 \
           libQt6XcbQpa.so.6; do
    copy_lib "$lib"
done

# Libs de suporte
for lib in libicui18n.so.72 libicui18n.so.71 libicui18n.so.70 \
           libicuuc.so.72  libicuuc.so.71  libicuuc.so.70 \
           libicudata.so.72 libicudata.so.71 libicudata.so.70 \
           libmd4c.so.0 libdouble-conversion.so.3 \
           libpcre2-16.so.0 libzstd.so.1 libb2.so.1; do
    copy_lib "$lib"
done

# xcb libs para o plugin de plataforma
for lib in libxcb-util.so.1 libxcb-render-util.so.0 libxcb-image.so.0 \
           libxcb-icccm.so.4 libxcb-keysyms.so.1 libxkbcommon-x11.so.0 \
           libxkbcommon.so.0; do
    copy_lib "$lib"
done
info "Qt libs copiadas"

# ─── 7. Qt plugins ───────────────────────────────────────────────────────────
step "A copiar Qt plugins"

# Platform xcb (obrigatório no Linux X11)
for f in $(find "${QT_PLUGINS}/platforms" -name "libqxcb.so" 2>/dev/null); do
    cp -L "$f" "${PKG_DIR}${INSTALL_DIR}/plugins/platforms/" && \
        info "  platform: libqxcb.so"
done

# Wayland (opcional)
for f in $(find "${QT_PLUGINS}/platforms" -name "libqwayland*.so" 2>/dev/null); do
    cp -L "$f" "${PKG_DIR}${INSTALL_DIR}/plugins/platforms/" 2>/dev/null || true
done

# Multimedia / audio
for dir in audio multimedia; do
    src="${QT_PLUGINS}/${dir}"
    dst="${PKG_DIR}${INSTALL_DIR}/plugins/${dir}"
    mkdir -p "$dst"
    [ -d "$src" ] && cp -L ${src}/*.so "$dst/" 2>/dev/null || true
done

# Imageformats
for f in $(find "${QT_PLUGINS}/imageformats" \
           -name "libqjpeg.so" -o -name "libqpng.so" \
           -o -name "libqsvg.so" -o -name "libqico.so" 2>/dev/null); do
    cp -L "$f" "${PKG_DIR}${INSTALL_DIR}/plugins/imageformats/" 2>/dev/null || true
done
info "Plugins copiados"

# ─── 8. qt.conf ──────────────────────────────────────────────────────────────
cat > "${PKG_DIR}${INSTALL_DIR}/bin/qt.conf" << EOF
[Paths]
Prefix   = ${INSTALL_DIR}
Plugins  = ${INSTALL_DIR}/plugins
Libraries = ${INSTALL_DIR}/lib
EOF
info "qt.conf criado"

# ─── 9. Dados ────────────────────────────────────────────────────────────────
step "A copiar dados"
for f in language.ini resources/language.ini; do
    [ -f "$f" ] && cp "$f" "${PKG_DIR}${INSTALL_DIR}/data/language.ini" && break
done
for f in Time_Announce.wav resources/Time_Announce.wav; do
    [ -f "$f" ] && cp "$f" "${PKG_DIR}${INSTALL_DIR}/data/" && break
done
for f in West_Musette.wav resources/West_Musette.wav; do
    [ -f "$f" ] && cp "$f" "${PKG_DIR}${INSTALL_DIR}/data/" && break
done
info "Dados copiados"

# ─── 10. Ícone ───────────────────────────────────────────────────────────────
step "A criar ícone"
SVG_FILE="/tmp/${APP_NAME}.svg"
cat > "${SVG_FILE}" << 'SVGEOF'
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 256 256">
  <rect width="256" height="256" rx="36" fill="#12121f"/>
  <rect x="16" y="24" width="224" height="88" rx="10" fill="#0a0a14"/>
  <rect x="20" y="28" width="216" height="80" rx="8" fill="#001800"/>
  <text x="128" y="84" font-family="monospace" font-size="48"
        font-weight="bold" text-anchor="middle" fill="#00ee44">-02.57</text>
  <rect x="16"  y="124" width="42" height="28" rx="5" fill="#1e3a1e"/>
  <rect x="63"  y="124" width="42" height="28" rx="5" fill="#1e3a1e"/>
  <rect x="110" y="124" width="42" height="28" rx="5" fill="#003a00"/>
  <rect x="157" y="124" width="42" height="28" rx="5" fill="#1e3a1e"/>
  <rect x="16"  y="158" width="42" height="28" rx="5" fill="#1e3a1e"/>
  <rect x="63"  y="158" width="42" height="28" rx="5" fill="#3a3a00"/>
  <rect x="110" y="158" width="42" height="28" rx="5" fill="#1e3a1e"/>
  <rect x="157" y="158" width="42" height="28" rx="5" fill="#1e3a1e"/>
  <rect x="110" y="124" width="42" height="28" rx="5" fill="#00bb44"/>
  <text x="131" y="143" font-family="sans-serif" font-size="10"
        font-weight="bold" text-anchor="middle" fill="#fff">NEWS</text>
  <rect x="212" y="124" width="28" height="62" rx="4" fill="#080808"/>
  <rect x="215" y="163" width="22" height="21" rx="2" fill="#00cc00"/>
  <rect x="215" y="149" width="22" height="12" rx="2" fill="#99cc00"/>
  <rect x="215" y="136" width="22" height="11" rx="2" fill="#cc3300"/>
  <path d="M20 215 Q55 195 90 215 Q125 235 160 215 Q195 195 230 215"
        stroke="#0055cc" stroke-width="3" fill="none"/>
  <text x="128" y="248" font-family="sans-serif" font-size="13"
        text-anchor="middle" fill="#4466aa" font-weight="bold">JINGLE PALETTE PRO</text>
</svg>
SVGEOF

# Copia SVG
cp "${SVG_FILE}" "${PKG_DIR}/usr/share/icons/hicolor/scalable/apps/${APP_NAME}.svg"
cp "${SVG_FILE}" "${PKG_DIR}${INSTALL_DIR}/${APP_NAME}.svg"

# Converte para PNG
convert_png() {
    local size=$1
    local out="${PKG_DIR}/usr/share/icons/hicolor/${size}x${size}/apps/${APP_NAME}.png"
    if   command -v rsvg-convert &>/dev/null; then
        rsvg-convert -w $size -h $size "${SVG_FILE}" -o "$out" 2>/dev/null && return 0
    elif command -v inkscape &>/dev/null; then
        inkscape "${SVG_FILE}" -w $size -h $size -o "$out" 2>/dev/null && return 0
    elif command -v convert &>/dev/null; then
        convert -background none "${SVG_FILE}" -resize ${size}x${size} "$out" 2>/dev/null && return 0
    fi
    return 1
}

PNG_OK=false
for size in 256 128 64; do
    convert_png $size && info "  PNG ${size}px" && PNG_OK=true
done

if [ "$PNG_OK" = false ]; then
    warn "Para converter o ícone SVG para PNG instala: sudo apt install librsvg2-bin"
    warn "O ícone SVG foi incluído na mesma — funciona na maioria dos ambientes."
fi
info "Ícone criado"

# ─── 11. Launcher /usr/bin ───────────────────────────────────────────────────
step "A criar launcher"
cat > "${PKG_DIR}/usr/bin/${APP_NAME}" << WRAPEOF
#!/bin/bash
INSTALL_DIR="${INSTALL_DIR}"
USER_DIR="\$HOME/.jinglepalette"
mkdir -p "\$USER_DIR"

# Copia dados na primeira execução
for f in language.ini Time_Announce.wav West_Musette.wav; do
    [ ! -f "\$USER_DIR/\$f" ] && [ -f "\$INSTALL_DIR/data/\$f" ] && \
        cp "\$INSTALL_DIR/data/\$f" "\$USER_DIR/"
done

# Qt autossuficiente
export LD_LIBRARY_PATH="\$INSTALL_DIR/lib:\$LD_LIBRARY_PATH"
export QT_PLUGIN_PATH="\$INSTALL_DIR/plugins"
export QT_QPA_PLATFORM_PLUGIN_PATH="\$INSTALL_DIR/plugins/platforms"

# GStreamer: usa o do sistema
unset GST_PLUGIN_PATH
unset GST_PLUGIN_SYSTEM_PATH_1_0

cd "\$USER_DIR"
exec "\$INSTALL_DIR/bin/${APP_BINARY}" "\$@"
WRAPEOF
chmod 755 "${PKG_DIR}/usr/bin/${APP_NAME}"
info "Launcher: /usr/bin/${APP_NAME}"

# ─── 12. .desktop ────────────────────────────────────────────────────────────
step "A criar entrada no menu"
cat > "${PKG_DIR}/usr/share/applications/${APP_NAME}.desktop" << EOF
[Desktop Entry]
Version=1.0
Type=Application
Name=${DISPLAY_NAME}
GenericName=Jingle Player
Comment=Instant jingle player for radio broadcasting
Exec=${APP_NAME}
Icon=${APP_NAME}
Categories=AudioVideo;Audio;Music;
Keywords=jingle;radio;audio;player;broadcast;
MimeType=audio/mpeg;audio/ogg;audio/wav;
StartupNotify=true
StartupWMClass=${APP_BINARY}
Terminal=false
EOF
info ".desktop criado → aparece em Som e Vídeo"

# ─── 13. Documentação ────────────────────────────────────────────────────────
cat > "${PKG_DIR}/usr/share/doc/${APP_NAME}/copyright" << EOF
${DISPLAY_NAME} — Qt/C++ cross-platform port
Original: Copyright (C) Horvárkosi Róbert <horvark@gmail.com>
Website: http://www.horvark.hu/jinglepalette

Freeware. Free for personal and commercial radio use.
Qt Framework: LGPLv3 — The Qt Company Ltd.
EOF
printf "${APP_NAME} (${VERSION})\n\n  * Qt/C++ port\n\n -- ${MAINTAINER}  $(date -R)\n" | \
    gzip -9 > "${PKG_DIR}/usr/share/doc/${APP_NAME}/changelog.Debian.gz"

# ─── 14. DEBIAN/control ──────────────────────────────────────────────────────
step "A criar DEBIAN/control"
INSTALLED_SIZE=$(du -sk "${PKG_DIR}" | cut -f1)
cat > "${PKG_DIR}/DEBIAN/control" << EOF
Package: ${APP_NAME}
Version: ${VERSION}
Architecture: ${ARCH}
Maintainer: ${MAINTAINER}
Installed-Size: ${INSTALLED_SIZE}
Depends: libgstreamer1.0-0,
 gstreamer1.0-plugins-good,
 gstreamer1.0-plugins-bad,
 gstreamer1.0-libav,
 gstreamer1.0-pulseaudio | gstreamer1.0-alsa,
 libxcb1,
 libx11-6,
 libgl1
Recommends: gstreamer1.0-plugins-ugly
Section: sound
Priority: optional
Homepage: http://www.horvark.hu/jinglepalette
Description: Instant jingle player for radio broadcasting
 ${DISPLAY_NAME} is a professional instant jingle player for radio
 broadcast studios. Features 30 jingles per palette, auto-mix, time
 announce, VU meter and internet stream playback.
 .
 Supports MP3, WAV, OGG and streaming audio.
 Qt libraries bundled — no Qt installation required.
EOF
info "control criado"

# ─── 15. Scripts pós-instalação ──────────────────────────────────────────────
cat > "${PKG_DIR}/DEBIAN/postinst" << 'EOF'
#!/bin/bash
set -e
command -v update-desktop-database &>/dev/null && \
    update-desktop-database /usr/share/applications 2>/dev/null || true
command -v gtk-update-icon-cache &>/dev/null && \
    gtk-update-icon-cache -f -t /usr/share/icons/hicolor 2>/dev/null || true
echo ""
echo "✓ Jingle Palette Pro instalado!"
echo "  Menu → Som e Vídeo → Jingle Palette Pro"
echo "  Terminal: jingle-palette-pro"
echo ""
EOF
chmod 755 "${PKG_DIR}/DEBIAN/postinst"

cat > "${PKG_DIR}/DEBIAN/postrm" << 'EOF'
#!/bin/bash
update-desktop-database /usr/share/applications 2>/dev/null || true
gtk-update-icon-cache -f -t /usr/share/icons/hicolor 2>/dev/null || true
EOF
chmod 755 "${PKG_DIR}/DEBIAN/postrm"

# ─── 16. Permissões ──────────────────────────────────────────────────────────
find "${PKG_DIR}" -type d -exec chmod 755 {} \;
find "${PKG_DIR}" -type f -exec chmod 644 {} \;
chmod 755 "${PKG_DIR}${INSTALL_DIR}/bin/${APP_BINARY}"
chmod 755 "${PKG_DIR}/usr/bin/${APP_NAME}"
chmod 755 "${PKG_DIR}/DEBIAN/postinst"
chmod 755 "${PKG_DIR}/DEBIAN/postrm"

# ─── 17. Gera .deb ───────────────────────────────────────────────────────────
step "A gerar .deb"
dpkg-deb --build --root-owner-group "${PKG_DIR}" "${DEB_NAME}"

# ─── Resultado ───────────────────────────────────────────────────────────────
echo ""
echo -e "${GREEN}=================================================${NC}"
echo -e "${GREEN}   ✓ Pacote criado com sucesso!${NC}"
echo -e "${GREEN}=================================================${NC}"
echo ""
echo -e "  Ficheiro  : ${YELLOW}${DEB_NAME}${NC}"
echo -e "  Tamanho   : $(du -sh ${DEB_NAME} | cut -f1)"
echo -e "  Instala em: ${INSTALL_DIR}"
echo ""
echo -e "${BLUE}  Instalar:${NC}"
echo -e "    sudo apt install ./${DEB_NAME}"
echo ""
echo -e "${BLUE}  Após instalar:${NC}"
echo -e "    Menu → Som e Vídeo → ${DISPLAY_NAME}"
echo -e "    Terminal: ${APP_NAME}"
echo ""
echo -e "${BLUE}  Desinstalar:${NC}"
echo -e "    sudo apt remove ${APP_NAME}"
echo ""
echo -e "${BLUE}  Dados do utilizador:${NC}"
echo -e "    ~/.jinglepalette/"
echo ""
