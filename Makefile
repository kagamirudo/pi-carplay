# pi-carplay — build and install the Raspberry Pi 5 head unit.
#
#   make deps        apt packages (sudo)
#   make build       h264bitstream + aasdk + OpenAuto (patched) + head-unit shell
#   make install     udev rule, labwc touch/autostart, OpenAuto config, restart script
#   make boot-config add the SPI display overlay to /boot/firmware/config.txt (reboot after)
#   make package     build dist/pi-carplay_<version>_arm64.deb and the GPL source tarball
#   make check       every quality gate CI runs (lint, analysis, tests, sanitizers, coverage)
#
# Run `make help` for every target.

SHELL := /bin/bash
.SHELLFLAGS := -eu -o pipefail -c
.DEFAULT_GOAL := help

REPO      := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
SRC_DIR   ?= $(HOME)/src
JOBS      ?= $(shell nproc)
BOOT_CFG  ?= /boot/firmware/config.txt
LABWC_DIR ?= $(HOME)/.config/labwc
BIN_DIR   ?= $(HOME)/.local/bin
VERSION   ?= $(shell cat $(REPO)/VERSION)
DIST_DIR  ?= $(REPO)/dist

# Upstream revisions the patches in patches/ were made against.
AASDK_URL     := https://github.com/openDsh/aasdk.git
AASDK_REV     := 1bc0fe69d5f5f505c978a0c6e32c860e820fa8f6
OPENAUTO_URL  := https://github.com/openDsh/openauto.git
OPENAUTO_REV  := e7caeb4d49186af867c1d4693c9f3bf9e9ef99e0
H264BS_URL    := https://github.com/aizvorski/h264bitstream.git
H264BS_REV    := 70124d3051ba45e6b326264f0b25e6f48a7479e7

HEADUNIT_DIR  := $(REPO)/raspi-headunit
HEADUNIT_BIN  := $(HEADUNIT_DIR)/build/headunit
OPENAUTO_BIN  := $(SRC_DIR)/openauto/bin

# Fill deploy/ templates for a from-source install.
RENDER := sed -e 's|@HEADUNIT@|$(HEADUNIT_BIN)|g' \
	-e 's|@SUPERVISE@|$(BIN_DIR)/pi-carplay-supervise|g' \
	-e 's|@AUTOAPP_LAUNCHER@|$(BIN_DIR)/pi-carplay-autoapp|g' \
	-e 's|@AUTOAPP@|$(OPENAUTO_BIN)/autoapp|g' \
	-e 's|@OPENAUTO_DIR@|$(OPENAUTO_BIN)|g' \
	-e 's|@DEFAULT_INI@|$(REPO)/deploy/openauto/openauto.ini|g'

SHELL_SCRIPTS := deploy/bin/aa-restart.sh deploy/bin/pi-carplay-autoapp deploy/bin/pi-carplay-boot-config \
	deploy/bin/pi-carplay-supervise deploy/bin/pi-carplay-wireless-setup packaging/pi-carplay-setup \
	packaging/postinst packaging/build-deb.sh packaging/test-deb.sh raspi-headunit/tests/smoke.sh \
	tools/pi-carplay-selftest tools/pi-carplay-wireless-check

# Line coverage of raspi-headunit/src may not drop below this (raise it, never lower it).
COVERAGE_MIN  ?= 80
QMLLINT       ?= /usr/lib/qt6/bin/qmllint
CMAKE_FLAGS   ?=
CTEST_ARGS    ?=

APT_PACKAGES := \
	build-essential cmake git pkg-config autoconf automake libtool \
	libusb-1.0-0-dev libssl-dev libprotobuf-dev protobuf-compiler \
	libboost-dev libboost-system-dev libboost-log-dev libboost-filesystem-dev \
	qtbase5-dev qtmultimedia5-dev libqt5multimedia5-plugins libqt5multimediawidgets5 \
	qtconnectivity5-dev qtdeclarative5-dev librtaudio-dev \
	qt6-base-dev qt6-base-dev-tools qt6-declarative-dev qt6-declarative-dev-tools qt6-wayland \
	qml6-module-qtquick qml6-module-qtquick-controls qml6-module-qtquick-layouts \
	qml6-module-qtquick-templates qml6-module-qtquick-window qml6-module-qtqml \
	qml6-module-qtqml-workerscript qml6-module-qtqml-models \
	bluez pulseaudio-utils wlr-randr grim \
	patchelf shellcheck

# Test and analysis tools (make check).
QA_PACKAGES := bats cppcheck gcovr lintian dbus

# Clone $(1) at commit $(3) into $(2) unless it already exists.
define fetch
	if [ ! -d "$(2)/.git" ]; then \
		echo ">> fetching $(1) @ $(3)"; \
		mkdir -p "$(2)"; \
		git -C "$(2)" init -q; \
		git -C "$(2)" remote add origin "$(1)"; \
		git -C "$(2)" fetch -q --depth 1 origin "$(3)"; \
		git -C "$(2)" checkout -q FETCH_HEAD; \
	fi
endef

# Apply patch $(2) to $(1) once; skip when it is already applied.
define apply_patch
	if git -C "$(1)" apply --reverse --check "$(2)" 2>/dev/null; then \
		echo ">> $(notdir $(2)) already applied"; \
	else \
		echo ">> applying $(notdir $(2))"; \
		git -C "$(1)" apply "$(2)"; \
	fi
endef

.PHONY: help
help: ## Show this help
	@echo "pi-carplay — Raspberry Pi 5 head unit"
	@echo
	@echo "Fresh Pi:  make deps build install boot-config && sudo reboot"
	@echo
	@grep -hE '^[a-zA-Z0-9_-]+:.*## ' $(MAKEFILE_LIST) | \
		awk 'BEGIN {FS = ":.*## "}; {printf "  \033[36m%-24s\033[0m %s\n", $$1, $$2}'
	@echo
	@echo "Variables: SRC_DIR=$(SRC_DIR)  JOBS=$(JOBS)  BOOT_CFG=$(BOOT_CFG)"

# ---------------------------------------------------------------- setup

.PHONY: deps
deps: ## Install build and runtime packages with apt (sudo)
	sudo apt-get update
	sudo apt-get install -y $(APT_PACKAGES)

.PHONY: deps-qa
deps-qa: ## Install the test and analysis tools used by make check (sudo)
	sudo apt-get install -y $(QA_PACKAGES)

# ---------------------------------------------------------------- build

.PHONY: build
build: openauto headunit ## Build everything (third-party stack and shell)

.PHONY: h264bitstream
h264bitstream: ## Build and install h264bitstream to /usr/local (sudo)
	@$(call fetch,$(H264BS_URL),$(SRC_DIR)/h264bitstream,$(H264BS_REV))
	cmake -S $(SRC_DIR)/h264bitstream -B $(SRC_DIR)/h264bitstream/build \
		-DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=ON \
		-DCMAKE_INSTALL_INCLUDEDIR=include/h264bitstream
	cmake --build $(SRC_DIR)/h264bitstream/build -j$(JOBS)
	sudo cmake --install $(SRC_DIR)/h264bitstream/build
	sudo ldconfig

.PHONY: aasdk
aasdk: ## Fetch, patch, build and install aasdk to /usr/local (sudo)
	@$(call fetch,$(AASDK_URL),$(SRC_DIR)/aasdk,$(AASDK_REV))
	@$(call apply_patch,$(SRC_DIR)/aasdk,$(REPO)/patches/aasdk.patch)
	cmake -S $(SRC_DIR)/aasdk -B $(SRC_DIR)/aasdk/build -DCMAKE_BUILD_TYPE=Release -DAASDK_TEST=OFF
	cmake --build $(SRC_DIR)/aasdk/build --target aasdk -j$(JOBS)
	sudo cmake --install $(SRC_DIR)/aasdk/build
	sudo ldconfig

.PHONY: openauto
openauto: h264bitstream aasdk ## Fetch, patch and build OpenAuto (autoapp)
	@$(call fetch,$(OPENAUTO_URL),$(SRC_DIR)/openauto,$(OPENAUTO_REV))
	@$(call apply_patch,$(SRC_DIR)/openauto,$(REPO)/patches/openauto.patch)
	cmake -S $(SRC_DIR)/openauto -B $(SRC_DIR)/openauto/build -DCMAKE_BUILD_TYPE=Release -DAASDK_DIR=/usr/local
	cmake --build $(SRC_DIR)/openauto/build --target autoapp -j$(JOBS)

.PHONY: headunit
headunit: ## Build the Qt 6 head-unit shell (warnings are errors)
	cmake -S $(HEADUNIT_DIR) -B $(HEADUNIT_DIR)/build -DCMAKE_BUILD_TYPE=Release -DHEADUNIT_WERROR=ON $(CMAKE_FLAGS)
	cmake --build $(HEADUNIT_DIR)/build -j$(JOBS)

# ---------------------------------------------------------------- quality gates

.PHONY: check
check: lint analyze test test-asan coverage ## Every gate CI runs before a merge or release

.PHONY: test
test: headunit test-scripts ## Unit, integration, UI, stress and smoke tests, plus script tests
	ctest --test-dir $(HEADUNIT_DIR)/build --output-on-failure -j$(JOBS) $(CTEST_ARGS)

.PHONY: test-unit
test-unit: headunit ## Fast tests only (unit label)
	ctest --test-dir $(HEADUNIT_DIR)/build --output-on-failure -j$(JOBS) -L unit

.PHONY: test-scripts
test-scripts: ## bats tests for the launcher, supervisor and setup scripts
	bats $(REPO)/tests/scripts

.PHONY: test-asan
test-asan: ## Every shell test under AddressSanitizer and UndefinedBehaviorSanitizer
	cmake -S $(HEADUNIT_DIR) -B $(HEADUNIT_DIR)/build-asan -DCMAKE_BUILD_TYPE=Debug -DHEADUNIT_WERROR=ON \
		-DHEADUNIT_SANITIZE=address,undefined
	cmake --build $(HEADUNIT_DIR)/build-asan -j$(JOBS)
	ASAN_OPTIONS=detect_leaks=1:abort_on_error=1:strict_string_checks=1 \
	UBSAN_OPTIONS=print_stacktrace=1:halt_on_error=1 HEADUNIT_STRESS_TAPS=1000 \
		ctest --test-dir $(HEADUNIT_DIR)/build-asan --output-on-failure -j$(JOBS)

.PHONY: coverage
coverage: ## Line and branch coverage; fails below COVERAGE_MIN percent of lines
	cmake -S $(HEADUNIT_DIR) -B $(HEADUNIT_DIR)/build-cov -DCMAKE_BUILD_TYPE=Debug -DHEADUNIT_COVERAGE=ON
	cmake --build $(HEADUNIT_DIR)/build-cov -j$(JOBS)
	find $(HEADUNIT_DIR)/build-cov -name '*.gcda' -delete
	HEADUNIT_STRESS_TAPS=500 ctest --test-dir $(HEADUNIT_DIR)/build-cov --output-on-failure -j$(JOBS)
	cd $(HEADUNIT_DIR) && gcovr -r . --object-directory build-cov --filter src/ --exclude src/main.cpp \
		--xml build-cov/coverage.xml --html-details build-cov/coverage.html --print-summary \
		--fail-under-line $(COVERAGE_MIN)

.PHONY: analyze
analyze: ## Static analysis: cppcheck and qmllint
	cppcheck --enable=warning,performance,portability --std=c++20 --inline-suppr --error-exitcode=1 \
		--quiet --suppress=missingIncludeSystem --library=qt -I $(HEADUNIT_DIR)/src $(HEADUNIT_DIR)/src
	$(QMLLINT) --unqualified disable $(HEADUNIT_DIR)/ui/*.qml

# ---------------------------------------------------------------- install

.PHONY: install
install: install-udev install-labwc install-openauto-config install-scripts ## Install all user and system config
	@echo
	@echo "Installed. If the display overlay is not set yet: make boot-config && sudo reboot"

.PHONY: install-udev
install-udev: ## udev rule so autoapp can open the phone in accessory mode (sudo)
	sudo install -m 0644 $(REPO)/deploy/udev/51-android-accessory.rules /etc/udev/rules.d/
	sudo udevadm control --reload-rules
	sudo udevadm trigger --subsystem-match=usb

.PHONY: install-labwc
install-labwc: ## labwc touch calibration and autostart (backs up existing files)
	mkdir -p $(LABWC_DIR)
	for f in rc.xml autostart; do \
		if [ -f $(LABWC_DIR)/$$f ] && ! cmp -s $(LABWC_DIR)/$$f <($(RENDER) $(REPO)/deploy/labwc/$$f); then \
			cp $(LABWC_DIR)/$$f $(LABWC_DIR)/$$f.bak; echo ">> backed up $(LABWC_DIR)/$$f"; \
		fi; \
		$(RENDER) $(REPO)/deploy/labwc/$$f > $(LABWC_DIR)/$$f; \
	done
	@echo ">> labwc config takes effect at the next login (or: kill -HUP the labwc process)"

.PHONY: install-openauto-config
install-openauto-config: ## OpenAuto settings (kept if one already exists)
	mkdir -p $(OPENAUTO_BIN)
	cp -n $(REPO)/deploy/openauto/openauto.ini $(OPENAUTO_BIN)/openauto.ini

.PHONY: install-scripts
install-scripts: ## Install aa-restart.sh, the autoapp launcher and the supervisor to ~/.local/bin
	mkdir -p $(BIN_DIR)
	for f in aa-restart.sh pi-carplay-autoapp pi-carplay-supervise; do \
		$(RENDER) $(REPO)/deploy/bin/$$f > $(BIN_DIR)/$$f; \
		chmod 0755 $(BIN_DIR)/$$f; \
	done

.PHONY: boot-config
boot-config: ## Add the SPI display overlay to config.txt (sudo, needs a reboot)
	BOOT_CFG=$(BOOT_CFG) $(REPO)/deploy/bin/pi-carplay-boot-config

# ---------------------------------------------------------------- release

.PHONY: package
package: ## Build dist/pi-carplay_<VERSION>_arm64.deb and the GPL source tarball
	$(REPO)/packaging/build-deb.sh $(VERSION) $(SRC_DIR) $(DIST_DIR)

.PHONY: test-package
test-package: ## Install the .deb in a scratch root and check it (lintian, smoke start, purge; sudo)
	$(REPO)/packaging/test-deb.sh $(DIST_DIR)/pi-carplay_$(VERSION)_$(shell dpkg --print-architecture).deb

.PHONY: lint
lint: ## shellcheck the scripts
	shellcheck $(addprefix $(REPO)/,$(SHELL_SCRIPTS))

# ---------------------------------------------------------------- run

.PHONY: restart
restart: ## Restart the shell and Android Auto on the running labwc session
	$(BIN_DIR)/aa-restart.sh

.PHONY: status
status: ## Show what is running and the health of the Pi
	@pgrep -a 'labwc|headunit|autoapp' || true
	@$(BIN_DIR)/pi-carplay-supervise --status 2>/dev/null || true
	@vcgencmd get_throttled 2>/dev/null || true
	@grep -E '^dtoverlay=piscreen' $(BOOT_CFG) || echo "display overlay not set"
	@tail -n 5 $(HOME)/.local/state/pi-carplay/crashes.log 2>/dev/null || true

.PHONY: selftest
selftest: ## Check this Pi: display, touch, Bluetooth, USB rule, binaries, power
	$(REPO)/tools/pi-carplay-selftest

.PHONY: wireless-check
wireless-check: ## Check readiness for wireless Android Auto: Bluetooth dongle, 5 GHz hotspot, ports (read-only)
	$(REPO)/tools/pi-carplay-wireless-check

.PHONY: wireless-setup
wireless-setup: ## Prepare wireless Android Auto: firmware, phone adapter, hotspot profile (ARGS=--dry-run to preview)
	$(REPO)/deploy/bin/pi-carplay-wireless-setup $(ARGS)

# ---------------------------------------------------------------- clean

.PHONY: clean
clean: ## Remove the shell build directories and dist/
	rm -rf $(HEADUNIT_DIR)/build $(HEADUNIT_DIR)/build-asan $(HEADUNIT_DIR)/build-cov $(DIST_DIR)

.PHONY: clean-all
clean-all: clean ## Also remove the OpenAuto, aasdk and h264bitstream build directories
	rm -rf $(SRC_DIR)/openauto/build $(SRC_DIR)/aasdk/build $(SRC_DIR)/h264bitstream/build
