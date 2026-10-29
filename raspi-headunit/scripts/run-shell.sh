#!/bin/sh
# Run the shell on the current labwc session. Does not enable anything at boot.
set -eu
ROOT=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
export XDG_RUNTIME_DIR="${XDG_RUNTIME_DIR:-/run/user/$(id -u)}"
export WAYLAND_DISPLAY="${WAYLAND_DISPLAY:-wayland-0}"
export QT_QPA_PLATFORM="${QT_QPA_PLATFORM:-wayland}"
export QT_QUICK_CONTROLS_STYLE=Basic
exec "$ROOT/build/headunit" "$@"
