# IsoWizardBoot

Native Qt 6 / QML application for writing USB-compatible ISO and IMG images
to USB drives on Linux.

## Requirements

Linux 5.15+, CMake 3.16+, a C++17 compiler, Qt 6.5+ (Quick,
QuickControls2, Quick Dialogs, and Qt Test when tests are enabled),
util-linux, polkit, and Make for the commands below.

## Install for your user (KDE, GNOME, and other Linux desktops)

Run from the project directory:

```sh
make install
```

This builds the app and installs it in `~/.local`, together with its icon and
application-menu entry. No `sudo` is needed. Search for **IsoWizardBoot** in
your application launcher, then pin it to your panel or dock if desired.
The launcher works even when `~/.local/bin` is not in `PATH`.

## Install for all users

```sh
make PREFIX=/usr/local
sudo cmake --install build --prefix /usr/local
```

## Build, test, or install using CMake

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
cmake --install build --prefix "$HOME/.local"
```

To run without installing: `./build/src/IsoWizardBoot`.
Configure with `-DBUILD_TESTING=OFF` if Qt Test is unavailable.
For package staging, use `DESTDIR=/path/to/stage cmake --install build --prefix /usr`.
Make also accepts `PREFIX`, `BUILD_DIR`, `BUILD_TYPE`, and `JOBS` overrides.

The icon is embedded in the executable and installed for desktop launchers.
A bare Linux binary may still have a generic file-manager icon; use the
installed application-menu entry. Qt runtime dependencies must be installed
on the machine that runs the app.

See [the full documentation](README) for usage, image compatibility, language
and theme settings, and disk-writing safety checks. The icon artwork and
ImageGen prompt are in [assets/icons](assets/icons/README.md).
