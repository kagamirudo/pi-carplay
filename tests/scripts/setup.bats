#!/usr/bin/env bats
# Boot config, per-user setup, the autoapp launcher and `make install-*`.

setup() {
  REPO="$BATS_TEST_DIRNAME/../.."
  WORK=$BATS_TEST_TMPDIR
}

# ---------------------------------------------------------------- boot config

@test "boot-config appends the overlay once, with a backup" {
  printf '[all]\ndtparam=audio=on\n' > "$WORK/config.txt"
  BOOT_CFG="$WORK/config.txt" run "$REPO/deploy/bin/pi-carplay-boot-config"
  [ "$status" -eq 0 ]
  grep -qx 'dtoverlay=piscreen,drm,speed=32000000,rotate=180,invx,invy' "$WORK/config.txt"
  cmp -s "$WORK/config.txt.bak" <(printf '[all]\ndtparam=audio=on\n')

  BOOT_CFG="$WORK/config.txt" run "$REPO/deploy/bin/pi-carplay-boot-config"
  [ "$status" -eq 0 ]
  [[ "$output" == *"already has"* ]]
  [ "$(grep -c '^dtoverlay=piscreen' "$WORK/config.txt")" -eq 1 ]
}

@test "boot-config leaves a hand-tuned overlay line alone" {
  printf 'dtoverlay=piscreen,drm,speed=24000000\n' > "$WORK/config.txt"
  BOOT_CFG="$WORK/config.txt" run "$REPO/deploy/bin/pi-carplay-boot-config"
  [ "$status" -eq 0 ]
  grep -qx 'dtoverlay=piscreen,drm,speed=24000000' "$WORK/config.txt"
  [ "$(wc -l < "$WORK/config.txt")" -eq 1 ]
}

@test "boot-config refuses when there is no config.txt" {
  BOOT_CFG="$WORK/missing.txt" run "$REPO/deploy/bin/pi-carplay-boot-config"
  [ "$status" -eq 1 ]
  [[ "$output" == *"not found"* ]]
}

# ---------------------------------------------------------------- pi-carplay-setup

setup_share() {
  mkdir -p "$WORK/share/labwc"
  echo "rc" > "$WORK/share/labwc/rc.xml"
  echo "autostart" > "$WORK/share/labwc/autostart"
}

@test "setup installs labwc files and backs up different ones" {
  [ "$(id -u)" -ne 0 ] || skip "refuses to run as root by design"
  setup_share
  mkdir -p "$WORK/home/.config/labwc"
  echo "mine" > "$WORK/home/.config/labwc/autostart"
  HOME="$WORK/home" XDG_CONFIG_HOME= PI_CARPLAY_SHARE="$WORK/share" run "$REPO/packaging/pi-carplay-setup"
  [ "$status" -eq 0 ]
  [ "$(cat "$WORK/home/.config/labwc/autostart")" = autostart ]
  [ "$(cat "$WORK/home/.config/labwc/autostart.bak")" = mine ]
  [ "$(cat "$WORK/home/.config/labwc/rc.xml")" = rc ]
  [ ! -e "$WORK/home/.config/labwc/rc.xml.bak" ]
}

@test "setup run twice keeps the user's original backup" {
  [ "$(id -u)" -ne 0 ] || skip "refuses to run as root by design"
  setup_share
  mkdir -p "$WORK/home/.config/labwc"
  echo "mine" > "$WORK/home/.config/labwc/autostart"
  HOME="$WORK/home" XDG_CONFIG_HOME= PI_CARPLAY_SHARE="$WORK/share" run "$REPO/packaging/pi-carplay-setup"
  HOME="$WORK/home" XDG_CONFIG_HOME= PI_CARPLAY_SHARE="$WORK/share" run "$REPO/packaging/pi-carplay-setup"
  [ "$(cat "$WORK/home/.config/labwc/autostart.bak")" = mine ]
}

@test "setup renders the dashcam unit for the desktop user" {
  [ "$(id -u)" -ne 0 ] || skip "refuses to run as root by design"
  setup_share
  printf 'User=@USER@\nXDG=/run/user/@UID@\nExec=@BIN_DIR@/pi-carplay-dashcam\n' \
    > "$WORK/dashcam.service"
  HOME="$WORK/home" PI_CARPLAY_SHARE="$WORK/share" PI_CARPLAY_UNIT="$WORK/dashcam.service" \
    run "$REPO/packaging/pi-carplay-setup"
  [ "$status" -eq 0 ]
  rendered="$WORK/home/.local/share/pi-carplay/pi-carplay-dashcam.service"
  grep -qx "User=$(id -un)" "$rendered"
  grep -qx "XDG=/run/user/$(id -u)" "$rendered"
  grep -qx "Exec=/usr/bin/pi-carplay-dashcam" "$rendered"
  ! grep -q '@' "$rendered"
}

@test "setup --boot runs the boot config step" {
  [ "$(id -u)" -ne 0 ] || skip "refuses to run as root by design"
  setup_share
  printf '#!/bin/sh\ntouch "%s/boot-ran"\n' "$WORK" > "$WORK/boot"
  chmod +x "$WORK/boot"
  HOME="$WORK/home" PI_CARPLAY_SHARE="$WORK/share" PI_CARPLAY_BOOT_CONFIG="$WORK/boot" \
    run "$REPO/packaging/pi-carplay-setup" --boot
  [ "$status" -eq 0 ]
  [ -e "$WORK/boot-ran" ]
}

# ---------------------------------------------------------------- autoapp launcher

render_launcher() {
  printf '#!/bin/sh\npwd > "%s/cwd"\nenv > "%s/env"\necho "$@" > "%s/args"\n' "$WORK" "$WORK" "$WORK" > "$WORK/fake-autoapp"
  chmod +x "$WORK/fake-autoapp"
  echo "defaults" > "$WORK/default.ini"
  sed -e "s|@OPENAUTO_DIR@|$WORK/conf|" -e "s|@DEFAULT_INI@|$WORK/default.ini|" \
      -e "s|@AUTOAPP@|$WORK/fake-autoapp|" "$REPO/deploy/bin/pi-carplay-autoapp" > "$WORK/launcher"
  chmod +x "$WORK/launcher"
}

@test "launcher seeds openauto.ini and runs autoapp from its config directory" {
  render_launcher
  run "$WORK/launcher" --flag
  [ "$status" -eq 0 ]
  [ "$(cat "$WORK/cwd")" = "$WORK/conf" ]
  [ "$(cat "$WORK/conf/openauto.ini")" = defaults ]
  [ "$(cat "$WORK/args")" = --flag ]
  grep -qx OPENAUTO_NO_MAIN_WINDOW=1 "$WORK/env"
}

@test "launcher keeps an existing openauto.ini" {
  render_launcher
  mkdir -p "$WORK/conf"
  echo "tuned" > "$WORK/conf/openauto.ini"
  run "$WORK/launcher"
  [ "$(cat "$WORK/conf/openauto.ini")" = tuned ]
}

# ---------------------------------------------------------------- make install-*

@test "make install-scripts and install-labwc leave no unrendered placeholders" {
  run make -C "$REPO" install-scripts install-labwc BIN_DIR="$WORK/bin" LABWC_DIR="$WORK/labwc"
  [ "$status" -eq 0 ]
  for f in "$WORK"/bin/* "$WORK"/labwc/*; do
    ! grep -n '@[A-Z_]*@' "$f"
  done
  [ -x "$WORK/bin/pi-carplay-supervise" ]
  [ -x "$WORK/bin/aa-restart.sh" ]
  grep -q "$WORK/bin/pi-carplay-supervise headunit" "$WORK/labwc/autostart"
}
