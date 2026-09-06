# User-local installation by default; override PREFIX for a system install.
PREFIX ?= $(HOME)/.local
BUILD_DIR ?= build
BUILD_TYPE ?= Release
JOBS ?= 2

.PHONY: all configure build test install
all: build

configure:
	cmake -S . -B "$(BUILD_DIR)" -DCMAKE_BUILD_TYPE="$(BUILD_TYPE)" -DCMAKE_INSTALL_PREFIX="$(PREFIX)"

build: configure
	cmake --build "$(BUILD_DIR)" --parallel "$(JOBS)"

test: build
	ctest --test-dir "$(BUILD_DIR)" --output-on-failure

install: build
	cmake --install "$(BUILD_DIR)" --prefix "$(PREFIX)"
