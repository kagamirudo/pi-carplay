#!/usr/bin/env bats
# pi-carplay-wireless-check and pi-carplay-wireless-setup against a fake sysfs and
# stub system tools, so no Bluetooth adapter, Wi-Fi or NetworkManager is needed.

bats_require_minimum_version 1.5.0

ONBOARD=D8:3A:DD:00:11:22
DONGLE=00:E0:4C:AA:BB:CC

setup() {
  REPO="$BATS_TEST_DIRNAME/../.."
  WORK=$BATS_TEST_TMPDIR
  CHECK="$REPO/tools/pi-carplay-wireless-check"
  SETUP="$REPO/deploy/bin/pi-carplay-wireless-setup"
  mkdir -p "$WORK/bin" "$WORK/sys/bluetooth" "$WORK/net/wlan0" "$WORK/fw/rtl_bt" "$WORK/nm" "$WORK/home"
  export LOG=$WORK/calls.log WORK
  export SYS_BT=$WORK/sys/bluetooth SYS_NET=$WORK/net FIRMWARE_DIR=$WORK/fw NM_CONN_DIR=$WORK/nm
  export HOME=$WORK/home XDG_CONFIG_HOME=$WORK/home/.config
  export WIRELESS_CONF=$WORK/home/.config/pi-carplay/wireless.conf
  export PATH="$WORK/bin:$PATH"
  export ACTIVE="bluetooth NetworkManager" COUNTRY=VN NM_AP=yes FW_INSTALLED=1
  export NM_DEVICES="wlan0:disconnected:" SS_5000='LISTEN 0 4096 0.0.0.0:5000 0.0.0.0:* users:(("autoapp",pid=1,fd=24))'
  : > "$LOG"
  : > "$WORK/klog"
  : > "$WORK/nm-names"
  touch "$WORK/fw/rtl_bt/rtl8761bu_fw.bin" "$WORK/fw/rtl_bt/rtl8761bu_config.bin"
  # btusb quirk table with the UB500 (2357:0604) and not the UB600 (37ad:0600).
  printf 'junk\x03\x00\x57\x23\x04\x06junk' > "$WORK/btusb.ko"
  stubs
  controller hci0 onboard "$ONBOARD"
}

stub() {
  printf '#!/bin/bash\necho "%s $*" >> "$LOG"\n%s\n' "$1" "$2" > "$WORK/bin/$1"
  chmod +x "$WORK/bin/$1"
}

stubs() {
  stub sudo 'exec "$@"'
  stub apt-get 'true'
  stub dnsmasq 'true'
  stub vcgencmd 'echo "timestamp 1746713597"'
  stub bluetoothctl 'echo "bluetoothctl: 5.66"'
  stub journalctl 'cat "$WORK/klog"'
  stub modinfo 'case "$2" in btusb) echo "$WORK/btusb.ko" ;; *) echo "/lib/modules/x/$2.ko" ;; esac'
  stub systemctl 'for s in $ACTIVE; do [ "$s" = "${@: -1}" ] && exit 0; done; exit 3'
  stub hciconfig 'cat "$WORK/addr-$1" 2>/dev/null'
  stub dpkg-query '[ "$FW_INSTALLED" = 1 ] || exit 1
case "$*" in *Status*) echo "install ok installed" ;; *) echo "1:20240709" ;; esac'
  stub ss 'case "$*" in *:5000*) echo "$SS_5000" ;; *:5288*) echo "${SS_5288:-}" ;; esac'
  stub iw 'case "$1" in
list) cat <<EOF
Wiphy phy0
	Supported interface modes:
		 * managed
		 * AP
		 * P2P-client
	Band 1:
		Frequencies:
			* 2412 MHz [1] (20.0 dBm)
	Band 2:
		Frequencies:
			* 5180 MHz [36] (20.0 dBm)
			* 5190 MHz [38] (disabled)
			* 5260 MHz [52] (20.0 dBm) (no IR, radar detection)
			* 5745 MHz [149] (20.0 dBm)
EOF
;;
reg) printf "global\ncountry %s: DFS-FCC\n\nphy#0\ncountry 99: DFS-UNSET\n" "$COUNTRY" ;;
esac'
  stub nmcli 'case "$*" in
*"device show"*) printf "WIFI-PROPERTIES.AP:%s\nWIFI-PROPERTIES.5GHZ:yes\n" "$NM_AP" ;;
*"connection show"*) cat "$WORK/nm-names" ;;
*DEVICE,STATE,CONNECTION*) echo "$NM_DEVICES" ;;
esac'
}

# controller <hci> onboard|<vid:pid> <address>
controller() {
  local dev
  if [ "$2" = onboard ]; then
    dev=$WORK/devices/serial/$1
  else
    dev=$WORK/devices/usb/$1/1-1:1.0
    mkdir -p "$dev"
    echo "${2%:*}" > "$WORK/devices/usb/$1/idVendor"
    echo "${2#*:}" > "$WORK/devices/usb/$1/idProduct"
  fi
  mkdir -p "$dev" "$SYS_BT/$1"
  ln -s "$dev" "$SYS_BT/$1/device"
  echo "	BD Address: $3  ACL MTU: 1021:8  SCO MTU: 64:1" > "$WORK/addr-$1"
}

configure() {
  mkdir -p "$(dirname "$WIRELESS_CONF")"
  printf 'ADAPTER=%s\nCHANNEL=%s\nCONNECTION=pi-carplay-aa\n' "$1" "${2:-36}" > "$WIRELESS_CONF"
}

# ---------------------------------------------------------------- check

@test "check passes with a UB500 whose firmware loaded" {
  controller hci1 2357:0604 "$DONGLE"
  configure "$DONGLE"
  echo "pi-carplay-aa" > "$WORK/nm-names"
  echo "Bluetooth: hci1: RTL: fw version 0xdfc6d922" > "$WORK/klog"
  run "$CHECK"
  [ "$status" -eq 0 ]
  [[ "$output" == *"PASS  hci0 XX:XX:XX:XX:11:22: onboard (UART)"* ]]
  [[ "$output" == *"PASS  hci1 XX:XX:XX:XX:BB:CC: USB 2357:0604 TP-Link UB500"*"firmware loaded (0xdfc6d922)"* ]]
  [[ "$output" == *"PASS  phone adapter XX:XX:XX:XX:BB:CC is present"* ]]
  [[ "$output" == *"PASS  5 GHz channels without radar detection: 36 149"* ]]
  [[ "$output" == *"PASS  hotspot channel 36 is usable here"* ]]
  [[ "$output" == *"PASS  autoapp listens for wireless sessions on TCP 5000"* ]]
  [[ "$output" == *"0 failed"* ]]
}

@test "check hides addresses unless asked" {
  run "$CHECK" --no-fail
  [[ "$output" != *"$ONBOARD"* ]]
  run "$CHECK" --no-fail --show-addresses
  [[ "$output" == *"hci0 $ONBOARD: onboard"* ]]
}

@test "check fails when the Realtek firmware download failed, unless --no-fail" {
  controller hci1 2357:0604 "$DONGLE"
  printf 'Bluetooth: hci1: RTL: fw version 0x1\nBluetooth: hci1: RTL: download fw command failed (-110)\n' > "$WORK/klog"
  run "$CHECK"
  [ "$status" -eq 1 ]
  [[ "$output" == *"FAIL  hci1"*"firmware download failed"* ]]
  run "$CHECK" --no-fail
  [ "$status" -eq 0 ]
}

@test "check fails when firmware files are missing" {
  controller hci1 2357:0604 "$DONGLE"
  echo "Bluetooth: hci1: RTL: firmware file rtl_bt/rtl8761bu_fw.bin not found" > "$WORK/klog"
  rm "$WORK/fw/rtl_bt/rtl8761bu_fw.bin"
  run "$CHECK"
  [ "$status" -eq 1 ]
  [[ "$output" == *"FAIL  TP-Link UB500"*"missing"* ]]
  [[ "$output" == *"FAIL  hci1"*"firmware file missing"* ]]
}

@test "check without a dongle only warns" {
  run "$CHECK"
  [ "$status" -eq 0 ]
  [[ "$output" == *"WARN  no USB Bluetooth adapter plugged in"* ]]
  [[ "$output" == *"WARN  no phone adapter chosen yet"* ]]
}

@test "check tells which TP-Link adapters this kernel drives" {
  run "$CHECK"
  [[ "$output" == *"PASS  TP-Link UB500 / UB500 Plus / UB5A (2357:0604"*"btusb knows it, firmware present"* ]]
  [[ "$output" == *"PASS  TP-Link UB400"*"no firmware needed"* ]]
  [[ "$output" == *"WARN  TP-Link UB600 (37ad:0600"*"do not buy"* ]]
}

@test "check names an untested USB adapter and the CSR UB400" {
  controller hci1 1234:5678 "$DONGLE"
  controller hci2 0a12:0001 00:1A:7D:DA:71:10
  run "$CHECK"
  [[ "$output" == *"WARN  hci1"*"USB 1234:5678, not a tested adapter"* ]]
  [[ "$output" == *"PASS  hci2"*"USB 0a12:0001 TP-Link UB400"* ]]
}

@test "check warns when the dongle took hci0 from the onboard controller" {
  rm -rf "$SYS_BT"/hci0
  controller hci0 2357:0604 "$DONGLE"
  controller hci1 onboard "$ONBOARD"
  run "$CHECK"
  [[ "$output" == *"WARN  hci0 is the USB adapter"* ]]
}

@test "check fails when the chosen adapter is unplugged" {
  configure "$DONGLE"
  run "$CHECK"
  [ "$status" -eq 1 ]
  [[ "$output" == *"FAIL  phone adapter XX:XX:XX:XX:BB:CC from"*"is not plugged in"* ]]
}

@test "check rejects a radar (DFS) hotspot channel" {
  configure "" 52
  run "$CHECK"
  [ "$status" -eq 1 ]
  [[ "$output" == *"FAIL  hotspot channel 52 is not usable here (pick one of: 36 149)"* ]]
}

@test "check fails without a regulatory country" {
  COUNTRY=00 run "$CHECK"
  [ "$status" -eq 1 ]
  [[ "$output" == *"FAIL  regulatory country not set"* ]]
}

@test "check fails without NetworkManager or Bluetooth" {
  ACTIVE="" run "$CHECK"
  [ "$status" -eq 1 ]
  [[ "$output" == *"FAIL  bluetooth.service is not running"* ]]
  [[ "$output" == *"FAIL  NetworkManager is not running"* ]]
}

@test "check warns that starting the hotspot drops a Wi-Fi client connection" {
  NM_DEVICES="wlan0:connected:Home" run "$CHECK"
  [[ "$output" == *"WARN  wlan0 is a Wi-Fi client right now"* ]]
}

@test "check fails when another program holds the Android Auto port" {
  SS_5000='LISTEN 0 5 0.0.0.0:5000 0.0.0.0:* users:(("python3",pid=9,fd=3))' run "$CHECK"
  [ "$status" -eq 1 ]
  [[ "$output" == *'FAIL  TCP 5000 is taken by another program: users:(("python3"'* ]]
  SS_5000="" SS_5288="LISTEN 0 5 0.0.0.0:5288" run "$CHECK"
  [[ "$output" == *"WARN  nothing listens on TCP 5000"* ]]
  [[ "$output" == *"WARN  TCP 5288"*"is taken"* ]]
}

@test "check is read-only" {
  run "$CHECK" --no-fail
  run ! grep -Eq '^(sudo|apt-get)|connection (up|add|load|modify|delete)' "$LOG"
  [ ! -e "$WIRELESS_CONF" ]
}

@test "check rejects unknown options" {
  run "$CHECK" --bogus
  [ "$status" -eq 2 ]
}

# ---------------------------------------------------------------- setup

@test "setup --dry-run changes nothing" {
  [ "$(id -u)" -ne 0 ] || skip "refuses to run as root by design"
  FW_INSTALLED=0 run "$SETUP" --dry-run
  [ "$status" -eq 0 ]
  [[ "$output" == *"would run: sudo apt-get install -y firmware-realtek"* ]]
  [[ "$output" == *"psk=********"* ]]
  [[ "$output" == *"dry run: nothing was changed"* ]]
  [ ! -e "$WIRELESS_CONF" ]
  [ -z "$(ls -A "$WORK/nm")" ]
  run ! grep -Eq '^apt-get|connection load' "$LOG"
}

@test "setup writes a hotspot profile that never starts by itself" {
  [ "$(id -u)" -ne 0 ] || skip "refuses to run as root by design"
  controller hci1 2357:0604 "$DONGLE"
  run "$SETUP"
  [ "$status" -eq 0 ]
  local key=$WORK/nm/pi-carplay-aa.nmconnection
  [ "$(stat -c %a "$key")" = 600 ]
  grep -qx 'autoconnect=false' "$key"
  grep -qx 'mode=ap' "$key"
  grep -qx 'band=a' "$key"
  grep -qx 'channel=36' "$key"
  grep -qx 'method=shared' "$key"
  grep -qx 'address1=10.42.0.1/24' "$key"
  grep -Eqx 'psk=[A-Za-z0-9]{20}' "$key"
  grep -qx "ADAPTER=$DONGLE" "$WIRELESS_CONF"
  run ! grep -q psk "$WIRELESS_CONF"
  grep -q "nmcli connection load $key" "$LOG"
  run ! grep -Eq 'connection (up|activate)|apt-get' "$LOG"
}

@test "setup run twice keeps the password and UUID" {
  [ "$(id -u)" -ne 0 ] || skip "refuses to run as root by design"
  "$SETUP" --adapter "$DONGLE" > /dev/null
  cp "$WORK/nm/pi-carplay-aa.nmconnection" "$WORK/first"
  : > "$LOG"
  run "$SETUP"
  [ "$status" -eq 0 ]
  [[ "$output" == *"wireless.conf unchanged"* ]]
  [[ "$output" == *"hotspot profile pi-carplay-aa unchanged"* ]]
  cmp "$WORK/first" "$WORK/nm/pi-carplay-aa.nmconnection"
  run ! grep -q 'connection load' "$LOG"

  run "$SETUP" --channel 149 --ssid "My Car"
  [ "$status" -eq 0 ]
  grep -qx 'channel=149' "$WORK/nm/pi-carplay-aa.nmconnection"
  grep -qx 'ssid=My Car' "$WORK/nm/pi-carplay-aa.nmconnection"
  [ "$(grep '^psk=' "$WORK/first")" = "$(grep '^psk=' "$WORK/nm/pi-carplay-aa.nmconnection")" ]
  [ "$(grep '^uuid=' "$WORK/first")" = "$(grep '^uuid=' "$WORK/nm/pi-carplay-aa.nmconnection")" ]
  grep -qx "ADAPTER=$DONGLE" "$WIRELESS_CONF"
}

@test "setup installs firmware-realtek only when missing" {
  [ "$(id -u)" -ne 0 ] || skip "refuses to run as root by design"
  FW_INSTALLED=0 run "$SETUP"
  grep -q 'apt-get install -y firmware-realtek' "$LOG"
  : > "$LOG"
  FW_INSTALLED=0 run "$SETUP" --no-firmware
  run ! grep -q apt-get "$LOG"
}

@test "setup rejects bad input before changing anything" {
  [ "$(id -u)" -ne 0 ] || skip "refuses to run as root by design"
  run "$SETUP" --channel 52
  [ "$status" -eq 1 ]
  [[ "$output" == *"--channel must be one of"* ]]
  run "$SETUP" --adapter 'AA:BB;reboot'
  [ "$status" -eq 1 ]
  run "$SETUP" --ssid "$(printf 'x%.0s' {1..33})"
  [ "$status" -eq 1 ]
  run "$SETUP" --ssid 'a=b'
  [ "$status" -eq 1 ]
  run "$SETUP" --ssid 'Car\name'
  [ "$status" -eq 1 ]
  run "$SETUP" --ssid ' Car'
  [ "$status" -eq 1 ]
  run "$SETUP" --bogus
  [ "$status" -eq 1 ]
  [ ! -e "$WIRELESS_CONF" ]
  [ -z "$(ls -A "$WORK/nm")" ]
}

@test "setup refuses a Wi-Fi chip without 5 GHz access point support" {
  [ "$(id -u)" -ne 0 ] || skip "refuses to run as root by design"
  NM_AP=no run "$SETUP"
  [ "$status" -eq 1 ]
  [[ "$output" == *"no 5 GHz access point support"* ]]
  [ -z "$(ls -A "$WORK/nm")" ]
}

@test "setup --remove deletes the profile and settings" {
  [ "$(id -u)" -ne 0 ] || skip "refuses to run as root by design"
  "$SETUP" > /dev/null
  run "$SETUP" --remove
  [ "$status" -eq 0 ]
  [ ! -e "$WORK/nm/pi-carplay-aa.nmconnection" ]
  [ ! -e "$WIRELESS_CONF" ]
  grep -q 'nmcli connection reload' "$LOG"
}
