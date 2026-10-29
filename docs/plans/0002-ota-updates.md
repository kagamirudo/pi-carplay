# Over-the-air updates

| | |
| --- | --- |
| **Plan** | 0002 |
| **Status** | Proposed |
| **Date** | 2026-10-29 |
| **Summary** | Over-the-air updates from GitHub Releases over the phone's or home internet: signed manifest, safe install when parked, automatic rollback. |

Phase 0 (this design and a read-only `pi-carplay-update check`) is implemented; nothing downloads or installs on its own yet.

The goal: the Pi in the car notices that a newer `pi-carplay_<version>_arm64.deb` has been published on GitHub Releases, fetches it over whatever internet the phone (or home Wi-Fi) offers, checks that it really came from this project's release pipeline, and installs it only when the car is parked and idle. If the new version does not come up healthy, the Pi goes back to the previous one by itself.

## Summary of the recommendations

| Topic | Recommendation |
| --- | --- |
| Connectivity | Home Wi-Fi when parked first, then the phone's Wi-Fi hotspot, then Bluetooth tethering (PAN). USB tethering is not usable while wired Android Auto holds the phone's USB port |
| Data use | Packages are under 1 MiB (0.1.0 is 757 KiB). Checks cost about 10 KiB. Downloads on metered networks only with the user's consent, and capped |
| Source of truth | GitHub Releases. A signed manifest attached to each release, fetched from the `releases/latest/download/` URL, which is not subject to the API rate limit |
| Authenticity | A detached OpenPGP signature over the manifest, made in `release.yml` with a key held in a protected GitHub environment, verified on the Pi with `gpgv` against a key shipped in the package. Build provenance attestations added alongside, for humans to audit |
| When to install | At the next boot, before the desktop session starts, or on request from the Settings page while parked. Never while Android Auto is connected |
| Recovery | Last three packages kept in `/var/cache/pi-carplay/debs`. `dpkg --configure -a` at boot. A new version must stay healthy for 10 minutes to be marked good; otherwise the next boot reinstalls the previous package |
| Privileges | A root systemd service does the installing. The desktop user can only ask it to start, through one polkit rule for one unit |

## How the Pi gets online

| Option | Works with wired Android Auto? | Works with wireless Android Auto (Pi hosts a 5 GHz AP)? | Speed | Cost to the user |
| --- | --- | --- | --- | --- |
| Home Wi-Fi in the garage | Yes, `wlan0` as a client | Only when `wlan0` is not the AP (car parked, no phone) or with a second Wi-Fi adapter | Fast | Unmetered |
| Phone Wi-Fi hotspot | Yes, `wlan0` as a client | No: `wlan0` is busy being the AP, and the phone's Wi-Fi is a client of the Pi | Fast | Mobile data; Android marks it metered |
| Bluetooth tethering (PAN) | Yes | Yes, ideally on the second (USB) Bluetooth adapter | About 0.5–2 Mbit/s, enough for a 1 MiB package in under 20 s | Mobile data |
| USB tethering | No (see below) | Not applicable, nothing on USB | Fast | Mobile data |

**USB tethering is out while Android Auto is wired.** Wired Android Auto switches the phone's USB port into Android Open Accessory mode, where the phone exposes the accessory interface (plus optionally adb) instead of its normal functions. USB tethering is a different USB gadget function (RNDIS/NCM), and Android does not offer it while it is in accessory mode (worth confirming once on the actual phone). It could only be used when Android Auto is not running, and then autoapp would claim the phone again as soon as it appears, so it is not worth supporting.

**Phone hotspot is the best in-car option today.** With wired Android Auto, `wlan0` is free, so the Pi can join the phone's hotspot as an ordinary NetworkManager Wi-Fi connection. Android hotspots announce themselves as metered (the `ANDROID_METERED` vendor string in their DHCP answers), and NetworkManager reports that as `GENERAL.METERED: yes (guessed)`; any profile can also be marked metered by hand with `nmcli connection modify <name> connection.metered yes`. So the updater can tell it apart from home Wi-Fi usually without any configuration. The catch is that most people do not leave the hotspot on.

**Bluetooth PAN is the option that survives wireless Android Auto.** The Pi is a PAN user (PANU) of the phone's Bluetooth tethering (Android: "Bluetooth tethering"; iPhone: Personal Hotspot over Bluetooth). BlueZ provides the `org.bluez.Network1` interface, and NetworkManager can bring it up as a `bluetooth` connection with `bt-type panu`. Caveats:

- Android turns Bluetooth tethering off after the phone restarts on many versions, so it may need re-enabling.
- The head unit already streams A2DP audio to the car stereo over Bluetooth. Running PAN on the same onboard adapter can make that audio stutter. The USB Bluetooth dongle being added for wireless Android Auto gives a second adapter; the plan is to keep phone links (wireless AA bootstrap, PAN) on one adapter and the stereo on the other. That split needs agreeing with the wireless Android Auto work.
- PAN is slow, but the package is small, so this does not matter for this project.

**Interaction with wireless Android Auto.** Once the Pi hosts a 5 GHz access point on `wlan0` for wireless Android Auto, `wlan0` cannot also be a client of the phone's hotspot or of home Wi-Fi (the Pi 5's CYW43455 only does AP and client at the same time on the same channel, and not reliably). Then the order becomes: home Wi-Fi when the AP is down (for example, when the car is parked and no phone has connected for a while, NetworkManager can switch `wlan0` back to client mode), Bluetooth PAN while driving, or a cheap USB Wi-Fi adapter dedicated to the client role. The updater itself does not care which interface it uses; it only asks NetworkManager whether the current default route is metered.

**The updater never changes network state.** It does not turn on tethering, join networks or switch `wlan0` between AP and client. Connectivity belongs to NetworkManager profiles that the user (or the wireless Android Auto setup) creates. The updater only uses whatever internet there is, if the policy allows.

### Data policy

| Setting (`/etc/pi-carplay/update.conf`) | Default | Meaning |
| --- | --- | --- |
| `CHECK` | `yes` | Ask for the latest version (about 10 KiB) |
| `DOWNLOAD` | `unmetered` | `unmetered`: only on networks NetworkManager does not mark metered. `always`: also on the phone. `never`: only by hand |
| `METERED_MAX_KIB` | `5120` | Never download more than this on a metered network, even with `DOWNLOAD=always` |
| `CHANNEL` | `stable` | `stable` or `beta` (prereleases) |
| `APPLY` | `boot` | `boot`: install at the next boot. `manual`: only from Settings or the command line |

A check of a 1 MiB package costs nothing worth worrying about, so checking on any network is fine. The real risk is a future release that changes dependencies and needs `apt-get update` (Raspberry Pi OS indexes are tens of MiB); that path is unmetered-only, see [Dependencies](#dependencies).

## Where updates come from

### GitHub Releases, with a signed manifest

Every tag already produces a release with the `.deb`, the GPL source and `SHA256SUMS`, and only after every gate in `pipeline.yml` passed. We keep that and add two assets:

- `pi-carplay-release.json`, the manifest:

  ```json
  {
    "schema": 1,
    "version": "0.2.0",
    "channel": "stable",
    "published": "2026-11-12T10:00:00Z",
    "min_from_version": "0.1.0",
    "deb": {
      "name": "pi-carplay_0.2.0_arm64.deb",
      "size": 781234,
      "sha256": "…",
      "url": "https://github.com/kagamirudo/pi-carplay/releases/download/v0.2.0/pi-carplay_0.2.0_arm64.deb"
    },
    "depends": "libc6 (>= 2.34), …",
    "notes_url": "https://github.com/kagamirudo/pi-carplay/releases/tag/v0.2.0"
  }
  ```

- `pi-carplay-release.json.asc`, its detached signature.

`depends` is copied from the package's control file so the Pi can tell before downloading whether the new version needs anything it does not have. `min_from_version` lets a release say "update to 0.3.0 first" if a migration ever needs a stepping stone.

The Pi fetches `https://github.com/kagamirudo/pi-carplay/releases/latest/download/pi-carplay-release.json`. That is a plain redirect to the asset, served outside the REST API, so the 60 requests per hour limit for unauthenticated API calls does not apply. This matters because a phone hotspot sits behind carrier-grade NAT, where many strangers share one public IP and its API quota. The beta channel has no such URL (`latest` skips prereleases), so it uses the API (`/repos/kagamirudo/pi-carplay/releases`), sends `If-None-Match` with the last ETag, and checks at most a few times a day. GitHub documents that a `304 Not Modified` answer does not count against the limit only for authenticated requests, so a rate-limited check simply waits for the next run; `check` explains the limit and accepts an optional token in `PI_CARPLAY_GITHUB_TOKEN`.

The Phase 0 `check` command uses the API for both channels, because the manifest does not exist yet.

### Why not an apt repository on GitHub Pages

A signed apt repository (`deb [signed-by=…] https://kagamirudo.github.io/pi-carplay stable main`) would let plain `apt` do the work, including dependencies and `unattended-upgrades`. Reasons not to start there:

- `apt-get update` refreshes every configured source, including Raspberry Pi OS and Debian, which is tens of MiB on a metered phone connection. It can be limited to our source with `-o Dir::Etc::sourcelist=…`, but then the tooling is custom anyway.
- `unattended-upgrades` knows nothing about "the car is moving". We would wrap it in the same safety logic regardless.
- It needs a Pages deployment, `reprepro` or `aptly` in CI, and repository key management on top of the release key.

It remains a good later step if other people start using the project, since it also gives them `apt upgrade`. The signing key proposed below can sign that repository too.

### Versions and channels

Versions compare with `dpkg --compare-versions`, which already gets this project's scheme right: `0.2.0~rc1 < 0.2.0`, and CI builds `0.2.0~git<sha>` sort below the `0.2.0` release. A device running a CI build newer than the latest release reports "up to date".

Open issue for the beta channel: Git tag names cannot contain `~`, but `release.yml` requires the tag to equal `VERSION`. A prerelease tag `v0.2.0-rc1` would need `release.yml` to map `-rc` to `~rc` and mark the release as a prerelease. The updater takes the version from the `.deb` asset name, not the tag. GitHub may rewrite unusual characters in asset names, so this mapping has to be tested on the first beta release.

## Making sure the package is ours

A SHA-256 sum next to the file, or GitHub's own `digest` field, only proves the download was not corrupted. Anyone who can replace the asset (a stolen token, a compromised account, a malicious redirect target) can replace the sum too. The device needs a check rooted in something it already trusts.

| Option | On the Pi | In CI | Trust root | Verdict |
| --- | --- | --- | --- | --- |
| OpenPGP detached signature, verified with `gpgv` | `gpgv` is already installed on every Raspberry Pi OS system (apt depends on it) | `gpg --detach-sign` with a key from a GitHub secret | A public key shipped in the package | **Recommended** |
| minisign / signify | `minisign` (0.11 in Bookworm) is a new dependency | Simple, small keys | Public key in the package | Simpler format, but an extra package and its own key format; nothing else on the system uses it |
| GitHub artifact attestations (`actions/attest-build-provenance`, `gh attestation verify`) | Needs the `gh` CLI, a GitHub login on the car, and network access to GitHub and Sigstore at verify time | One workflow step | GitHub's Sigstore instance and the workflow identity | Not suitable on the device; good as an extra audit trail |

**Decision: OpenPGP signature over the manifest, verified with `gpgv`.** It adds no dependency on the Pi, works offline once the files are downloaded, and the same key can later sign an apt repository. The manifest carries the package's SHA-256 and size, so one signature covers the package. We also add `actions/attest-build-provenance` to `release.yml` because it is one step and lets anyone run `gh attestation verify pi-carplay_0.2.0_arm64.deb --repo kagamirudo/pi-carplay` on a laptop. The Pi does not rely on it.

Key handling:

- An Ed25519 primary key kept offline (not on GitHub, not on the Pi), with a signing subkey exported to a GitHub secret.
- The secret lives in a `release` environment that only `release.yml` on `v*` tags can use, optionally with a required reviewer, so pull request workflows can never read it.
- The package ships the public key as `/usr/share/pi-carplay/keys/release.gpg`. A rotated key is shipped in a release signed by the old key, with both keys accepted for one release.
- If the subkey leaks: revoke it, ship a release with the new key signed by the old one, and publish a notice. Devices that are not updated in between still trust the old key; this is accepted for a one-person project.

Other checks on the Pi, before installing anything:

- The manifest signature verifies against the shipped key.
- The downloaded size and SHA-256 equal the manifest's.
- `dpkg-deb -f` reports `Package: pi-carplay` and the manifest's version and architecture.
- The version is newer than the installed one. An attacker could serve an old, correctly signed release with a known bug, so downgrades only happen through the explicit `rollback` command.

## Safety in a car

### When it is safe to install

The Pi has no ignition or speed signal today. Power most likely drops the moment the ignition goes off, so "install on shutdown" is not possible without a power-hold circuit or UPS HAT. Installing while Android Auto is projecting would restart the screen under the driver. So:

1. **Never** while Android Auto is in use. The updater treats any of these as "busy": a USB device in accessory mode (vendor `18d1`, product `2d00`–`2d05`), a running `autoapp` with an active phone session, an active Bluetooth call, or (later) the shell reporting that a phone page is open.
2. **Default: at the next boot**, before the desktop session starts. A oneshot unit ordered before the display manager installs a downloaded, verified package. The program files are not running yet, nothing on screen restarts, and installing a 1 MiB package takes a few seconds on a Pi 5. The cost is a slightly longer boot on the next drive; if it takes more than 60 seconds, the unit gives up and boots the old version.
3. **On request**: "Install now" on the Settings page, allowed only when the "busy" checks above are all clear. It installs, then restarts the shell and Android Auto with `pi-carplay-restart`.

Downloading is fine at any time the data policy allows. It is small, runs at low priority (`nice`, `ionice`, `curl --limit-rate`), and does not touch the running programs.

### Memory and load budget

The Pi has 4 GB of RAM and only 200 MB of swap, and running out of memory has already taken it down (several parallel sanitizer and coverage builds during development). An out-of-memory reset in the middle of `dpkg` is exactly the case that recovery must handle, so the updater avoids causing one:

- `download` and `apply` skip their run when `MemAvailable` in `/proc/meminfo` is below 1200 MB, or when a heavy job holds `/tmp/pi-carplay-build.lock`, the lock that local builds and tests take (`flock /tmp/pi-carplay-build.lock nice -n 10 …`). They try again on the next timer run.
- The units set `MemoryMax=` (for example 256M), `Nice=10` and `IOSchedulingClass=idle`, so a runaway download or a hung `dpkg` cannot push the shell or Android Auto out of memory.
- Nothing in the updater compiles. It only downloads and installs prebuilt packages.

### Download, verify, install

- Downloads go to `/var/cache/pi-carplay/partial/` and resume with `curl --continue-at -`, so a hotspot that drops halfway does not start from zero.
- A completed and verified package moves to `/var/cache/pi-carplay/debs/` with an atomic rename. Only verified files are ever in that directory.
- The installer runs `dpkg -i` (not `apt`, see below) on the cached file, with logs in `/var/log/pi-carplay/update.log` and a short status line in the state file.

### Power loss during an install

dpkg unpacks each file next to the old one and renames it into place, and fsyncs as it goes, so a power cut leaves the package half-configured rather than as a mix of broken files. The update service runs at every boot, before anything else from the project:

1. `dpkg --audit`; if anything is half-installed or half-configured, `dpkg --configure -a`.
2. If `pi-carplay` is still not `install ok installed`, reinstall the package the state file says was being installed, or, if that fails, the previous one.
3. Only then continue with the normal boot.

The package is small, so the window for this is a couple of seconds. A read-only root with an overlay, or A/B root partitions, would close it completely; that is Phase 3 and probably not worth it for a 1 MiB package.

### Health check and automatic rollback

After installing, the version is "on trial":

1. Immediately: the shipped binaries resolve every library (`ldd`), and `headunit --smoke` starts offscreen (the package test already does this).
2. In the session: `pi-carplay-selftest` runs, and the supervisor's `crashes.log` is watched. The version is marked good after 10 minutes without a crash loop (more than three restarts of either program).
3. If any step fails, or the Pi reboots while the version is still on trial for the second time (a boot loop or a crash that takes the session down), the next boot reinstalls the previous cached package with `dpkg -i` and marks the new version as bad so it is not tried again. (`apt install --allow-downgrades ./old.deb` works too, but needs apt to resolve dependencies.)

The last three verified packages stay in `/var/cache/pi-carplay/debs/` (about 3 MiB in total), so a rollback never needs the network. `pi-carplay-update rollback` does the same by hand.

### Dependencies

A new `.deb` that needs a package the Pi does not have cannot install offline. Rules:

- A release that only changes our own files (the common case) has the same `depends` as the installed package. The updater compares the manifest's `depends` with the installed `Depends` and, when they match, installs with `dpkg -i` and no network at all.
- When dependencies change, the updater needs an unmetered network: it runs `apt-get update` and `apt-get install --download-only ./new.deb` while online, which puts the extra packages in apt's cache. The boot-time install then runs `apt-get install --no-download ./new.deb`. If that is not possible, the update waits and the Settings page says why.
- The changelog calls out dependency changes, and the release procedure gets a checkbox for it.

### Settings and data migrations

- The shell already tolerates a corrupt or unknown `headunit.json` and falls back to defaults. New versions must also read the previous version's file, and old versions must ignore unknown keys, so a rollback never loses settings.
- Migrations that change files run from the shell at start-up (per user), not from `postinst` (root). They keep a copy of the old file (`headunit.json.v1`) until the version is marked good.
- `postinst` stays idempotent and must work when moving to an older version too.

### Releases

- Already true: a release is only published when every gate in `pipeline.yml` passes for the tagged commit, plus the hardware checklist.
- New: publish to `beta` (prerelease) first and let the Pi in the car run it for a few drives before promoting to stable. With one car, that is the staged rollout. A percentage-based rollout (`rollout` field in the manifest, devices hash a random ID) is easy to add later if more people use the project.

### In the shell

- The existing `ShellPresenter` notice shows "Update 0.2.0 ready, installs at next start" or "Update 0.2.0 failed, back on 0.1.0".
- The Settings page gets an Updates section: installed version, latest version, the state ("Up to date", "Downloading 40 %", "Ready", "Waiting for Wi-Fi"), and buttons **Check now**, **Install now** (only when parked and idle), **Later**, plus the data policy choice.
- The shell reads the state file and calls `systemctl start` over D-Bus. It never runs anything as root itself.
- Updater logs for the user stay in `~/.local/state/pi-carplay/` next to the supervisor's logs; the root service writes `/var/log/pi-carplay/update.log`, which the shell can show.

## Architecture

```
 pi-carplay-update.timer (root, every 6 h + 2 min after boot)
        │
        ▼
 pi-carplay-update check → download ──► /var/cache/pi-carplay/debs/  (verified only)
        │ state.json                        │
        ▼                                   ▼
 /var/lib/pi-carplay/update/state.json   pi-carplay-update-apply.service
        ▲ read                              (root oneshot: at boot before the
        │                                    display manager, or on request)
 headunit Settings page ── D-Bus StartUnit ─┘  polkit: only this unit, only
                                               the desktop user's group
```

### Components

| Component | Runs as | Does |
| --- | --- | --- |
| `pi-carplay-update check` | Anyone | Asks for the latest version. Phase 0, implemented |
| `pi-carplay-update download` | Root (timer) | Applies the data policy, resumes, verifies, caches |
| `pi-carplay-update apply` | Root (service) | Busy checks, install, health check, record the trial |
| `pi-carplay-update rollback` | Root | Reinstalls the previous cached package |
| `pi-carplay-update recover` | Root (every boot) | `dpkg --configure -a`, finish or undo an interrupted install, roll back a failed trial |
| `pi-carplay-update status` | Anyone | Prints the state file, as text or `--json` |
| `pi-carplay-update.timer` and `.service` | Root, sandboxed (`ProtectSystem=strict`, `ReadWritePaths=/var/cache/pi-carplay /var/lib/pi-carplay`, `PrivateTmp`, `NoNewPrivileges`) | check and download |
| `pi-carplay-update-apply.service` | Root | apply; `Before=display-manager.service` when triggered at boot |
| `50-pi-carplay-update.rules` (polkit) | | Lets members of a `pi-carplay` group start (only start) `pi-carplay-update-apply.service` and `pi-carplay-update.service` |

**Why a root service and polkit, not sudo or the user session.** Installing a package needs root. A sudoers line for `dpkg -i` would let the desktop user install any package, which is root for anyone with a shell as that user. With polkit, the user can only start a unit whose behaviour is fixed, which then only installs files from a root-owned directory that only contains signature-checked packages. The download runs as root too, but in a sandbox, so that only one place writes to the cache. Bookworm ships polkitd 122, which supports JavaScript rules.

### State file

`/var/lib/pi-carplay/update/state.json`, world-readable, written atomically (temporary file, `fsync`, rename):

```json
{
  "schema": 1,
  "channel": "stable",
  "installed": "0.1.0",
  "last_check": "2026-11-12T10:03:12Z",
  "last_check_result": "update-available",
  "latest": {"version": "0.2.0", "size": 781234, "sha256": "…"},
  "downloaded": {"version": "0.2.0", "path": "/var/cache/pi-carplay/debs/pi-carplay_0.2.0_arm64.deb"},
  "pending": "boot",
  "trial": {"version": "0.2.0", "previous": "0.1.0", "started": "…", "boots": 1},
  "bad_versions": [],
  "last_error": null
}
```

### CI changes (`release.yml`)

1. In the `publish` job, after the checksums: write `pi-carplay-release.json` from the built `.deb` (`dpkg-deb -f` for version and `Depends`, `stat` and `sha256sum`), import the signing subkey from the `release` environment secret, `gpg --armor --detach-sign` it, and `gpgv` it against the public key in the repository before uploading. A release whose signature does not verify fails.
2. Also sign `SHA256SUMS` (`SHA256SUMS.asc`) for people installing by hand.
3. `actions/attest-build-provenance` on the `.deb` (needs `id-token: write` and `attestations: write`).
4. Prereleases: accept `vX.Y.Z-rcN` tags that map to `VERSION` `X.Y.Z~rcN`, and create them with `--prerelease`.

### Tests

- bats, with a local fake GitHub (`tests/scripts/fake_github.py`, already used by `update.bats`) that serves the manifest, signature and package: good path; bad signature; signature by an unknown key; size or SHA-256 mismatch; truncated download resumed; older version refused; network drops mid-download; metered network with each `DOWNLOAD` setting (NetworkManager answers faked through a `PATH` stub for `nmcli`).
- Apply and rollback in the clean Bookworm container of `pipeline.yml`: install package A, update to B through the fake server, make B's smoke test fail, check that the next `recover` returns to A; kill `dpkg` midway and check `recover` finishes the job.
- A throwaway signing key generated in the test, so tests never touch the release key.
- On the hardware checklist: update over the phone hotspot and over Bluetooth PAN, pull the power during an install, and confirm an update is refused while Android Auto is projecting.

## Phases

| Phase | Contents | Effort | Main risks |
| --- | --- | --- | --- |
| 0 | This document. `pi-carplay-update check` (read-only, text and JSON, exit codes), bats tests against a fake API, packaged and checked by the package tests | Done in this change | None: nothing is downloaded or installed |
| 1 | Manifest and signature in `release.yml`, the key in a protected environment, `download`, `apply`, `rollback`, `recover`, `status` run by hand as root; the boot-time recovery unit; tests above | 2–3 days | Key management mistakes; the first signed release is the first real test of the CI step |
| 2 | Timer, data policy with NetworkManager metered detection, apply at boot, polkit rule, Settings page and notices in the shell, trial and automatic rollback | 3–5 days | Shell work needs its own tests and coverage; boot ordering against the display manager; Bluetooth PAN reliability on real phones |
| 3 (optional) | Read-only root with an overlay, or A/B partitions, for whole-system updates | 1–2 weeks | Large change to the OS image and the install story; likely unnecessary while the project only updates its own 1 MiB package |

## Open questions

1. **Connectivity:** is the phone hotspot acceptable for downloads, or only home Wi-Fi? Does the car get within reach of home Wi-Fi when parked? Should Bluetooth PAN be set up (needs "Bluetooth tethering" on the phone)?
2. **Data budget:** is the proposed 5 MiB cap per update on mobile data right, or should mobile downloads be off by default?
3. **Signatures:** require a valid signature from the first automatic update (recommended; 0.1.0 devices would need one manual install of the first signed release), or allow unsigned updates until then?
4. **When to install:** is installing at the next start (a few seconds added to that boot) acceptable, or should it only ever happen from the Settings page?
5. **Power:** is the Pi powered only while the ignition is on? A power-hold circuit would allow installing after the engine is switched off, which is the most natural time.
6. **Second radio:** with wireless Android Auto planned, is a USB Wi-Fi adapter for the client role acceptable, or should updates rely on Bluetooth PAN and home Wi-Fi?
7. **Beta channel:** is a beta channel wanted, given it needs the `-rc` tag mapping in `release.yml`?
