# MVVM Refactor — Clean Signal Separation Across All Models

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Every model has clean separation between user-initiated actions and programmatic updates. No signal cascades, no guards, no hacks.

**Architecture:** Each model has two paths: (1) user intent signals that AppController handles for business logic, and (2) programmatic bulk-load methods using `beginResetModel`/`endResetModel` that update display silently. AppController is the only bridge between user intent, profile cache, and hardware.

**Tech Stack:** C++20, Qt 6 Quick, existing codebase

---

## The Pattern

Every model follows the same contract:

```
USER PATH:    QML click → Model.setFoo()    → emits fooChangeRequested() → AppController handles
PROGRAM PATH: AppController → Model.loadFromProfile(data) → beginResetModel/endResetModel → QML rebinds
```

AppController NEVER calls `setFoo()` — only `loadFromProfile()`.
QML NEVER calls `loadFromProfile()` — only `setFoo()`.
These two paths never cross.

---

## Current State

| Model | User path | Program path | Status |
|-------|-----------|-------------|--------|
| ButtonModel | `setAction()` → `userActionChanged` | `loadFromProfile()` → `modelReset` | ✅ Done |
| DeviceModel | `setDPI()` → `dpiChangeRequested` etc. | `setDisplayValues()` → individual emits | ⚠️ Half done — `setDisplayValues` emits per-field signals that can cascade |
| ProfileModel | `setActiveByIndex()` → `profileSwitched` | `setActiveByWmClass()` → dataChanged only | ⚠️ Half done — `setActiveByWmClass` still calls into same activeIndex, gestureChanged lambda still fires saveCurrentProfile |
| ProfileEngine | N/A | `setDisplayProfile` / `setHardwareProfile` | ⚠️ Works but `displayProfileChanged` triggers `onDisplayProfileChanged` which has side effects |

---

## File Structure

### Files to modify

```
src/app/models/DeviceModel.h/.cpp      — add loadFromProfile(), stop forwarding DeviceManager signals
src/app/models/ProfileModel.h/.cpp     — clean user vs programmatic paths
src/app/AppController.h/.cpp           — simplify signal wiring, remove cascading paths
src/core/ProfileEngine.h/.cpp          — minor: remove unused hardwareProfileChanged listener
```

### No new files needed

---

## Task 1: Clean DeviceModel — Proper loadFromProfile

**Files:**
- Modify: `src/app/models/DeviceModel.h`
- Modify: `src/app/models/DeviceModel.cpp`

The problem: `setDisplayValues()` emits 5 individual signals (`currentDPIChanged`, `smartShiftEnabledChanged`, etc.). These signals are also forwarded from DeviceManager (hardware), so programmatic updates look like hardware changes to any listener. Also, DeviceModel forwards DeviceManager signals directly — when `applyProfileToHardware` calls `DeviceManager.setDPI()`, DeviceManager emits `currentDPIChanged`, which DeviceModel forwards, which QML picks up as a display change even if a different tab is shown.

- [ ] **Step 1: Remove DeviceManager signal forwarding**

In `DeviceModel::setDeviceManager()`, remove all `connect(dm, &DeviceManager::*Changed, this, ...)` calls EXCEPT:
- `deviceConnectedChanged` — needed for device connect/disconnect
- `deviceNameChanged` — device name display
- `batteryLevelChanged` / `batteryChargingChanged` — battery chip
- `connectionTypeChanged` — connection type display

Remove forwarding for: `currentDPIChanged`, `smartShiftChanged`, `scrollConfigChanged`. These are now driven by `setDisplayValues()` / `loadFromProfile()` only.

- [ ] **Step 2: Replace setDisplayValues with loadSettingsFromProfile**

Rename `setDisplayValues(dpi, smartShift, ..., thumbWheelMode)` to `loadSettingsFromProfile(const Profile &p)`.

The new method:
```cpp
void DeviceModel::loadSettingsFromProfile(int dpi, bool ssEnabled, int ssThreshold,
                                           bool hiRes, bool invert, const QString &twMode)
{
    m_displayDpi = dpi;
    m_displaySmartShiftEnabled = ssEnabled;
    m_displaySmartShiftThreshold = ssThreshold;
    m_displayScrollHiRes = hiRes;
    m_displayScrollInvert = invert;
    m_displayThumbWheelMode = twMode;
    m_hasDisplayValues = true;

    // Single batch notification — QML rebinds all properties at once
    emit settingsReloaded();
}
```

Add a new signal `settingsReloaded()` and connect ALL property NOTIFYs for DPI/SmartShift/scroll/thumbWheel to it:

```cpp
Q_PROPERTY(int currentDPI READ currentDPI NOTIFY settingsReloaded)
Q_PROPERTY(bool smartShiftEnabled READ smartShiftEnabled NOTIFY settingsReloaded)
Q_PROPERTY(int smartShiftThreshold READ smartShiftThreshold NOTIFY settingsReloaded)
Q_PROPERTY(bool scrollHiRes READ scrollHiRes NOTIFY settingsReloaded)
Q_PROPERTY(bool scrollInvert READ scrollInvert NOTIFY settingsReloaded)
Q_PROPERTY(QString thumbWheelMode READ thumbWheelMode NOTIFY settingsReloaded)
```

This way: one signal, one QML rebind cycle, no per-field cascades.

- [ ] **Step 3: User setters remain as-is**

`setDPI()`, `setSmartShift()`, `setScrollConfig()`, `setThumbWheelMode()` still emit `dpiChangeRequested`, etc. These are only called from QML user interactions.

- [ ] **Step 4: Update AppController to use loadSettingsFromProfile**

Replace all `setDisplayValues(...)` / `pushDisplayValues(...)` calls with the new method.

- [ ] **Step 5: Build and test**

```bash
cd /home/mina/repos/logitune/build && cmake --build . 2>&1
timeout 30 ./tests/logitune-tests 2>&1
```

- [ ] **Step 6: Commit**

```bash
git add -A && git commit -m "refactor: DeviceModel uses single settingsReloaded signal — no per-field cascades from programmatic updates"
```

---

## Task 2: Clean ProfileModel — User Click vs Hardware Highlight

**Files:**
- Modify: `src/app/models/ProfileModel.h`
- Modify: `src/app/models/ProfileModel.cpp`

The problem: `setActiveByWmClass()` updates the visual highlight and sometimes triggers `profileSwitched` through shared code paths. `setActiveByIndex()` is called from both QML (user click) and programmatic paths.

- [ ] **Step 1: Add hwActiveIndex separate from displayActiveIndex**

ProfileModel should track TWO indices:
- `m_displayIndex` — which tab the user is viewing (set by user tab click)
- `m_hwActiveIndex` — which profile is on the hardware (set by window focus)

QML chips show both: the display tab has accent color, the hardware-active tab has a subtle indicator (e.g., a small dot).

```cpp
// In ProfileModel.h
int m_displayIndex = 0;   // user's selected tab
int m_hwActiveIndex = 0;  // what's on the hardware
```

- [ ] **Step 2: Split setActiveByIndex into two methods**

```cpp
// Called by QML chip click only — emits profileSwitched for AppController
Q_INVOKABLE void selectTab(int index);

// Called by AppController on window focus — visual indicator only, no profileSwitched
void setHwActiveIndex(int index);

// Called by AppController on window focus — finds index by wmClass
void setHwActiveByWmClass(const QString &wmClass);
```

Remove `setActiveByIndex` and `setActiveByWmClass`.

- [ ] **Step 3: selectTab emits profileSwitched**

```cpp
void ProfileModel::selectTab(int index) {
    if (index < 0 || index >= m_profiles.size()) return;
    if (m_displayIndex == index) return;
    int prev = m_displayIndex;
    m_displayIndex = index;
    emit dataChanged(createIndex(prev, 0), createIndex(prev, 0), { IsActiveRole });
    emit dataChanged(createIndex(index, 0), createIndex(index, 0), { IsActiveRole });
    emit profileSwitched(index == 0 ? "default" : m_profiles.at(index).name);
}
```

- [ ] **Step 4: setHwActiveIndex updates indicator only**

```cpp
void ProfileModel::setHwActiveIndex(int index) {
    if (m_hwActiveIndex == index) return;
    int prev = m_hwActiveIndex;
    m_hwActiveIndex = index;
    emit dataChanged(createIndex(prev, 0), createIndex(prev, 0), { HwActiveRole });
    emit dataChanged(createIndex(index, 0), createIndex(index, 0), { HwActiveRole });
}
```

Add `HwActiveRole` to the roles enum and expose in `data()`.

- [ ] **Step 5: Update QML AppProfileBar**

Chips use `model.isActive` for the user's selected tab (accent color) and `model.isHwActive` for a subtle hardware indicator (e.g., small dot or underline).

- [ ] **Step 6: Update AppController connections**

```cpp
// User tab click
connect(&m_profileModel, &ProfileModel::profileSwitched,
        this, &AppController::onTabSwitched);

// Window focus → hardware indicator (NOT profileSwitched)
// In onWindowFocusChanged:
m_profileModel.setHwActiveByWmClass(wmClass);
// NOT: m_profileModel.setActiveByWmClass(wmClass);
```

- [ ] **Step 7: Build and test**

- [ ] **Step 8: Commit**

```bash
git add -A && git commit -m "refactor: ProfileModel separates display tab from hardware indicator — no cross-contamination"
```

---

## Task 3: Clean Gesture Saves — No Lambda Cascade

**Files:**
- Modify: `src/app/models/DeviceModel.h`
- Modify: `src/app/models/DeviceModel.cpp`
- Modify: `src/app/AppController.cpp`

The problem: `gestureChanged` signal is connected via lambda to `saveCurrentProfile()`, which saves ALL buttons+gestures. This fires during programmatic gesture restore too.

- [ ] **Step 1: Add userGestureChanged signal**

Same pattern as ButtonModel:

```cpp
// In DeviceModel.h
signals:
    void userGestureChanged(const QString &direction, const QString &actionName, const QString &keystroke);
```

- [ ] **Step 2: Split setGestureAction**

```cpp
// Called by QML — user intent
void DeviceModel::setGestureAction(const QString &direction, const QString &name, const QString &ks) {
    m_gestures[direction] = qMakePair(name, ks);
    emit userGestureChanged(direction, name, ks);
}

// Called by AppController — programmatic restore, no user signal
void DeviceModel::loadGesturesFromProfile(const QMap<QString, QPair<QString, QString>> &gestures) {
    m_gestures = gestures;
    emit gestureChanged();  // QML re-reads, but no save trigger
}
```

- [ ] **Step 3: Update AppController connections**

Replace:
```cpp
connect(&m_deviceModel, &DeviceModel::gestureChanged,
    [this]() { saveCurrentProfile(); });
```
With:
```cpp
connect(&m_deviceModel, &DeviceModel::userGestureChanged,
    [this](...) { saveCurrentProfile(); });
```

- [ ] **Step 4: Update restoreButtonModelFromProfile**

The gesture restore currently calls `setGestureAction()` (the user path). Change it to call `loadGesturesFromProfile()`.

- [ ] **Step 5: Build and test**

- [ ] **Step 6: Commit**

```bash
git add -A && git commit -m "refactor: gesture changes use userGestureChanged — programmatic restore doesn't trigger saves"
```

---

## Task 4: Remove All Debug Logging

**Files:**
- Modify: All files with `fprintf(stderr, "[AC]...` etc.

- [ ] **Step 1: Remove all `[AC]`, `[PE]`, `[PM]`, `[KDE]` fprintf debug lines**

These were added for debugging the profile system. Remove them all.

- [ ] **Step 2: Build and test**

- [ ] **Step 3: Commit**

```bash
git add -A && git commit -m "chore: remove all debug fprintf logging from profile pipeline"
```

---

## Task 5: End-to-End Verification

- [ ] **Step 1: Clean all profiles**

```bash
rm -f ~/.config/Logitune/Logitune/devices/2537c7758c0ba302/profiles/*.conf
# Create clean default
cat > ~/.config/Logitune/Logitune/devices/2537c7758c0ba302/profiles/default.conf << 'EOF'
[Buttons]
0=default
1=default
2=default
3=default
4=default
5=gesture-trigger
6=keystroke:smartshift-toggle
7=default

[DPI]
value=1800

[%General]
icon=
name=Default
version=1

[Gestures]
click=keystroke:Super+W
down=keystroke:Super+D
left=keystroke:Ctrl+Super+Left
right=keystroke:Ctrl+Super+Right
up=default

[Scroll]
direction=standard
hires=false
mode=ratchet

[SmartShift]
enabled=true
threshold=100

[ThumbWheel]
mode=scroll
EOF
```

- [ ] **Step 2: Launch and verify**

1. Default profile loads clean — buttons 3,4 are "default"
2. Add Chrome profile → starts with device defaults
3. Switch to Chrome tab → UI shows Chrome's settings, NO hardware writes
4. Change thumb wheel to "volume" on Chrome tab → saves to Chrome profile only
5. Switch to Default tab → UI shows Default's settings (scroll mode), Chrome stays in cache
6. Switch window to Chrome → hardware applies Chrome's profile (volume mode active)
7. Switch window to terminal → hardware applies Default profile (scroll mode)
8. Open Logitune → tab stays on whatever you last selected, hardware doesn't change

- [ ] **Step 3: Verify profiles on disk**

```bash
cat ~/.config/Logitune/Logitune/devices/*/profiles/default.conf | grep -A1 ThumbWheel
# Should show: mode=scroll

cat ~/.config/Logitune/Logitune/devices/*/profiles/"Google Chrome.conf" | grep -A1 ThumbWheel
# Should show: mode=volume
```

- [ ] **Step 4: Commit**

```bash
git add -A && git commit -m "test: verified MVVM signal separation — profiles independent, no cascades"
```

---

## Summary of Signal Paths After Refactor

```
USER CLICKS:
  QML → DeviceModel.setDPI()         → dpiChangeRequested        → AppController → cache + save + maybe hw
  QML → DeviceModel.setGestureAction()→ userGestureChanged        → AppController → cache + save
  QML → ButtonModel.setAction()      → userActionChanged          → AppController → cache + save + maybe hw
  QML → ProfileModel.selectTab()     → profileSwitched            → AppController → setDisplayProfile

PROGRAMMATIC:
  AppController → DeviceModel.loadSettingsFromProfile()  → settingsReloaded (one signal, QML rebinds)
  AppController → DeviceModel.loadGesturesFromProfile()  → gestureChanged   (QML re-reads, no save)
  AppController → ButtonModel.loadFromProfile()          → modelReset       (QML rebinds, no signals)
  AppController → ProfileModel.setHwActiveByWmClass()    → dataChanged(HwActiveRole) (visual only)

HARDWARE:
  DeviceManager → DeviceModel (battery, name, connection only — NOT dpi/smartshift/scroll)
  KWinScript    → KDeDesktop.focusChanged()              → AppController → hw apply + indicator update
```

No signal ever triggers itself. No programmatic update triggers a save. No hardware event changes the display tab.
