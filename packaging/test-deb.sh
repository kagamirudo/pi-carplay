#!/bin/bash
# Check a built pi-carplay .deb before it is released.
#
#   packaging/test-deb.sh <deb>             static checks and a smoke start from the extracted tree
#   packaging/test-deb.sh --install <deb>   also install, exercise and purge it (throwaway system only:
#                                           CI containers; it changes /opt, /usr/bin and udev rules)
set -euo pipefail

INSTALL=0
if [ "${1:-}" = --install ]; then
  INSTALL=1
  shift
fi
DEB=$(realpath "${1:?usage: test-deb.sh [--install] <deb>}")
if [ "$INSTALL" -eq 1 ]; then
  [ "$(id -u)" -eq 0 ] || { echo "--install needs root (run it in a throwaway container)" >&2; exit 2; }
  export DEBIAN_FRONTEND=noninteractive
  # The static checks below need binutils, lintian and the runtime libraries.
  apt-get install -y --no-install-recommends binutils lintian "$DEB" > /dev/null
  apt-get purge -y pi-carplay > /dev/null
fi
WORK=$(mktemp -d)
trap 'rm -rf "$WORK"' EXIT
failures=0

ok() { echo "ok   $*"; }
fail() { echo "FAIL $*"; failures=$((failures + 1)); }
check() {
  local what=$1
  shift
  if "$@" > "$WORK/out" 2>&1; then ok "$what"; else fail "$what"; sed 's/^/     /' "$WORK/out" | tail -n 20; fi
}

# ---------------------------------------------------------------- metadata

check "control fields" bash -c "
  f() { dpkg-deb -f '$DEB' \"\$1\"; }
  [ \"\$(f Package)\" = pi-carplay ] && [ -n \"\$(f Version)\" ] && [ -n \"\$(f Depends)\" ] && [ -n \"\$(f Maintainer)\" ]"

dpkg-deb -x "$DEB" "$WORK/root"
dpkg-deb -e "$DEB" "$WORK/DEBIAN"
R=$WORK/root

REQUIRED="opt/pi-carplay/bin/headunit opt/pi-carplay/bin/autoapp opt/pi-carplay/share/openauto.ini
opt/pi-carplay/share/labwc/autostart opt/pi-carplay/share/labwc/rc.xml lib/udev/rules.d/51-android-accessory.rules
usr/bin/pi-carplay-setup usr/bin/pi-carplay-autoapp usr/bin/pi-carplay-restart usr/bin/pi-carplay-boot-config
usr/bin/pi-carplay-supervise usr/bin/pi-carplay-selftest usr/bin/pi-carplay-wireless-check
usr/bin/pi-carplay-wireless-setup usr/bin/pi-carplay-update
usr/bin/pi-carplay-dashcam usr/bin/pi-carplay-align-display usr/bin/pi-carplay-display-profile \
usr/bin/pi-carplay-hotspot \
usr/share/pi-carplay/elecrow-1024x600.bin usr/share/pi-carplay/pi-carplay-dashcam.service \
usr/share/pi-carplay/profiles/dell-hdmi.env usr/share/pi-carplay/profiles/elecrow-7.env \
usr/share/doc/pi-carplay/copyright"
missing=""
for f in $REQUIRED; do [ -e "$R/$f" ] || missing="$missing $f"; done
if [ -z "$missing" ]; then ok "required files"; else fail "missing:$missing"; fi

check "md5sums match" bash -c "cd '$R' && md5sum --quiet -c '$WORK/DEBIAN/md5sums'"

check "no unrendered @PLACEHOLDER@" bash -c "! grep -rnI '@[A-Z_]\\+@' '$R/usr/bin' '$R/opt/pi-carplay/share'"

check "scripts parse" bash -c "
  for f in '$R'/usr/bin/* '$WORK/DEBIAN/postinst'; do
    case \"\$(head -n1 \"\$f\")\" in
      '#!/bin/sh') sh -n \"\$f\" ;;
      '#!/bin/bash') bash -n \"\$f\" ;;
    esac || { echo \"\$f\"; exit 1; }
  done"

check "executables are executable" bash -c "
  for f in '$R'/usr/bin/* '$R'/opt/pi-carplay/bin/*; do [ -x \"\$f\" ] || { echo \"\$f\"; exit 1; }; done"

check "ELF files are stripped and use the bundled lib dir" bash -c "
  for f in '$R'/opt/pi-carplay/bin/autoapp '$R'/opt/pi-carplay/lib/*.so*; do
    readelf -S \"\$f\" | grep -q '\\.debug_info' && { echo \"not stripped: \$f\"; exit 1; }
    readelf -d \"\$f\" | grep -q 'R.*PATH.*\\\$ORIGIN/../lib' || { echo \"no \\\$ORIGIN rpath: \$f\"; exit 1; }
  done; true"

check "autoapp resolves every library" bash -c "! ldd '$R/opt/pi-carplay/bin/autoapp' | grep 'not found'"
check "headunit resolves every library" bash -c "! ldd '$R/opt/pi-carplay/bin/headunit' | grep 'not found'"

if command -v lintian > /dev/null; then
  # Tags that follow from shipping prebuilt, bundled binaries under /opt
  # rather than a Debian source package.
  check "lintian (no errors)" lintian --fail-on error --tag-display-limit 0 \
    --suppress-tags appstream-metadata-missing-modalias-provide,no-changelog,no-manual-page,dir-or-file-in-opt,custom-library-search-path,embedded-library,shared-library-lacks-prerequisites,package-installs-into-obsolete-dir,unstripped-binary-or-object,binary-without-manpage,description-starts-with-package-name,maintainer-script-calls-systemctl \
    "$DEB"
else
  echo "skip lintian (not installed)"
fi

check "headunit starts from the package tree" env QT_QPA_PLATFORM=offscreen HEADUNIT_NO_BLUETOOTH=1 \
  HOME="$WORK/home" XDG_CONFIG_HOME= timeout 30 "$R/opt/pi-carplay/bin/headunit" --smoke

# ---------------------------------------------------------------- install / purge

if [ "$INSTALL" -eq 1 ]; then
  check "apt installs the package and its dependencies" apt-get install -y --no-install-recommends "$DEB"
  check "dpkg reports it installed" bash -c "dpkg-query -W -f='\${Status}' pi-carplay | grep -q 'install ok installed'"
  check "installed headunit starts" env QT_QPA_PLATFORM=offscreen HEADUNIT_NO_BLUETOOTH=1 \
    HOME="$WORK/home2" timeout 30 /opt/pi-carplay/bin/headunit --smoke
  check "installed autoapp links" bash -c "! ldd /opt/pi-carplay/bin/autoapp | grep 'not found'"

  id tester > /dev/null 2>&1 || useradd -m tester
  check "pi-carplay-setup works for a desktop user" su tester -c "pi-carplay-setup && test -s ~/.config/labwc/autostart \
    && grep -q '/usr/bin/pi-carplay-supervise headunit /opt/pi-carplay/bin/headunit' ~/.config/labwc/autostart"
  check "supervisor runs for a desktop user" su tester -c "pi-carplay-supervise --status"
  check "selftest runs (results may fail off-Pi)" bash -c "pi-carplay-selftest --no-fail > /dev/null"
  check "wireless check runs (results may fail off-Pi)" bash -c "pi-carplay-wireless-check --no-fail > /dev/null"
  check "wireless setup previews for a desktop user" su tester -c "pi-carplay-wireless-setup --dry-run --no-firmware 2>&1 \
    | grep -qE 'dry run: nothing was changed|no 5 GHz access point support'"
  check "update check runs and reports offline" bash -c "
    PI_CARPLAY_UPDATE_URL=http://127.0.0.1:1 pi-carplay-update check --json; [ \$? -eq 3 ]"

  check "apt purges the package" apt-get purge -y pi-carplay
  check "purge leaves nothing behind" bash -c "
    [ ! -e /opt/pi-carplay ] && [ ! -e /usr/bin/pi-carplay-setup ] && [ ! -e /lib/udev/rules.d/51-android-accessory.rules ]"
  check "reinstall after purge" apt-get install -y --no-install-recommends "$DEB"
fi

if [ "$failures" -ne 0 ]; then
  echo "$failures package check(s) failed"
  exit 1
fi
echo "package checks passed: $(basename "$DEB")"
