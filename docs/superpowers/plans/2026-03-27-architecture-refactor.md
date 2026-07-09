# Architecture Refactor — Pluggable Multi-Device, Multi-Desktop

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Refactor Logitune into a modular architecture where adding a new peripheral, desktop environment, or compositor requires only implementing an interface and registering it — no changes to existing code.

**Architecture:** Plugin-style interfaces for devices, desktop environments, and input injection. A central `AppController` replaces the 300-line main.cpp lambda soup. Device-specific knowledge (CIDs, features, hotspot positions) lives in device descriptor files, not hardcoded arrays. The HID++ transport layer gets proper testability via interfaces.

**Tech Stack:** C++20, Qt 6.x Quick, HID++ 2.0 over hidraw, uinput, D-Bus, CMake

---

## File Structure

### New files to create

```
src/core/interfaces/
  IDevice.h                    — Abstract device interface (features, CIDs, capabilities)
  IDesktopIntegration.h        — Abstract desktop integration (window tracking, shortcuts)
  IInputInjector.h             — Abstract input injection (keystrokes, scroll, media)
  ITransport.h                 — Abstract HID++ transport (for testing)

src/core/
  AppController.h/.cpp         — Orchestrator: replaces main.cpp business logic
  DeviceRegistry.h/.cpp        — Maps PID → device descriptor, factory for device profiles
  ButtonAction.h               — Single canonical ButtonAction definition

src/core/devices/
  MxMaster3sDescriptor.h/.cpp  — Device-specific: CIDs, feature set, hotspot positions, images

src/core/desktop/
  KDeDesktop.h/.cpp            — KDE/KWin integration (implements IDesktopIntegration)
  GenericDesktop.h/.cpp         — Fallback for unknown DEs

src/core/input/
  UinputInjector.h/.cpp        — Linux uinput (implements IInputInjector)
```

### Files to heavily modify

```
src/app/main.cpp               — Slim down to ~80 lines: create AppController, register QML, run
src/core/DeviceManager.h/.cpp  — Remove business logic, keep only HID++ lifecycle
src/core/ActionExecutor.h/.cpp — Extract injection to UinputInjector, keep gesture detector
src/app/models/DeviceModel.h/.cpp — Read directly from DeviceManager, remove stale caches
```

### Files to delete

```
src/core/Placeholder.cpp       — Dead file
```

### Files to modify (minor)

```
src/core/hidpp/Transport.h/.cpp — Fix m_running to std::atomic<bool>, remove dead run() code
src/core/hidpp/HidppTypes.h     — Remove misleading kDeviceIndexReceiver constant
src/core/CMakeLists.txt          — Add new source files
src/app/CMakeLists.txt           — Add AppController
```

---

## Task 1: Fix Critical Bugs (no architecture change)

**Files:**
- Modify: `src/core/hidpp/Transport.h`
- Modify: `src/core/hidpp/Transport.cpp`
- Modify: `src/core/ActionExecutor.cpp`
- Modify: `src/core/ActionExecutor.h`
- Modify: `src/core/ProfileEngine.h`
- Delete dead code in: `src/core/DeviceManager.h`, `src/core/DeviceManager.cpp`
- Modify: `src/core/hidpp/HidppTypes.h`
- Delete: `src/core/Placeholder.cpp`
- Modify: `src/core/CMakeLists.txt`

These are the 3 critical bugs + dead code removal. No new architecture yet.

- [ ] **Step 1: Fix Transport::m_running data race**

In `src/core/hidpp/Transport.h`, change:
```cpp
// line 32 — change:
bool m_running = false;
// to:
std::atomic<bool> m_running{false};
```
Add `#include <atomic>` at the top. Remove the misleading comment "uses a mutex to serialize access" from line 17.

- [ ] **Step 2: Remove dead I/O thread infrastructure**

In `src/core/DeviceManager.h`, remove:
```cpp
void startIoThread();  // line 107 — never called
QThread m_ioThread;    // line 122 — never used
```

In `src/core/DeviceManager.cpp`, delete the entire `startIoThread()` implementation (around line 800-815) and `stopIoThread()` (around line 817-830). In `disconnectDevice()`, remove the `stopIoThread()` call.

In `src/core/hidpp/Transport.h`, remove `void run()` and `void stop()` declarations. In `src/core/hidpp/Transport.cpp`, delete the `run()` and `stop()` implementations.

- [ ] **Step 3: Unify ButtonAction — single canonical definition**

Create `src/core/ButtonAction.h`:
```cpp
#pragma once
#include <QString>

namespace logitune {

struct ButtonAction {
    enum Type {
        Default,
        Keystroke,
        GestureTrigger,
        SmartShiftToggle,
        AppLaunch,
        DBus,
        Media,
    };

    Type type = Default;
    QString payload;
};

} // namespace logitune
```

In `src/core/ProfileEngine.h`:
- Remove the inline `ButtonAction` struct definition (lines 12-24)
- Add `#include "ButtonAction.h"`

In `src/core/ActionExecutor.cpp`:
- Remove the `#ifndef LOGITUNE_BUTTON_ACTION_DEFINED` guarded local `ButtonAction` definition (lines 19-29)
- Add `#include "ButtonAction.h"`

In `src/core/ActionExecutor.h`:
- Add `#include "ButtonAction.h"`

- [ ] **Step 4: Remove duplicate gesture pipeline**

In `src/app/main.cpp`, remove connection 7 (the `gestureEvent` → `GestureDetector` path, lines 438-461). The `gestureRawXY` + manual accumulation path (connection 5, lines 332-412) is the one that actually works with ReprogControls diversion. The `GestureDetector` class in `ActionExecutor` can stay for now but should not be wired.

Also remove the `GestureV2` signal and notification handler from `DeviceManager`:
- In `DeviceManager.h`, remove `void gestureEvent(int dx, int dy, bool released);` from signals
- In `DeviceManager.cpp` `handleNotification()`, remove the GestureV2 notification block (lines ~756-765)

- [ ] **Step 5: Remove misleading constant and dead file**

In `src/core/hidpp/HidppTypes.h`, remove:
```cpp
constexpr uint8_t kDeviceIndexReceiver = 0xFF;  // never used, misleading
```

Delete `src/core/Placeholder.cpp`. Remove it from `src/core/CMakeLists.txt` if listed.

- [ ] **Step 6: Build and test**

```bash
cd /home/mina/repos/logitune/build && cmake --build . 2>&1 | tail -20
./tests/logitune-tests
```

- [ ] **Step 7: Commit**

```bash
git add -A && git commit -m "fix: resolve critical bugs — atomic m_running, unify ButtonAction, remove duplicate gesture pipeline, delete dead code"
```

---

## Task 2: Create Core Interfaces

**Files:**
- Create: `src/core/interfaces/ITransport.h`
- Create: `src/core/interfaces/IDesktopIntegration.h`
- Create: `src/core/interfaces/IInputInjector.h`
- Create: `src/core/interfaces/IDevice.h`

These are pure abstract classes. No implementations yet — just the contracts.

- [ ] **Step 1: Create ITransport**

Create `src/core/interfaces/ITransport.h`:
```cpp
#pragma once
#include "hidpp/HidppTypes.h"
#include <QObject>
#include <optional>
#include <span>

namespace logitune {

class ITransport : public QObject {
    Q_OBJECT
public:
    using QObject::QObject;
    virtual ~ITransport() = default;

    /// Send a HID++ request and wait for the matching response.
    /// Returns nullopt on timeout or error.
    virtual std::optional<hidpp::Report> sendRequest(
        const hidpp::Report &request, int timeoutMs = 2000) = 0;

    /// File descriptor for notification polling (QSocketNotifier).
    /// Returns -1 if not applicable.
    virtual int notificationFd() const = 0;

    /// Read one raw report (non-blocking if timeoutMs == 0).
    virtual std::vector<uint8_t> readRawReport(int timeoutMs = 0) = 0;

signals:
    void notificationReceived(const logitune::hidpp::Report &report);
    void deviceDisconnected();
};

} // namespace logitune
```

- [ ] **Step 2: Create IDesktopIntegration**

Create `src/core/interfaces/IDesktopIntegration.h`:
```cpp
#pragma once
#include <QObject>
#include <QString>
#include <QStringList>

namespace logitune {

class IDesktopIntegration : public QObject {
    Q_OBJECT
public:
    using QObject::QObject;
    virtual ~IDesktopIntegration() = default;

    /// Start monitoring active window changes.
    virtual void start() = 0;

    /// Whether this integration is available on the current system.
    virtual bool available() const = 0;

    /// Name of the desktop environment (e.g., "KDE", "GNOME", "Hyprland").
    virtual QString desktopName() const = 0;

    /// List of running compositors detected.
    virtual QStringList detectedCompositors() const = 0;

signals:
    /// Emitted when the focused window changes.
    void activeWindowChanged(const QString &wmClass, const QString &title);
};

} // namespace logitune
```

- [ ] **Step 3: Create IInputInjector**

Create `src/core/interfaces/IInputInjector.h`:
```cpp
#pragma once
#include <QObject>
#include <QString>

namespace logitune {

class IInputInjector : public QObject {
    Q_OBJECT
public:
    using QObject::QObject;
    virtual ~IInputInjector() = default;

    /// Initialize the input device. Returns false on failure.
    virtual bool init() = 0;

    /// Inject a keystroke combo (e.g., "Ctrl+C", "Super+D").
    virtual void injectKeystroke(const QString &combo) = 0;

    /// Inject Ctrl+Scroll (for zoom).
    virtual void injectCtrlScroll(int direction) = 0;

    /// Send a D-Bus method call (service,path,interface,method).
    virtual void sendDBusCall(const QString &spec) = 0;

    /// Launch an application by command.
    virtual void launchApp(const QString &command) = 0;
};

} // namespace logitune
```

- [ ] **Step 4: Create IDevice (device descriptor interface)**

Create `src/core/interfaces/IDevice.h`:
```cpp
#pragma once
#include "hidpp/HidppTypes.h"
#include "ButtonAction.h"
#include <QString>
#include <QList>
#include <QMap>
#include <vector>

namespace logitune {

/// Describes one physical button/control on a device.
struct ControlDescriptor {
    uint16_t controlId;       // HID++ CID (e.g., 0x0050 for left click)
    int buttonIndex;          // Logical index in ButtonModel (0-based)
    QString defaultName;      // e.g., "Left click", "Gesture button"
    QString defaultActionType;// e.g., "default", "gesture-trigger", "smartshift-toggle"
    bool configurable;        // Whether the user can remap this button
};

/// Describes a hotspot position on the device render image.
struct HotspotDescriptor {
    int buttonIndex;
    double xPct;              // X position as fraction of image width
    double yPct;              // Y position as fraction of image height
    QString side;             // "left" or "right" — which side the label goes
    double labelOffsetYPct;   // Optional Y offset for label
};

/// Describes which HID++ features a device supports.
struct FeatureSupport {
    bool battery = false;
    bool adjustableDpi = false;
    bool smartShift = false;
    bool hiResWheel = false;
    bool thumbWheel = false;
    bool reprogControls = false;
    bool gestureV2 = false;
};

/// Abstract device descriptor — one per supported Logitech peripheral.
/// Subclass this to add a new device. Everything device-specific lives here.
class IDevice {
public:
    virtual ~IDevice() = default;

    /// Human-readable device name (e.g., "MX Master 3S").
    virtual QString deviceName() const = 0;

    /// All known product IDs for this device (Bolt, BT, USB).
    virtual std::vector<uint16_t> productIds() const = 0;

    /// Whether a given PID belongs to this device.
    virtual bool matchesPid(uint16_t pid) const = 0;

    /// Button/control descriptors.
    virtual QList<ControlDescriptor> controls() const = 0;

    /// Hotspot positions for the buttons page render.
    virtual QList<HotspotDescriptor> buttonHotspots() const = 0;

    /// Hotspot positions for the point & scroll page render.
    virtual QList<HotspotDescriptor> scrollHotspots() const = 0;

    /// Which features this device supports.
    virtual FeatureSupport features() const = 0;

    /// QRC path to front image (home screen).
    virtual QString frontImagePath() const = 0;

    /// QRC path to side image (buttons & point/scroll pages).
    virtual QString sideImagePath() const = 0;

    /// QRC path to back image (easy-switch page).
    virtual QString backImagePath() const = 0;

    /// Default gesture actions (direction → ButtonAction).
    virtual QMap<QString, ButtonAction> defaultGestures() const = 0;

    /// DPI range.
    virtual int minDpi() const = 0;
    virtual int maxDpi() const = 0;
    virtual int dpiStep() const = 0;

    /// Number of Easy-Switch slots.
    virtual int easySwitchSlots() const = 0;
};

} // namespace logitune
```

- [ ] **Step 5: Build**

```bash
cd /home/mina/repos/logitune/build && cmake --build . 2>&1 | tail -10
```

Headers only — should compile with no changes to CMakeLists.txt (they'll be included by later files).

- [ ] **Step 6: Commit**

```bash
git add -A && git commit -m "feat: add core interfaces — ITransport, IDesktopIntegration, IInputInjector, IDevice"
```

---

## Task 3: Implement MX Master 3S Device Descriptor

**Files:**
- Create: `src/core/devices/MxMaster3sDescriptor.h`
- Create: `src/core/devices/MxMaster3sDescriptor.cpp`
- Create: `src/core/DeviceRegistry.h`
- Create: `src/core/DeviceRegistry.cpp`
- Modify: `src/core/CMakeLists.txt`

- [ ] **Step 1: Create MxMaster3sDescriptor**

Create `src/core/devices/MxMaster3sDescriptor.h`:
```cpp
#pragma once
#include "interfaces/IDevice.h"

namespace logitune {

class MxMaster3sDescriptor : public IDevice {
public:
    QString deviceName() const override;
    std::vector<uint16_t> productIds() const override;
    bool matchesPid(uint16_t pid) const override;
    QList<ControlDescriptor> controls() const override;
    QList<HotspotDescriptor> buttonHotspots() const override;
    QList<HotspotDescriptor> scrollHotspots() const override;
    FeatureSupport features() const override;
    QString frontImagePath() const override;
    QString sideImagePath() const override;
    QString backImagePath() const override;
    QMap<QString, ButtonAction> defaultGestures() const override;
    int minDpi() const override;
    int maxDpi() const override;
    int dpiStep() const override;
    int easySwitchSlots() const override;
};

} // namespace logitune
```

Create `src/core/devices/MxMaster3sDescriptor.cpp` — move all hardcoded MX Master 3S data here:
```cpp
#include "MxMaster3sDescriptor.h"

namespace logitune {

QString MxMaster3sDescriptor::deviceName() const { return QStringLiteral("MX Master 3S"); }

std::vector<uint16_t> MxMaster3sDescriptor::productIds() const {
    return {0xb034, 0xc548}; // Direct BT PID, Bolt receiver PID (device behind receiver)
}

bool MxMaster3sDescriptor::matchesPid(uint16_t pid) const {
    return pid == 0xb034; // Direct device PID only; receiver PID is handled separately
}

QList<ControlDescriptor> MxMaster3sDescriptor::controls() const {
    return {
        {0x0050, 0, "Left click",          "default",           false},
        {0x0051, 1, "Right click",         "default",           false},
        {0x0052, 2, "Middle click",        "default",           true},
        {0x0053, 3, "Back",                "default",           true},
        {0x0056, 4, "Forward",             "default",           true},
        {0x00C3, 5, "Gesture button",      "gesture-trigger",   true},
        {0x00C4, 6, "Shift wheel mode",    "smartshift-toggle", true},
        {0x0000, 7, "Thumb wheel",         "default",           true},  // virtual — no CID
    };
}

QList<HotspotDescriptor> MxMaster3sDescriptor::buttonHotspots() const {
    return {
        {2, 0.71, 0.15, "right", 0.0},   // Middle button
        {6, 0.81, 0.34, "right", 0.0},   // Top / ModeShift
        {7, 0.55, 0.515, "right", 0.0},  // ThumbWheel
        {4, 0.35, 0.43, "left",  0.0},   // Forward
        {3, 0.45, 0.60, "left",  0.20},  // Back (label offset down)
        {5, 0.08, 0.58, "left",  0.0},   // Gesture
    };
}

QList<HotspotDescriptor> MxMaster3sDescriptor::scrollHotspots() const {
    return {
        {-1, 0.73, 0.16, "right", 0.0},  // Scroll wheel
        {-2, 0.55, 0.51, "left",  0.0},  // Thumb wheel
        {-3, 0.83, 0.54, "right", 0.0},  // Pointer speed
    };
}

FeatureSupport MxMaster3sDescriptor::features() const {
    return {
        .battery = true,
        .adjustableDpi = true,
        .smartShift = true,
        .hiResWheel = true,
        .thumbWheel = true,
        .reprogControls = true,
        .gestureV2 = false,  // We use ReprogControls RawXY instead
    };
}

QString MxMaster3sDescriptor::frontImagePath() const {
    return QStringLiteral("qrc:/Logitune/qml/assets/mx-master-3s.png");
}
QString MxMaster3sDescriptor::sideImagePath() const {
    return QStringLiteral("qrc:/Logitune/qml/assets/mx-master-3s-side.png");
}
QString MxMaster3sDescriptor::backImagePath() const {
    return QStringLiteral("qrc:/Logitune/qml/assets/mx-master-3s-back.png");
}

QMap<QString, ButtonAction> MxMaster3sDescriptor::defaultGestures() const {
    return {
        {"up",    {ButtonAction::Default, ""}},
        {"down",  {ButtonAction::Keystroke, "Super+D"}},
        {"left",  {ButtonAction::Keystroke, "Ctrl+Super+Left"}},
        {"right", {ButtonAction::Keystroke, "Ctrl+Super+Right"}},
        {"click", {ButtonAction::Keystroke, "Super+W"}},
    };
}

int MxMaster3sDescriptor::minDpi() const { return 200; }
int MxMaster3sDescriptor::maxDpi() const { return 8000; }
int MxMaster3sDescriptor::dpiStep() const { return 50; }
int MxMaster3sDescriptor::easySwitchSlots() const { return 3; }

} // namespace logitune
```

- [ ] **Step 2: Create DeviceRegistry**

Create `src/core/DeviceRegistry.h`:
```cpp
#pragma once
#include "interfaces/IDevice.h"
#include <memory>
#include <vector>

namespace logitune {

/// Registry of known device descriptors. Adding a new device = register it here.
class DeviceRegistry {
public:
    DeviceRegistry();

    /// Find the device descriptor matching a given product ID.
    /// Returns nullptr if no match.
    const IDevice* findByPid(uint16_t pid) const;

    /// Register a device descriptor. Takes ownership.
    void registerDevice(std::unique_ptr<IDevice> device);

    /// All registered devices.
    const std::vector<std::unique_ptr<IDevice>>& devices() const;

private:
    std::vector<std::unique_ptr<IDevice>> m_devices;
};

} // namespace logitune
```

Create `src/core/DeviceRegistry.cpp`:
```cpp
#include "DeviceRegistry.h"
#include "devices/MxMaster3sDescriptor.h"

namespace logitune {

DeviceRegistry::DeviceRegistry() {
    // Register all known devices
    registerDevice(std::make_unique<MxMaster3sDescriptor>());
}

const IDevice* DeviceRegistry::findByPid(uint16_t pid) const {
    for (const auto &dev : m_devices) {
        if (dev->matchesPid(pid))
            return dev.get();
    }
    return nullptr;
}

void DeviceRegistry::registerDevice(std::unique_ptr<IDevice> device) {
    m_devices.push_back(std::move(device));
}

const std::vector<std::unique_ptr<IDevice>>& DeviceRegistry::devices() const {
    return m_devices;
}

} // namespace logitune
```

- [ ] **Step 3: Update CMakeLists.txt**

In `src/core/CMakeLists.txt`, add to sources:
```cmake
DeviceRegistry.cpp
devices/MxMaster3sDescriptor.cpp
```

- [ ] **Step 4: Write test for DeviceRegistry**

Create `tests/test_device_registry.cpp`:
```cpp
#include <gtest/gtest.h>
#include "DeviceRegistry.h"

using namespace logitune;

TEST(DeviceRegistry, FindsMxMaster3sByPid) {
    DeviceRegistry reg;
    auto *dev = reg.findByPid(0xb034);
    ASSERT_NE(dev, nullptr);
    EXPECT_EQ(dev->deviceName(), "MX Master 3S");
}

TEST(DeviceRegistry, ReturnsNullForUnknownPid) {
    DeviceRegistry reg;
    EXPECT_EQ(reg.findByPid(0x0000), nullptr);
}

TEST(DeviceRegistry, ControlsHaveExpectedCids) {
    DeviceRegistry reg;
    auto *dev = reg.findByPid(0xb034);
    auto controls = dev->controls();
    EXPECT_GE(controls.size(), 7);
    EXPECT_EQ(controls[0].controlId, 0x0050); // Left click
    EXPECT_EQ(controls[5].controlId, 0x00C3); // Gesture button
}

TEST(DeviceRegistry, DefaultGesturesPresent) {
    DeviceRegistry reg;
    auto *dev = reg.findByPid(0xb034);
    auto gestures = dev->defaultGestures();
    EXPECT_TRUE(gestures.contains("down"));
    EXPECT_EQ(gestures["down"].payload, "Super+D");
}
```

Add to `tests/CMakeLists.txt`:
```cmake
test_device_registry.cpp
```

- [ ] **Step 5: Build and test**

```bash
cd /home/mina/repos/logitune/build && cmake .. && cmake --build . 2>&1 | tail -10
./tests/logitune-tests --gtest_filter="DeviceRegistry*"
```

- [ ] **Step 6: Commit**

```bash
git add -A && git commit -m "feat: add DeviceRegistry and MxMaster3sDescriptor — device-specific data in one place"
```

---

## Task 4: Implement Concrete Interface Adapters

**Files:**
- Create: `src/core/desktop/KDeDesktop.h`
- Create: `src/core/desktop/KDeDesktop.cpp`
- Create: `src/core/desktop/GenericDesktop.h`
- Create: `src/core/desktop/GenericDesktop.cpp`
- Create: `src/core/input/UinputInjector.h`
- Create: `src/core/input/UinputInjector.cpp`
- Modify: `src/core/CMakeLists.txt`
- Modify: `src/app/WindowTracker.h` — delete, replaced by KDeDesktop

- [ ] **Step 1: Create KDeDesktop (refactor from KWinWindowTracker)**

Create `src/core/desktop/KDeDesktop.h`:
```cpp
#pragma once
#include "interfaces/IDesktopIntegration.h"

namespace logitune {

class KDeDesktop : public IDesktopIntegration {
    Q_OBJECT
public:
    explicit KDeDesktop(QObject *parent = nullptr);

    void start() override;
    bool available() const override;
    QString desktopName() const override;
    QStringList detectedCompositors() const override;

private slots:
    void onActiveClientChanged();

private:
    bool m_available = false;
};

} // namespace logitune
```

Create `src/core/desktop/KDeDesktop.cpp` — move the implementation from `WindowTracker.cpp`:
```cpp
#include "KDeDesktop.h"
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusReply>
#include <QProcessEnvironment>

namespace logitune {

KDeDesktop::KDeDesktop(QObject *parent) : IDesktopIntegration(parent) {
    auto env = QProcessEnvironment::systemEnvironment();
    QString desktop = env.value("XDG_CURRENT_DESKTOP", "").toLower();
    m_available = desktop.contains("kde") || desktop.contains("plasma");
}

void KDeDesktop::start() {
    if (!m_available) return;

    QDBusConnection::sessionBus().connect(
        "org.kde.KWin", "/KWin", "org.kde.KWin",
        "activeClientChanged", this, SLOT(onActiveClientChanged()));
}

bool KDeDesktop::available() const { return m_available; }
QString KDeDesktop::desktopName() const { return QStringLiteral("KDE Plasma"); }

QStringList KDeDesktop::detectedCompositors() const {
    QStringList result;
    if (m_available) result << "KWin";
    return result;
}

void KDeDesktop::onActiveClientChanged() {
    QDBusInterface kwin("org.kde.KWin", "/KWin", "org.kde.KWin");
    QDBusReply<QVariantMap> reply = kwin.call("queryWindowInfo",
        kwin.call("activeClient").arguments().value(0));
    if (!reply.isValid()) return;

    auto map = reply.value();
    QString wmClass = map.value("resourceClass").toString();
    QString title = map.value("caption").toString();
    if (!wmClass.isEmpty())
        emit activeWindowChanged(wmClass, title);
}

} // namespace logitune
```

- [ ] **Step 2: Create GenericDesktop (no-op fallback)**

Create `src/core/desktop/GenericDesktop.h`:
```cpp
#pragma once
#include "interfaces/IDesktopIntegration.h"

namespace logitune {

class GenericDesktop : public IDesktopIntegration {
    Q_OBJECT
public:
    using IDesktopIntegration::IDesktopIntegration;

    void start() override {}
    bool available() const override { return true; }
    QString desktopName() const override { return QStringLiteral("Generic"); }
    QStringList detectedCompositors() const override { return {}; }
};

} // namespace logitune
```

Create `src/core/desktop/GenericDesktop.cpp`:
```cpp
#include "GenericDesktop.h"
// No-op implementation — all inline in header
```

- [ ] **Step 3: Create UinputInjector (extract from ActionExecutor)**

Create `src/core/input/UinputInjector.h`:
```cpp
#pragma once
#include "interfaces/IInputInjector.h"

namespace logitune {

class UinputInjector : public IInputInjector {
    Q_OBJECT
public:
    explicit UinputInjector(QObject *parent = nullptr);
    ~UinputInjector();

    bool init() override;
    void injectKeystroke(const QString &combo) override;
    void injectCtrlScroll(int direction) override;
    void sendDBusCall(const QString &spec) override;
    void launchApp(const QString &command) override;

    /// Parse a keystroke combo string into Linux keycodes.
    static std::vector<int> parseKeystroke(const QString &combo);

private:
    int m_uinputFd = -1;
    void emitKey(int code, int value);
    void emitSync();
};

} // namespace logitune
```

Create `src/core/input/UinputInjector.cpp` — move the uinput code from `ActionExecutor.cpp`:
- `initUinput()` → `init()`
- `injectKeystroke()` — move the implementation
- `injectCtrlScroll()` — move the implementation
- `parseKeystroke()` — move the static helper
- `executeDBusCall()` → `sendDBusCall()`
- `launchApp()` — move as-is
- `emitKey()`, `emitSync()` — move private helpers

The actual implementation bodies are copied from `ActionExecutor.cpp` lines 38-220.

- [ ] **Step 4: Slim down ActionExecutor**

`ActionExecutor` now becomes a thin coordinator that owns an `IInputInjector*` and a `GestureDetector`:

In `src/core/ActionExecutor.h`:
```cpp
#pragma once
#include "ButtonAction.h"
#include "interfaces/IInputInjector.h"
#include <QObject>
#include <QString>

namespace logitune {

enum class GestureDirection { None, Up, Down, Left, Right, Click };

class GestureDetector {
public:
    void addDelta(int dx, int dy);
    GestureDirection resolve() const;
    void reset();
private:
    int m_dx = 0, m_dy = 0;
    static constexpr int kThreshold = 50;
};

class ActionExecutor : public QObject {
    Q_OBJECT
public:
    explicit ActionExecutor(IInputInjector *injector, QObject *parent = nullptr);

    void executeKeystroke(const QString &combo);
    void executeCtrlScroll(int direction);
    void executeDBusCall(const QString &spec);
    void launchApp(const QString &command);

    GestureDetector &gestureDetector();

private:
    IInputInjector *m_injector;  // not owned
    GestureDetector m_gestureDetector;
};

} // namespace logitune
```

`ActionExecutor.cpp` becomes ~30 lines: each method just delegates to `m_injector`.

- [ ] **Step 5: Update CMakeLists.txt**

In `src/core/CMakeLists.txt`, add:
```cmake
desktop/KDeDesktop.cpp
desktop/GenericDesktop.cpp
input/UinputInjector.cpp
```

Remove `Placeholder.cpp` if not already done.

- [ ] **Step 6: Delete old WindowTracker**

Delete `src/app/WindowTracker.h` and `src/app/WindowTracker.cpp`. Remove from `src/app/CMakeLists.txt`.

- [ ] **Step 7: Build and run tests**

```bash
cmake .. && cmake --build . 2>&1 | tail -20
./tests/logitune-tests
```

Fix any compilation errors from the refactored ActionExecutor. Tests for `parseKeystroke` and `GestureDetector` should still pass against the new locations.

- [ ] **Step 8: Commit**

```bash
git add -A && git commit -m "feat: implement IDesktopIntegration (KDE + Generic), IInputInjector (uinput), slim ActionExecutor"
```

---

## Task 5: Create AppController — Extract Business Logic from main.cpp

**Files:**
- Create: `src/app/AppController.h`
- Create: `src/app/AppController.cpp`
- Modify: `src/app/main.cpp`
- Modify: `src/app/CMakeLists.txt`
- Modify: `src/app/models/DeviceModel.h`
- Modify: `src/app/models/DeviceModel.cpp`

This is the biggest task. All the lambda logic in main.cpp moves into `AppController`.

- [ ] **Step 1: Create AppController header**

Create `src/app/AppController.h`:
```cpp
#pragma once
#include <QObject>
#include <QMap>
#include <memory>

namespace logitune {

class DeviceManager;
class ProfileEngine;
class ActionExecutor;
class IDesktopIntegration;
class IInputInjector;
class DeviceRegistry;
class ButtonModel;
class ActionModel;
class ProfileModel;
class DeviceModel;
class IDevice;

class AppController : public QObject {
    Q_OBJECT
public:
    explicit AppController(QObject *parent = nullptr);
    ~AppController();

    /// Initialize all subsystems. Call once after construction.
    void init();

    // Accessors for QML singleton registration
    DeviceManager *deviceManager() const;
    DeviceModel *deviceModel() const;
    ButtonModel *buttonModel() const;
    ActionModel *actionModel() const;
    ProfileModel *profileModel() const;
    ProfileEngine *profileEngine() const;
    IDesktopIntegration *desktop() const;

private slots:
    void onDeviceSetupComplete();
    void onButtonModelChanged(const QModelIndex &topLeft, const QModelIndex &, const QVector<int> &);
    void onDivertedButtonPressed(uint16_t controlId, bool pressed);
    void onGestureRawXY(int16_t dx, int16_t dy);
    void onThumbWheelRotation(int delta);
    void onDeviceStateChanged();

private:
    void wireSignals();
    void saveCurrentProfile();
    void restoreProfileToDevice(const struct Profile &profile);
    QString buttonActionToName(const struct ButtonAction &ba) const;

    // Subsystems (owned)
    std::unique_ptr<DeviceRegistry> m_registry;
    std::unique_ptr<DeviceManager> m_deviceManager;
    std::unique_ptr<ProfileEngine> m_profileEngine;
    std::unique_ptr<IInputInjector> m_injector;
    std::unique_ptr<ActionExecutor> m_actionExecutor;
    std::unique_ptr<IDesktopIntegration> m_desktop;

    // QML models (owned)
    std::unique_ptr<DeviceModel> m_deviceModel;
    std::unique_ptr<ButtonModel> m_buttonModel;
    std::unique_ptr<ActionModel> m_actionModel;
    std::unique_ptr<ProfileModel> m_profileModel;

    // Gesture state
    int m_gestureTotalDx = 0;
    int m_gestureTotalDy = 0;
    bool m_gestureActive = false;
    static constexpr int kGestureThreshold = 50;

    // Thumb wheel accumulator
    int m_thumbAccum = 0;
    static constexpr int kThumbThreshold = 15;

    // Profile save guard
    bool m_savingProfile = false;

    // Current device descriptor
    const IDevice *m_currentDevice = nullptr;
};

} // namespace logitune
```

- [ ] **Step 2: Create AppController implementation**

Create `src/app/AppController.cpp`. This is a large file (~350 lines) that contains all the logic currently in main.cpp lambdas. The key methods:

- `init()` — creates all subsystems, detects desktop environment, wires signals
- `wireSignals()` — all QObject::connect calls (currently scattered through main.cpp)
- `onDeviceSetupComplete()` — the 110-line profile bootstrap (currently main.cpp lines 201-312)
- `onButtonModelChanged()` — button divert logic (currently main.cpp lines 158-167)
- `onDivertedButtonPressed()` — gesture + action dispatch (currently main.cpp lines 346-412)
- `onGestureRawXY()` — delta accumulation (currently main.cpp lines 338-344)
- `onThumbWheelRotation()` — thumb wheel logic (currently main.cpp lines 417-435)
- `saveCurrentProfile()` — profile save (currently main.cpp lines 115-142)
- `onDeviceStateChanged()` — triggers `saveCurrentProfile()` (replaces 6 lambda connections)
- `restoreProfileToDevice()` — applies profile to hardware (extracted from onDeviceSetupComplete)
- `buttonActionToName()` — reverse lookup (currently main.cpp lines 74-92)

The body of each method is a direct move from main.cpp with `[&]` captures replaced by `this->m_` member access. Static locals become member variables.

- [ ] **Step 3: Slim down main.cpp to ~80 lines**

```cpp
#include <QApplication>
#include <QQmlApplicationEngine>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QQuickWindow>
#include <signal.h>
#include "AppController.h"
#include "DeviceManager.h"
#include "models/DeviceModel.h"
#include "models/ButtonModel.h"
#include "models/ActionModel.h"
#include "models/ProfileModel.h"

int main(int argc, char *argv[])
{
    signal(SIGPIPE, SIG_IGN);

    QApplication app(argc, argv);
    app.setOrganizationName("Logitune");
    app.setApplicationName("Logitune");

    // Detect dark mode from system palette
    QColor bg = app.palette().window().color();
    double lum = bg.redF() * 0.299 + bg.greenF() * 0.587 + bg.blueF() * 0.114;
    bool isDark = lum < 0.5;

    // Create and initialize the application controller
    logitune::AppController controller;
    controller.init();

    // QML engine + singleton registration
    QQmlApplicationEngine engine;

    qmlRegisterSingletonInstance("Logitune", 1, 0, "DeviceModel",  controller.deviceModel());
    qmlRegisterSingletonInstance("Logitune", 1, 0, "ButtonModel",  controller.buttonModel());
    qmlRegisterSingletonInstance("Logitune", 1, 0, "ActionModel",  controller.actionModel());
    qmlRegisterSingletonInstance("Logitune", 1, 0, "ProfileModel", controller.profileModel());

    engine.load(QUrl(QStringLiteral("qrc:/Logitune/qml/Main.qml")));
    if (engine.rootObjects().isEmpty()) return -1;

    // Set dark mode
    if (auto *theme = engine.singletonInstance<QObject*>("Logitune", "Theme"))
        theme->setProperty("dark", isDark);

    // System tray
    QSystemTrayIcon trayIcon;
    trayIcon.setIcon(QIcon::fromTheme("input-mouse"));
    trayIcon.setToolTip("Logitune");

    QMenu trayMenu;
    QAction *showAction = trayMenu.addAction("Show Logitune");
    trayMenu.addSeparator();
    QAction *batteryAction = trayMenu.addAction("Battery: ---%");
    batteryAction->setEnabled(false);
    trayMenu.addSeparator();
    QAction *quitAction = trayMenu.addAction("Quit");
    trayIcon.setContextMenu(&trayMenu);
    trayIcon.show();

    auto showWindow = [&engine]() {
        for (auto *obj : engine.rootObjects()) {
            if (auto *w = qobject_cast<QQuickWindow*>(obj)) {
                w->show(); w->raise(); w->requestActivate();
            }
        }
    };

    QObject::connect(showAction, &QAction::triggered, showWindow);
    QObject::connect(quitAction, &QAction::triggered, &app, &QApplication::quit);
    QObject::connect(&trayIcon, &QSystemTrayIcon::activated,
        [&showWindow](QSystemTrayIcon::ActivationReason r) {
            if (r == QSystemTrayIcon::Trigger) showWindow();
        });

    // Update tray battery text
    QObject::connect(controller.deviceManager(), &logitune::DeviceManager::batteryLevelChanged,
        [batteryAction, dm = controller.deviceManager()]() {
            QString text = QString("Battery: %1%").arg(dm->batteryLevel());
            if (dm->batteryCharging()) text += " (charging)";
            batteryAction->setText(text);
        });

    // Update tray tooltip with device name
    QObject::connect(controller.deviceManager(), &logitune::DeviceManager::deviceNameChanged,
        [&trayIcon, dm = controller.deviceManager()]() {
            trayIcon.setToolTip("Logitune - " + dm->deviceName());
        });

    return app.exec();
}
```

- [ ] **Step 4: Fix DeviceModel — remove stale caches**

In `DeviceModel.h/.cpp`, remove `m_currentDPI`, `m_smartShiftEnabled`, `m_smartShiftThreshold` cached copies. All property reads delegate directly to `m_dm->currentDPI()` etc. Remove `setCurrentDPI()`, `setSmartShiftState()` integration setters. The `activeProfileName` can stay as local state since DeviceManager doesn't track it.

- [ ] **Step 5: Update CMakeLists.txt**

In `src/app/CMakeLists.txt`, add `AppController.cpp`. Remove `WindowTracker.cpp`.

- [ ] **Step 6: Build and test**

```bash
cmake .. && cmake --build . 2>&1 | tail -20
./tests/logitune-tests
```

Then launch the app to verify everything still works:
```bash
pkill -9 logitune; nohup ./src/app/logitune > /tmp/logitune.log 2>&1 &
sleep 3 && tail -10 /tmp/logitune.log
```

- [ ] **Step 7: Commit**

```bash
git add -A && git commit -m "refactor: extract AppController from main.cpp — proper separation of concerns"
```

---

## Task 6: Wire DeviceRegistry into DeviceManager

**Files:**
- Modify: `src/core/DeviceManager.h`
- Modify: `src/core/DeviceManager.cpp`
- Modify: `src/app/AppController.cpp`

Replace all hardcoded PIDs, CID arrays, and `isDirectDevice()` / `isReceiver()` with `DeviceRegistry` lookups.

- [ ] **Step 1: Add DeviceRegistry dependency to DeviceManager**

In `DeviceManager.h`, add:
```cpp
class DeviceRegistry;
class IDevice;
```

Add to constructor: `DeviceManager(DeviceRegistry *registry, QObject *parent = nullptr);`

Add member: `DeviceRegistry *m_registry;` and `const IDevice *m_activeDevice = nullptr;`

Add public accessor: `const IDevice* activeDevice() const;`

- [ ] **Step 2: Replace hardcoded PID checks**

In `DeviceManager.cpp`:
- Replace `isReceiver(pid)` with check for known receiver PIDs (keep as static — receivers are protocol-level, not device-level)
- Replace `isDirectDevice(pid)` with `m_registry->findByPid(pid) != nullptr`
- In `enumerateAndSetup()`, after getting the device name from HID++, look up `m_activeDevice = m_registry->findByPid(m_devicePid)` (or by name if the PID is the receiver's)
- Replace all `kAllButtons` / `kButtonCids` arrays with `m_activeDevice->controls()`

- [ ] **Step 3: Replace CID arrays in AppController**

In `AppController.cpp`, replace the 4 duplicated CID tables with reads from `m_currentDevice->controls()`. The `onDeviceSetupComplete` slot sets `m_currentDevice = m_deviceManager->activeDevice()`.

- [ ] **Step 4: Build and test**

```bash
cmake --build . 2>&1 | tail -20
./tests/logitune-tests
pkill -9 logitune; nohup ./src/app/logitune > /tmp/logitune.log 2>&1 &
sleep 3 && grep -E "battery|profile applied|connected" /tmp/logitune.log
```

- [ ] **Step 5: Commit**

```bash
git add -A && git commit -m "refactor: wire DeviceRegistry into DeviceManager — no more hardcoded PIDs/CIDs"
```

---

## Task 7: Expose Device Descriptor to QML

**Files:**
- Modify: `src/app/models/DeviceModel.h`
- Modify: `src/app/models/DeviceModel.cpp`
- Modify: `src/app/qml/components/DeviceRender.qml`
- Modify: `src/app/qml/pages/ButtonsPage.qml`
- Modify: `src/app/qml/pages/PointScrollPage.qml`
- Modify: `src/app/qml/pages/EasySwitchPage.qml`

Replace hardcoded image paths, hotspot positions, and button data in QML with reads from `DeviceModel` which delegates to the `IDevice` descriptor.

- [ ] **Step 1: Add device descriptor properties to DeviceModel**

In `DeviceModel.h`, add Q_PROPERTYs:
```cpp
Q_PROPERTY(QString frontImage READ frontImage NOTIFY deviceConnectedChanged)
Q_PROPERTY(QString sideImage READ sideImage NOTIFY deviceConnectedChanged)
Q_PROPERTY(QString backImage READ backImage NOTIFY deviceConnectedChanged)
Q_PROPERTY(QVariantList buttonHotspots READ buttonHotspots NOTIFY deviceConnectedChanged)
Q_PROPERTY(QVariantList scrollHotspots READ scrollHotspots NOTIFY deviceConnectedChanged)
Q_PROPERTY(QVariantList controlDescriptors READ controlDescriptors NOTIFY deviceConnectedChanged)
Q_PROPERTY(int easySwitchSlots READ easySwitchSlots NOTIFY deviceConnectedChanged)
```

Each reads from `m_dm->activeDevice()` and returns QML-friendly types (QVariantList of QVariantMap).

- [ ] **Step 2: Update QML to use dynamic data**

In `ButtonsPage.qml`, replace the hardcoded `calloutData` array with a binding to `DeviceModel.buttonHotspots`. Same for image paths in `DeviceRender` instances.

In `PointScrollPage.qml`, replace hardcoded hotspot positions with `DeviceModel.scrollHotspots`.

In `EasySwitchPage.qml`, replace hardcoded 3 slots with `DeviceModel.easySwitchSlots`.

- [ ] **Step 3: Build, test, verify UI**

```bash
cmake --build . && pkill -9 logitune; nohup ./src/app/logitune > /tmp/logitune.log 2>&1 &
sleep 3 && tail -5 /tmp/logitune.log
```

Navigate to each page and verify everything renders correctly.

- [ ] **Step 4: Commit**

```bash
git add -A && git commit -m "feat: QML reads device data from descriptor — no more hardcoded hotspots/images"
```

---

## Task 8: Final Cleanup

**Files:**
- Various minor cleanups across the codebase

- [ ] **Step 1: Remove unused Qt6::Concurrent from CMakeLists.txt**

In `src/core/CMakeLists.txt`, remove `Qt6::Concurrent` from `target_link_libraries`.

- [ ] **Step 2: Update SettingsPage firmware/serial**

In `SettingsPage.qml`, replace hardcoded "12.00.11" with "Unknown" and replace "XXXX-XXXX" with a binding to `DeviceModel.deviceSerial` (already available via `DeviceManager::deviceSerial()`). Add a `deviceSerial` property to `DeviceModel` if not already there.

- [ ] **Step 3: Clean up ThemeBridge**

In `main.cpp`, remove the `qmlRegisterSingletonType` for `ThemeBridge` — it's no longer used since Theme.dark is set via `singletonInstance`.

- [ ] **Step 4: Build, full test suite, manual smoke test**

```bash
cmake .. && cmake --build . 2>&1 | tail -10
./tests/logitune-tests
pkill -9 logitune; nohup ./src/app/logitune > /tmp/logitune.log 2>&1 &
sleep 3 && tail -10 /tmp/logitune.log
```

Verify: mouse connects, DPI/SmartShift/scroll work, gestures fire, profile saves, dark theme works, all pages render.

- [ ] **Step 5: Commit**

```bash
git add -A && git commit -m "chore: final cleanup — remove dead deps, fix settings page, remove ThemeBridge"
```

---

## Architecture After Refactor

```
                                    ┌─────────────┐
                                    │   QML UI     │
                                    │  (pages,     │
                                    │  components) │
                                    └──────┬───────┘
                                           │ binds to
                                    ┌──────▼───────┐
                                    │  Qt Models   │
                                    │ DeviceModel  │
                                    │ ButtonModel  │
                                    │ ActionModel  │
                                    │ ProfileModel │
                                    └──────┬───────┘
                                           │ owned by
                                    ┌──────▼───────┐
                                    │AppController │ ← orchestrator
                                    └──┬──┬──┬──┬──┘
                                       │  │  │  │
            ┌──────────────────────────┘  │  │  └──────────────────────┐
            │                             │  │                         │
    ┌───────▼───────┐           ┌────────▼──▼────────┐        ┌──────▼──────┐
    │DeviceManager  │           │  ProfileEngine     │        │IDesktop     │
    │(HID++ lifecycle│          │  (INI persistence) │        │Integration  │
    │ udev, notify) │           └────────────────────┘        ├─────────────┤
    └───────┬───────┘                                         │ KDeDesktop  │
            │ uses                                            │ GenericDE   │
    ┌───────▼───────┐     ┌──────────────┐                   │ (GnomeDE)   │
    │DeviceRegistry │     │IInputInjector│                   │ (HyprDE)    │
    │               │     ├──────────────┤                   └─────────────┘
    │ ┌───────────┐ │     │UinputInjector│
    │ │MxMaster3s │ │     │ (future:     │
    │ │Descriptor │ │     │  libei,      │
    │ ├───────────┤ │     │  wlroots)    │
    │ │(MxKeys)   │ │     └──────────────┘
    │ │(MxAnyw.)  │ │
    │ └───────────┘ │
    └───────────────┘

    ┌────────────────────────────────┐
    │     hidpp/ (protocol layer)    │
    │ HidrawDevice → Transport →     │
    │ FeatureDispatcher → features/* │
    └────────────────────────────────┘
```

**Adding a new device:** Create `FooDescriptor.cpp` implementing `IDevice`, register in `DeviceRegistry` constructor. Done.

**Adding a new DE:** Create `GnomeDesktop.cpp` implementing `IDesktopIntegration`, add detection logic in `AppController::init()`. Done.

**Adding a new compositor:** Same as DE — the `IDesktopIntegration` interface covers both.

**Adding a new input method:** Create `LibeiInjector.cpp` implementing `IInputInjector`, select in `AppController::init()` based on compositor detection. Done.
