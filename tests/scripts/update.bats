#!/usr/bin/env bats
# pi-carplay-update check, against a local fake of the GitHub releases API.

setup_file() {
  export FAKE_ROOT=$BATS_FILE_TMPDIR/api
  mkdir -p "$FAKE_ROOT"
  python3 "$BATS_TEST_DIRNAME/fake_github.py" "$FAKE_ROOT" "$BATS_FILE_TMPDIR/port" 3>&- &
  echo "$!" > "$BATS_FILE_TMPDIR/pid"
  for _ in $(seq 1 50); do
    [ -s "$BATS_FILE_TMPDIR/port" ] && break
    sleep 0.1
  done
  [ -s "$BATS_FILE_TMPDIR/port" ]
}

teardown_file() {
  kill "$(cat "$BATS_FILE_TMPDIR/pid")" 2>/dev/null || true
}

setup() {
  UPDATE="$BATS_TEST_DIRNAME/../../tools/pi-carplay-update"
  API=$FAKE_ROOT/t$BATS_TEST_NUMBER
  mkdir -p "$API/releases"
  export PI_CARPLAY_UPDATE_URL="http://127.0.0.1:$(cat "$BATS_FILE_TMPDIR/port")/t$BATS_TEST_NUMBER"
  export PI_CARPLAY_ARCH=arm64 PI_CARPLAY_INSTALLED_VERSION=0.1.0
  unset PI_CARPLAY_UPDATE_CHANNEL PI_CARPLAY_GITHUB_TOKEN
}

# release VERSION [prerelease] [draft]: one release object with a .deb and the GPL source.
release() {
  local v=$1 pre=${2:-false} draft=${3:-false} dl=https://example.invalid/download/v$1
  cat <<EOF
{"tag_name": "v$v", "draft": $draft, "prerelease": $pre, "published_at": "2026-10-29T06:46:59Z",
 "html_url": "https://example.invalid/releases/tag/v$v",
 "assets": [
  {"name": "pi-carplay-$v-gpl-source.tar.gz", "size": 562249, "browser_download_url": "$dl/src.tar.gz"},
  {"name": "pi-carplay_${v}_arm64.deb", "size": 774740,
   "digest": "sha256:55fe2f4495ea43320d429383cb568fe08576a3851ee604493446f6035e06a18d",
   "browser_download_url": "$dl/pi-carplay_${v}_arm64.deb"}]}
EOF
}

latest() { release "$@" > "$API/releases/latest.json"; }

json_field() { python3 -c 'import json,sys; d=json.load(sys.stdin); print(eval("d" + sys.argv[1]))' "$1"; }

# ---------------------------------------------------------------- versions

@test "a newer release is an update (exit 10)" {
  latest 0.2.0
  run "$UPDATE" check
  [ "$status" -eq 10 ]
  [[ "$output" == *"installed: 0.1.0"* ]]
  [[ "$output" == *"latest:    0.2.0 (stable"* ]]
  [[ "$output" == *"pi-carplay_0.2.0_arm64.deb, 756 KiB, sha256:55fe2f44"* ]]
  [[ "$output" == *"status:    update-available"* ]]
}

@test "the same version is up to date (exit 0)" {
  latest 0.1.0
  run "$UPDATE" check
  [ "$status" -eq 0 ]
  [[ "$output" == *"status:    up-to-date"* ]]
}

@test "a CI build newer than the release is up to date" {
  latest 0.1.0
  PI_CARPLAY_INSTALLED_VERSION=0.2.0~git1a2b3c4 run "$UPDATE" check
  [ "$status" -eq 0 ]
  [[ "$output" == *"newer than the latest release"* ]]
}

@test "a release candidate sorts below its release" {
  latest 0.2.0
  PI_CARPLAY_INSTALLED_VERSION=0.2.0~rc1 run "$UPDATE" check
  [ "$status" -eq 10 ]
}

@test "a CI build of the released version sorts below it" {
  latest 0.2.0
  PI_CARPLAY_INSTALLED_VERSION=0.2.0~git1a2b3c4 run "$UPDATE" check
  [ "$status" -eq 10 ]
}

@test "no installed package is reported, not treated as an update (exit 5)" {
  latest 0.2.0
  PI_CARPLAY_INSTALLED_VERSION= run "$UPDATE" check
  [ "$status" -eq 5 ]
  [[ "$output" == *"not installed as a package"* ]]
}

# ---------------------------------------------------------------- output

@test "--json gives a machine-readable result" {
  latest 0.2.0
  run "$UPDATE" check --json
  [ "$status" -eq 10 ]
  [ "$(json_field '["status"]' <<< "$output")" = update-available ]
  [ "$(json_field '["installed"]' <<< "$output")" = 0.1.0 ]
  [ "$(json_field '["latest"]' <<< "$output")" = 0.2.0 ]
  [ "$(json_field '["asset"]["size"]' <<< "$output")" = 774740 ]
  [ "$(json_field '["asset"]["sha256"]' <<< "$output")" = 55fe2f4495ea43320d429383cb568fe08576a3851ee604493446f6035e06a18d ]
  [ "$(json_field '["asset"]["url"]' <<< "$output")" = https://example.invalid/download/v0.2.0/pi-carplay_0.2.0_arm64.deb ]
}

@test "--json reports errors as JSON too" {
  run "$UPDATE" check --json
  [ "$status" -eq 4 ]
  [ "$(json_field '["status"]' <<< "$output")" = error ]
  [[ "$(json_field '["error"]' <<< "$output")" == *"HTTP 404"* ]]
}

@test "release text cannot break the line-based parsing" {
  release 0.2.0 | sed 's|"html_url": "|"html_url": "a\\tb\\nc|' > "$API/releases/latest.json"
  run "$UPDATE" check --json
  [ "$status" -eq 10 ]
  [ "$(json_field '["latest"]' <<< "$output")" = 0.2.0 ]
}

# ---------------------------------------------------------------- channels

@test "beta takes the newest non-draft release, prereleases included" {
  { echo '['; release 0.2.0; echo ,; release 0.4.0 false true; echo ,; release 0.3.0~rc1 true; echo ']'; } \
    > "$API/releases.json"
  run "$UPDATE" check --channel beta --json
  [ "$status" -eq 10 ]
  [ "$(json_field '["latest"]' <<< "$output")" = 0.3.0~rc1 ]
  [ "$(json_field '["prerelease"]' <<< "$output")" = True ]
}

@test "the channel can come from the environment" {
  { echo '['; release 0.1.0; echo ']'; } > "$API/releases.json"
  PI_CARPLAY_UPDATE_CHANNEL=beta run "$UPDATE" check
  [ "$status" -eq 0 ]
  [[ "$output" == *"(beta"* ]]
}

@test "stable ignores a prerelease even if the server returns one" {
  latest 0.3.0~rc1 true
  run "$UPDATE" check
  [ "$status" -eq 4 ]
  [[ "$output" == *"no stable release has a pi-carplay_*_arm64.deb"* ]]
}

@test "a release without a package for this architecture is an error" {
  latest 0.2.0
  PI_CARPLAY_ARCH=armhf run "$UPDATE" check
  [ "$status" -eq 4 ]
  [[ "$output" == *"pi-carplay_*_armhf.deb"* ]]
}

# ---------------------------------------------------------------- failures

@test "no release yet is an error (exit 4)" {
  run "$UPDATE" check
  [ "$status" -eq 4 ]
  [[ "$output" == *"no stable release published yet"* ]]
}

@test "the API rate limit is explained" {
  latest 0.2.0
  echo 403 > "$API/releases/latest.status"
  printf 'X-RateLimit-Remaining: 0\nX-RateLimit-Reset: 1793264165\n' > "$API/releases/latest.headers"
  run "$UPDATE" check
  [ "$status" -eq 4 ]
  [[ "$output" == *"rate limit reached"* ]]
}

@test "a server error is an error (exit 4)" {
  latest 0.2.0
  echo 502 > "$API/releases/latest.status"
  run "$UPDATE" check
  [ "$status" -eq 4 ]
  [[ "$output" == *"HTTP 502"* ]]
}

@test "a body that is not JSON is an error (exit 4)" {
  echo '<html>captive portal</html>' > "$API/releases/latest.json"
  run "$UPDATE" check
  [ "$status" -eq 4 ]
  [[ "$output" == *"cannot parse the release list"* ]]
}

@test "offline is exit 3" {
  PI_CARPLAY_UPDATE_URL=http://127.0.0.1:1 run "$UPDATE" check
  [ "$status" -eq 3 ]
  [[ "$output" == *"offline"* ]]
}

@test "a server that does not answer times out (exit 3)" {
  latest 0.2.0
  echo 5 > "$API/releases/latest.sleep"
  SECONDS=0
  run "$UPDATE" check --timeout 1
  [ "$status" -eq 3 ]
  [[ "$output" == *"within 1 s"* ]]
  [ "$SECONDS" -lt 4 ]
}

@test "check is read-only: it leaves no files behind" {
  latest 0.2.0
  mkdir -p "$BATS_TEST_TMPDIR/home" "$BATS_TEST_TMPDIR/tmp"
  HOME="$BATS_TEST_TMPDIR/home" TMPDIR="$BATS_TEST_TMPDIR/tmp" XDG_STATE_HOME= run "$UPDATE" check
  [ "$status" -eq 10 ]
  [ -z "$(ls -A "$BATS_TEST_TMPDIR/home" "$BATS_TEST_TMPDIR/tmp" | grep -v ':$' | grep -v '^$')" ]
}

# ---------------------------------------------------------------- usage

@test "usage errors are exit 2" {
  run "$UPDATE"
  [ "$status" -eq 2 ]
  run "$UPDATE" download
  [ "$status" -eq 2 ]
  run "$UPDATE" check --channel nightly
  [ "$status" -eq 2 ]
  run "$UPDATE" check --timeout 0
  [ "$status" -eq 2 ]
  run "$UPDATE" check --bogus
  [ "$status" -eq 2 ]
}
