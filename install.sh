#!/usr/bin/env bash
# Installer for the unofficial Clapper fork: https://github.com/Venando/clapper
#
# One-liner:
#   curl -fsSL https://raw.githubusercontent.com/Venando/clapper/master/install.sh | bash
#
# - Arch-based systems: builds and installs a pacman package (clean to remove).
# - Other systems:      builds from source and installs into /usr/local.
#
# Everything runs inside main(), so a dropped download can never run half a script.
set -euo pipefail

workdir=""
cleanup() { [ -n "$workdir" ] && rm -rf "$workdir"; return 0; }
trap cleanup EXIT

# When piped from curl, stdin is the script itself; interactive tools must use the terminal.
TTY_IN=/dev/stdin
[ -t 0 ] || { [ -r /dev/tty ] && TTY_IN=/dev/tty; }

REPO_URL="https://github.com/Venando/clapper.git"
RAW_URL="https://raw.githubusercontent.com/Venando/clapper/master"

say() { printf '==> %s\n' "$*"; }
die() { printf 'error: %s\n' "$*" >&2; exit 1; }
need() { command -v "$1" >/dev/null 2>&1 || die "'$1' is required but not installed. $2"; }

install_arch() {
  need makepkg "Install the 'base-devel' package group."
  need curl "Install curl."
  workdir=$(mktemp -d)

  say "Fetching the PKGBUILD"
  curl -fsSL "$RAW_URL/packaging/arch/PKGBUILD" -o "$workdir/PKGBUILD"

  say "Building and installing (pacman will ask for sudo and may offer to replace 'clapper')"
  (cd "$workdir" && makepkg -si <"$TTY_IN")
}

install_generic() {
  need git "Install git."
  need meson "Install meson."
  need ninja "Install ninja (ninja-build)."
  need cc "Install a C compiler (gcc)."
  need pkg-config "Install pkg-config."
  need sudo "Install sudo, or build manually, see FORK.md."

  workdir=$(mktemp -d)

  say "Cloning the fork"
  git clone --depth 1 "$REPO_URL" "$workdir/clapper"

  say "Configuring (needs GTK4, libadwaita, GStreamer, libpeas-2, libsoup3 and friends)"
  meson setup "$workdir/build" "$workdir/clapper" --prefix=/usr/local --buildtype=release \
    || die "Configuration failed. Install the missing development packages listed above (see FORK.md) and re-run."

  say "Compiling"
  meson compile -C "$workdir/build"

  say "Installing into /usr/local (sudo)"
  sudo meson install -C "$workdir/build" <"$TTY_IN"
  command -v ldconfig >/dev/null 2>&1 && sudo ldconfig || true
}

main() {
  if command -v pacman >/dev/null 2>&1; then
    install_arch
  else
    install_generic
  fi
  say "Done. Start it with: clapper"
}

main "$@"
