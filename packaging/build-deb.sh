#!/bin/bash
# Build the pi-carplay .deb and the GPL corresponding-source tarball.
#
#   packaging/build-deb.sh <version> [src_dir] [out_dir]
#
# Expects `make build` to have run: the shell in raspi-headunit/build and
# OpenAuto's autoapp (with aasdk and h264bitstream) under src_dir.
set -euo pipefail

VERSION=${1:?usage: build-deb.sh <version> [src_dir] [out_dir]}
REPO=$(cd "$(dirname "$0")/.." && pwd)
SRC_DIR=$(realpath "${2:-$HOME/src}")
OUT_DIR=$(realpath -m "${3:-$REPO/dist}")
ARCH=$(dpkg --print-architecture)
PKG=pi-carplay
PREFIX=opt/pi-carplay

HEADUNIT_BIN=$REPO/raspi-headunit/build/headunit
AUTOAPP_BIN=$SRC_DIR/openauto/bin/autoapp
for f in "$HEADUNIT_BIN" "$AUTOAPP_BIN"; do
  [ -x "$f" ] || { echo "missing $f; run make build first" >&2; exit 1; }
done

WORK=$(mktemp -d)
trap 'rm -rf "$WORK"' EXIT
STAGE=$WORK/${PKG}_${VERSION}_${ARCH}
mkdir -p "$STAGE/DEBIAN" "$STAGE/$PREFIX"/{bin,lib,share/labwc} "$STAGE/usr/bin" \
  "$STAGE/lib/udev/rules.d" "$STAGE/usr/share/doc/$PKG"

render() {
  sed -e "s|@HEADUNIT@|/$PREFIX/bin/headunit|g" \
      -e "s|@SUPERVISE@|/usr/bin/pi-carplay-supervise|g" \
      -e "s|@AUTOAPP_LAUNCHER@|/usr/bin/pi-carplay-autoapp|g" \
      -e "s|@AUTOAPP@|/$PREFIX/bin/autoapp|g" \
      -e "s|@OPENAUTO_DIR@|\$HOME/.config/pi-carplay/openauto|g" \
      -e "s|@DEFAULT_INI@|/$PREFIX/share/openauto.ini|g" "$1"
}

# Binaries
install -m 0755 "$HEADUNIT_BIN" "$STAGE/$PREFIX/bin/headunit"
install -m 0755 "$AUTOAPP_BIN" "$STAGE/$PREFIX/bin/autoapp"

# Bundle every library autoapp resolves outside the system library paths
# (libopenauto, libbtservice_proto, libaasdk, libaasdk_proto, ...).
while read -r lib path; do
  case "$path" in
    /lib/*|/usr/lib/*|"") continue ;;
  esac
  cp -L "$path" "$STAGE/$PREFIX/lib/$lib"
done < <(ldd "$AUTOAPP_BIN" | awk '$2 == "=>" {print $1, $3}')
strip --strip-unneeded "$STAGE/$PREFIX/bin"/* "$STAGE/$PREFIX/lib"/*.so*
chmod 0644 "$STAGE/$PREFIX/lib"/*.so*
for f in "$STAGE/$PREFIX/bin/autoapp" "$STAGE/$PREFIX/lib"/*.so*; do
  # shellcheck disable=SC2016  # $ORIGIN is resolved by the dynamic loader
  patchelf --set-rpath '$ORIGIN/../lib' "$f"
done

# Launchers and setup helpers
render "$REPO/deploy/bin/pi-carplay-autoapp" > "$STAGE/usr/bin/pi-carplay-autoapp"
render "$REPO/deploy/bin/aa-restart.sh" > "$STAGE/usr/bin/pi-carplay-restart"
install -m 0755 "$REPO/deploy/bin/pi-carplay-boot-config" "$REPO/deploy/bin/pi-carplay-supervise" \
  "$REPO/tools/pi-carplay-selftest" "$STAGE/usr/bin/"
install -m 0755 "$REPO/packaging/pi-carplay-setup" "$STAGE/usr/bin/"
chmod 0755 "$STAGE/usr/bin/pi-carplay-autoapp" "$STAGE/usr/bin/pi-carplay-restart"

# Config templates and system files
render "$REPO/deploy/labwc/autostart" > "$STAGE/$PREFIX/share/labwc/autostart"
install -m 0644 "$REPO/deploy/labwc/rc.xml" "$STAGE/$PREFIX/share/labwc/rc.xml"
install -m 0644 "$REPO/deploy/openauto/openauto.ini" "$STAGE/$PREFIX/share/openauto.ini"
install -m 0644 "$REPO/deploy/udev/51-android-accessory.rules" "$STAGE/lib/udev/rules.d/"

# Docs and licenses
install -m 0644 "$REPO/README.md" "$STAGE/usr/share/doc/$PKG/README.md"
install -m 0644 "$REPO/packaging/copyright" "$STAGE/usr/share/doc/$PKG/copyright"
install -m 0644 "$REPO/patches/README.md" "$STAGE/usr/share/doc/$PKG/PATCHES.md"

# Runtime dependencies: the Debian packages owning each system library the
# shipped ELF files link directly, plus plugins and tools loaded at run time.
owner() {
  local p=$1
  dpkg -S "$p" 2>/dev/null || dpkg -S "/usr$p" 2>/dev/null || dpkg -S "$(realpath "$p")" 2>/dev/null || true
}
NEEDED=$(for f in "$STAGE/$PREFIX/bin"/* "$STAGE/$PREFIX/lib"/*.so*; do
  readelf -d "$f" | sed -n 's/.*Shared library: \[\(.*\)\]/\1/p'
done | sort -u)
DEPS=$(for so in $NEEDED; do
  [ -e "$STAGE/$PREFIX/lib/$so" ] && continue
  path=$(ldconfig -p | awk -v s="$so" '$1 == s {print $NF; exit}')
  [ -n "$path" ] || { echo "warning: cannot resolve $so" >&2; continue; }
  owner "$path"
done | cut -d: -f1 | sort -u)
EXTRA="qml6-module-qtquick qml6-module-qtquick-controls qml6-module-qtquick-layouts
qml6-module-qtquick-templates qml6-module-qtquick-window qml6-module-qtqml
qml6-module-qtqml-workerscript qml6-module-qtqml-models qt6-wayland
libqt5multimedia5-plugins bluez pulseaudio-utils wlr-randr udev"
# shellcheck disable=SC2086  # split the package lists into words
DEPENDS=$(printf '%s\n' $DEPS $EXTRA | sort -u | paste -sd, - | sed 's/,/, /g')

INSTALLED_SIZE=$(du -sk --exclude=DEBIAN "$STAGE" | cut -f1)
cat > "$STAGE/DEBIAN/control" <<EOF
Package: $PKG
Version: $VERSION
Architecture: $ARCH
Maintainer: kagamirudo <hoangvuph03@gmail.com>
Installed-Size: $INSTALLED_SIZE
Depends: $DEPENDS
Section: misc
Priority: optional
Homepage: https://github.com/kagamirudo/pi-carplay
Description: DIY car head unit for Raspberry Pi 5
 Wired Android Auto (patched OpenAuto + aasdk) on a 3.5" 480x320 SPI
 touchscreen, a Qt 6 head-unit shell, and Bluetooth audio to the car stereo.
 Run pi-carplay-setup --boot as the desktop user after installing.
EOF
install -m 0755 "$REPO/packaging/postinst" "$STAGE/DEBIAN/postinst"
(cd "$STAGE" && find . -type f ! -path './DEBIAN/*' -printf '%P\0' | xargs -0 md5sum > DEBIAN/md5sums)

mkdir -p "$OUT_DIR"
dpkg-deb --root-owner-group -Zxz --build "$STAGE" "$OUT_DIR/${PKG}_${VERSION}_${ARCH}.deb" >/dev/null
echo "built $OUT_DIR/${PKG}_${VERSION}_${ARCH}.deb"

# GPL corresponding source: the exact upstream trees with the patches applied.
SRC_NAME=$PKG-$VERSION-gpl-source
mkdir -p "$WORK/$SRC_NAME"
for p in openauto aasdk h264bitstream; do
  mkdir -p "$WORK/$SRC_NAME/$p"
  git -C "$SRC_DIR/$p" ls-files -z | (cd "$SRC_DIR/$p" && xargs -0 cp --parents -t "$WORK/$SRC_NAME/$p")
  git -C "$SRC_DIR/$p" rev-parse HEAD > "$WORK/$SRC_NAME/$p/UPSTREAM_COMMIT"
done
cp -r "$REPO/patches" "$WORK/$SRC_NAME/patches"
cat > "$WORK/$SRC_NAME/README" <<EOF
Corresponding source for the GPL/LGPL components of $PKG $VERSION:
OpenAuto and aasdk (GPL-3.0) and h264bitstream (LGPL-2.1), each at the commit
in its UPSTREAM_COMMIT file with the patches in patches/ applied.
Build instructions: https://github.com/kagamirudo/pi-carplay (make build).
EOF
tar -C "$WORK" -czf "$OUT_DIR/$SRC_NAME.tar.gz" "$SRC_NAME"
echo "built $OUT_DIR/$SRC_NAME.tar.gz"
