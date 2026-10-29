#!/bin/bash
# Restart the head-unit panel and Android Auto on the running labwc session,
# then re-plug the phone over USB so a fresh session starts.
#
# Run as the desktop user. sudo is used only to reset the USB port.
set -u

HEADUNIT=@HEADUNIT@
AUTOAPP_LAUNCHER=@AUTOAPP_LAUNCHER@
SUPERVISE=@SUPERVISE@

export XDG_RUNTIME_DIR="${XDG_RUNTIME_DIR:-/run/user/$(id -u)}"
export WAYLAND_DISPLAY="${WAYLAND_DISPLAY:-wayland-0}"
export QT_QPA_PLATFORM=wayland
export QT_WAYLAND_DISABLE_WINDOWDECORATION=1

"$SUPERVISE" --stop autoapp
"$SUPERVISE" --stop headunit
# Copies started by hand or by an older autostart.
pkill -u "$(id -u)" -x autoapp
pkill -u "$(id -u)" -x headunit
sleep 0.5

setsid -f "$SUPERVISE" headunit "$HEADUNIT" < /dev/null > /dev/null 2>&1
sleep 3
setsid -f "$SUPERVISE" autoapp "$AUTOAPP_LAUNCHER" < /dev/null > /dev/null 2>&1
sleep 2

# Google accessory (18d1) and Samsung (04e8) phones: drop and re-authorize the port.
for d in /sys/bus/usb/devices/*; do
  [ -f "$d/idVendor" ] || continue
  v=$(cat "$d/idVendor")
  if [ "$v" = 18d1 ] || [ "$v" = 04e8 ]; then
    echo 0 | sudo tee "$d/authorized" >/dev/null
    sleep 2
    echo 1 | sudo tee "$d/authorized" >/dev/null
    echo "reset $(basename "$d") $v:$(cat "$d/idProduct")"
  fi
done
