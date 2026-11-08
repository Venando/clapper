# Clapper (unofficial fork)

This is an **unofficial fork** of [Clapper](https://github.com/Rafostar/clapper) by Rafał Dzięgiel
(Rafostar), based on version 0.11.0. It is not affiliated with or endorsed by the upstream project.
All credit for Clapper itself goes to upstream. For the stock player, use the
[upstream repository](https://github.com/Rafostar/clapper) and report issues unrelated to the changes
below there.

## What is different

**Queue**
* Folders can be added to the queue (`Ctrl+Shift+O`, "Add folder").
* Opening a media file queues the other media in the same folder as well.

**Tracks**
* `Alt+A` cycles audio tracks.
* `Alt+L` cycles subtitle tracks.
* `Alt+H` toggles subtitles on and off.
* The audio and subtitle track you picked (matched by language and title) is selected again when the next item starts.

**Subtitle navigation**
* `Home` / `End` jump to the previous / next subtitle, including embedded subtitle tracks.
* Embedded subtitle cue times are prefetched in the background so jumping is instant.
* Jumping back never lands on the subtitle that is currently on screen.

All shortcuts are listed in the in-app shortcuts overlay (`Ctrl+?`).

## Install

### One-liner

```sh
curl -fsSL https://raw.githubusercontent.com/Venando/clapper/master/install.sh | bash
```

* On **Arch-based** systems it builds and installs a proper pacman package named `clapper-modified`
  (it replaces the official `clapper` package; remove it with `sudo pacman -R clapper-modified`).
* On **other distributions** it builds from source and installs into `/usr/local` (needs `sudo`).
  The development packages must already be installed, see below. This path is less tested than the
  Arch one.

Prefer to read a script before running it? Download [`install.sh`](install.sh), read it, then run
`bash install.sh`.

### Arch Linux (manual)

```sh
mkdir clapper-modified && cd clapper-modified
curl -fsSLO https://raw.githubusercontent.com/Venando/clapper/master/packaging/arch/PKGBUILD
makepkg -si
```

### Any distribution (manual build)

```sh
git clone https://github.com/Venando/clapper.git
cd clapper
meson setup builddir
meson compile -C builddir
sudo meson install -C builddir
```

To try it without installing, run it straight from the build folder after `meson compile`
(the binary is `builddir/src/bin/clapper-app/clapper`).

Build requirements: `meson`, `ninja`, a C compiler, `pkg-config`, `vala`, GObject Introspection and
development files for GLib (>= 2.76), GStreamer (>= 1.24), GTK4, libadwaita, libpeas-2, libsoup3,
libmicrodns, graphene and pango. For playback you also want the GStreamer `base`, `good` and, for
most video files, `bad`/`ugly`/`libav` plugin sets. Package names differ per distribution, and
`meson setup` tells you exactly which dependency is missing.

## Keeping up with upstream

```sh
git remote add upstream https://github.com/Rafostar/clapper.git
git fetch upstream
git merge upstream/master
```

## License

Same as upstream: the libraries are `LGPL-2.1-or-later`, the application is `GPL-3.0-or-later`.
See `COPYING-LGPL` and `COPYING-GPL`.
