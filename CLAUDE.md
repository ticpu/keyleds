# keyleds

Maintained fork of the abandoned `keyleds/keyleds`. C++17 daemon plus a C library,
driving per-key RGB on Logitech keyboards.

## Build

```
cmake -S . -B build -DWITH_TESTS=ON && cmake --build build -j$(nproc)
ctest --test-dir build --output-on-failure
```

Out-of-source only; `build/` is tracked but its contents are ignored. Options:
`WITH_KEYLEDSD` (ON), `WITH_LUA` (ON), `WITH_TESTS` (OFF), `WITH_PYTHON` (OFF),
`NO_DBUS`.

## Adding a dependency

Two files, and neither derives from the other:

- `debian/control` — `Build-Depends`. The Debian container and the native CI job
  both install from it through `mk-build-deps`, so they follow automatically.
- `keyleds.spec` — `BuildRequires`. The Fedora container installs from it through
  `dnf builddep`.

Miss one and only that distribution's packaging job fails, which is why both run
on every push. Runtime libraries need no `Depends`/`Requires` entry: dpkg and rpm
derive those from the linked sonames.

## Logging

Syslog levels, default `warning`; `-v` notice, `-vv` info, `-vvv` debug. The
prefix is lettered on a tty (`<N>`) and numeric through a pipe (`<5>`).

`DEBUG()` compiles to nothing when `NDEBUG` is defined, so release builds have no
per-key logging no matter the verbosity — use `-DCMAKE_BUILD_TYPE=Debug` for
that. Running from a build tree needs `-m build/lib`, or no effects load.

## Input path

Two sources feed `Service::handleKeyEvent`: `XInputWatcher` (X11 raw events,
which XWayland does not deliver at all) and `EvdevWatcher` (the keyboard's own
event nodes). The handler drops XInput-sourced events for any node evdev is
watching; without that, every keypress dispatches twice on X11. XInput emits
`detail - 8`, which is already the raw evdev keycode — the two share a space.

Window-class context still requires X, so under Wayland it only follows XWayland
clients. GNOME's `org.gnome.Shell.Introspect` denies non-allowlisted callers, so
a Shell extension would be the only route there.

## Traps

- `tools::Callback` holds a single listener and `connect()` asserts if one is
  already set; `disconnect()` first.
- `DeviceWatcher::setActive()` is called from the base constructor, where virtual
  overrides do not dispatch. `scan()` survives only because a 0-ms uv timer defers
  it; anything relying on a subclass override must be deferred the same way.
- Config paths go through the XDG search, so `-c keyledsd/keyledsd.conf` means
  `<xdg-config-dir>/keyledsd/keyledsd.conf`, not a filesystem-relative path.
- Stock udev tags only joysticks with `uaccess`, so the event nodes depend on
  `logitech.rules`. openrgb ships rules covering the same devices, which can hide
  a broken rule on a development machine.
- systemd user units install to the pkg-config `systemduserunitdir`, never
  `CMAKE_INSTALL_LIBDIR` — that is `lib64` on Fedora and openSUSE.

## Versioning and packaging

The version is written **only** in the top-level `CMakeLists.txt`; `keyledsd`
inherits it and it reaches the binaries as `KEYLEDSD_VERSION_STR`. dpkg and rpm
insist on their own copies, so `packaging/sync-version.sh` writes both and
`packaging/check-version.sh` fails the packaging builds once they have drifted.

`make deb` and `make rpm` from `build/`, or the scripts under `packaging/`
directly, build each package and install-test it on a clean system in podman.
They need podman and nothing else. CI runs the same scripts.

## Repository

`main` is the default branch. `master` is frozen at the fork point to keep the
upstream pull request intact — do not push to it.

Work on a feature branch and do not push to `main`; that is also how CI gets
exercised. When a fix belongs to a change that is still on the branch, rewrite
the commit it belongs to instead of adding another on top — no chains of fixes
over fixes.

Every commit has to build on its own. CI only ever builds the tip, so a split
that leaves an intermediate commit unbuildable passes unnoticed; it has happened
twice here, both times by adding a source file to the build in a later commit
than the one that started using it.
