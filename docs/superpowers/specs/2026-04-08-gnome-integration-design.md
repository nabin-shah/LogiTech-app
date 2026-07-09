# GNOME Desktop Integration Design

## Goal

Implement `GnomeDesktop`, a GNOME Wayland implementation of `IDesktopIntegration` that provides active window tracking, running application listing, and global shortcut blocking — matching the existing KDE integration's capabilities.

## Scope

- GNOME on Wayland only (covers ~95% of GNOME users)
- GNOME Shell 42-44 (legacy extension API) and 45+ (ES modules API)
- Ubuntu 22.04+, Fedora 37+, Debian 12+, Arch

Out of scope: GNOME on X11 (falls back to `GenericDesktop`).

## Architecture

### Class Hierarchy

```
IDesktopIntegration (abstract interface)
  └── LinuxDesktopBase (new — shared .desktop resolution + app listing)
        ├── KDeDesktop (refactored — KWin-specific focus + shortcuts)
        ├── GnomeDesktop (new — GNOME Shell extension focus + shortcuts)
        └── GenericDesktop (refactored — no-op focus, inherits app listing)
```

### LinuxDesktopBase (extracted from KDeDesktop)

Shared logic currently duplicated across `KDeDesktop` and `GenericDesktop`:

- `desktopDirs()` — static list of .desktop file search paths (includes Flatpak, Snap, host paths)
- `resolveDesktopFile(resourceClass)` — scans .desktop files by StartupWMClass or filename component, with cache
- `runningApplications()` — scans .desktop files for GUI applications, returns sorted QVariantList
- `m_resolveCache` — QHash<QString, QString> for caching resolution results

`KDeDesktop` and `GenericDesktop` are refactored to inherit from `LinuxDesktopBase` instead of `IDesktopIntegration` directly. No behavioral changes — pure extraction.

### GnomeDesktop

Implements `IDesktopIntegration` via `LinuxDesktopBase`. Focus tracking uses a GNOME Shell extension that calls back to our D-Bus service.

**Members:**
- `m_available` — true if GNOME Shell D-Bus is reachable
- `m_lastAppId` — dedup consecutive focus events for the same app

**Methods:**

| Method | Behavior |
|--------|----------|
| `start()` | Ensure extension is installed and enabled, register D-Bus service at `com.logitune.app /FocusWatcher`, expose `focusChanged` slot |
| `available()` | Returns `m_available` |
| `desktopName()` | Returns `"GNOME"` |
| `detectedCompositors()` | Checks `XDG_CURRENT_DESKTOP` for "GNOME", returns `["Mutter"]` |
| `blockGlobalShortcuts(bool)` | Calls extension's `BlockShortcuts` method via D-Bus on `com.logitune.ShellExtension /org/gnome/shell/extensions/logitune` |
| `focusChanged(appId, title)` | D-Bus slot. Resolves `appId` via `resolveDesktopFile()`, dedup, emits `activeWindowChanged` |

### GNOME Shell Extension

Installed at `/usr/share/gnome-shell/extensions/logitune-focus@logitune.com/`. Two JS files for API compatibility; shared `metadata.json`.

**Directory structure:**
```
data/gnome-extension/
  metadata.json
  v42/extension.js    (GNOME 42-44: imports-based API)
  v45/extension.js    (GNOME 45+: ES modules API)
```

The packaging scripts select the correct `extension.js` based on the installed GNOME Shell version, or ship both and let `GnomeDesktop::start()` install the right one.

**metadata.json:**
```json
{
  "uuid": "logitune-focus@logitune.com",
  "name": "Logitune Focus Watcher",
  "description": "Reports active window to Logitune for per-app profile switching",
  "shell-version": ["42", "43", "44", "45", "46", "47", "48"],
  "version": 1
}
```

**Extension behavior (both versions):**

On `enable()`:
1. Connect to `global.display` `'notify::focus-window'` signal
2. Fire initial focus report for the current window

On focus change:
1. Read `global.display.focus_window`
2. Extract `app_id` (Wayland `wm_class`) and `title`
3. Call `com.logitune.app /FocusWatcher local.logitune.logitune.GnomeDesktop.focusChanged(appId, title)` via `Gio.DBus.session.call()`

On `BlockShortcuts(bool)` D-Bus method received:
1. Call `Main.layoutManager.inhibitShortcuts(bool)` or equivalent

On `disable()`:
1. Disconnect signal handler
2. Unexport D-Bus interface

### Desktop Detection Factory

`AppController` currently hardcodes `KDeDesktop`. Replace with a factory in `AppController::AppController()`:

```cpp
if (!desktop) {
    const QString xdgDesktop = QProcessEnvironment::systemEnvironment()
                                   .value("XDG_CURRENT_DESKTOP");
    if (xdgDesktop.contains("KDE", Qt::CaseInsensitive))
        m_ownedDesktop = std::make_unique<KDeDesktop>();
    else if (xdgDesktop.contains("GNOME", Qt::CaseInsensitive))
        m_ownedDesktop = std::make_unique<GnomeDesktop>();
    else
        m_ownedDesktop = std::make_unique<GenericDesktop>();
    m_desktop = m_ownedDesktop.get();
}
```

### Extension Auto-Enable Flow

`GnomeDesktop::start()`:

1. Check `XDG_SESSION_TYPE` — if not `wayland`, set `m_available = false` and return
2. Check if `org.gnome.Shell` D-Bus service is reachable — if not, set `m_available = false` and return
3. Detect GNOME Shell version via `org.gnome.Shell` `ShellVersion` property
4. Determine extension API version: major >= 45 → v45, else v42
5. Check if extension is installed at system path (`/usr/share/gnome-shell/extensions/`) or user path (`~/.local/share/gnome-shell/extensions/`)
6. If not installed, copy the correct version from app resources (`qrc:/`) to user path
7. Check if enabled: call `org.gnome.Shell.Extensions` `ListExtensions()`, look for our UUID with state=ENABLED
8. If not enabled: call `org.gnome.Shell.Extensions` `EnableExtension("logitune-focus@logitune.com")`
9. Register `com.logitune.app` D-Bus service and `/FocusWatcher` object with `focusChanged` slot exported
10. Set `m_available = true`

### Focus Event Filtering

Same desktop component filter as KDE, adapted for GNOME:

```cpp
static const QStringList kIgnored = {
    "gnome-shell",
    "org.gnome.Shell",
    "org.gnome.Shell.Extensions",
};
```

### Packaging

**Arch (PKGBUILD):** Install extension to `/usr/share/gnome-shell/extensions/logitune-focus@logitune.com/`

**Debian (.deb):** Same install path. Add optional dependency on `gnome-shell` (not required — KDE users shouldn't pull it).

**CMakeLists.txt additions:**
```cmake
install(FILES data/gnome-extension/metadata.json
        DESTINATION share/gnome-shell/extensions/logitune-focus@logitune.com)
install(FILES data/gnome-extension/v42/extension.js
        DESTINATION share/gnome-shell/extensions/logitune-focus@logitune.com/v42)
install(FILES data/gnome-extension/v45/extension.js
        DESTINATION share/gnome-shell/extensions/logitune-focus@logitune.com/v45)
```

The correct `extension.js` is copied to the extension root at runtime by `GnomeDesktop::start()` based on detected Shell version.

## Files to Create

| File | Purpose |
|------|---------|
| `src/core/desktop/LinuxDesktopBase.h` | Shared base class header |
| `src/core/desktop/LinuxDesktopBase.cpp` | Shared .desktop resolution + app listing |
| `src/core/desktop/GnomeDesktop.h` | GNOME integration header |
| `src/core/desktop/GnomeDesktop.cpp` | GNOME integration implementation |
| `data/gnome-extension/metadata.json` | Extension manifest |
| `data/gnome-extension/v42/extension.js` | GNOME 42-44 extension |
| `data/gnome-extension/v45/extension.js` | GNOME 45+ extension |

## Files to Modify

| File | Change |
|------|--------|
| `src/core/desktop/KDeDesktop.h/.cpp` | Inherit from `LinuxDesktopBase`, remove duplicated methods |
| `src/core/desktop/GenericDesktop.h/.cpp` | Inherit from `LinuxDesktopBase`, remove duplicated `runningApplications()` |
| `src/app/AppController.cpp` | Desktop factory based on `XDG_CURRENT_DESKTOP` |
| `src/core/CMakeLists.txt` | Add new source files |
| `CMakeLists.txt` | Install extension files |
| `scripts/package-arch.sh` | Install extension in PKGBUILD |
| `scripts/package-deb.sh` | Optional gnome-shell dependency |

## Testing

- Unit tests: mock `GnomeDesktop` the same way `KDeDesktop` is mocked via `MockDesktop` in `IDesktopIntegration`
- Manual test on GNOME: verify focus switching, DPI changes, button diversions across app profiles
- Verify KDE still works after refactor (no regressions from `LinuxDesktopBase` extraction)
- Verify `GenericDesktop` fallback still works on unknown DEs
