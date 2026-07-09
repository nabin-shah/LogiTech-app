# Profile Cache Refactor — Decouple UI Tabs from Hardware State

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** All profiles live in memory. Tab clicks switch what the UI displays. Only window focus changes push settings to hardware. Fast, no flickering, no signal cascades.

**Architecture:** ProfileEngine becomes a profile cache. AppController has two separate concerns: "which profile is the UI showing" vs "which profile is active on the hardware". These are independent.

**Tech Stack:** C++20, Qt 6 Quick, existing codebase

---

## Current Problems

1. Tab click → `switchToProfile` → loads from disk → emits `activeProfileChanged` + `profileDelta` → triggers HID++ writes + UI restore → slow, blocks UI
2. UI state is coupled to hardware state — changing tabs applies settings to the device
3. Profile delta detection fails when profiles have same values → stale labels
4. Signal cascades: tab click → profile switch → settings change → save profile → settings change signal → ...
5. `m_savingProfile` guard is a band-aid for re-entrant signal loops

## Target Architecture

```
┌─────────────────┐     ┌──────────────────┐     ┌─────────────┐
│   Profile Tabs  │     │  Window Focus    │     │   Hardware   │
│   (UI only)     │     │  Detection       │     │   (HID++)    │
│                 │     │                  │     │             │
│ Click tab →     │     │ Focus change →   │     │             │
│ show that       │     │ apply profile    │     │             │
│ profile's       │     │ to hardware      │     │             │
│ settings in UI  │     │ (fast, cached)   │     │             │
└────────┬────────┘     └────────┬─────────┘     └──────▲──────┘
         │                       │                       │
         ▼                       ▼                       │
┌────────────────────────────────────────────────────────┤
│              ProfileCache (in memory)                  │
│                                                        │
│  profiles: QMap<QString, Profile>                      │
│  displayProfile: QString  ← what UI shows              │
│  activeProfile: QString   ← what's on hardware         │
│                                                        │
│  getProfile(name) → Profile&  (no disk read)           │
│  setDisplayProfile(name)  → UI updates only            │
│  applyToHardware(name)    → HID++ writes only          │
│  saveProfile(name)        → disk write only            │
└────────────────────────────────────────────────────────┘
```

## File Structure

### Files to modify

```
src/core/ProfileEngine.h/.cpp     — add in-memory cache, separate display vs active
src/app/AppController.h/.cpp      — decouple tab switching from hardware apply
src/app/models/DeviceModel.h/.cpp — UI reads from display profile, not hardware state
```

### No new files needed

---

## Task 1: Add Profile Cache to ProfileEngine

**Files:**
- Modify: `src/core/ProfileEngine.h`
- Modify: `src/core/ProfileEngine.cpp`

ProfileEngine currently loads profiles from disk on every `switchToProfile()` call. Change it to:

- [ ] **Step 1: Add profile cache**

In `ProfileEngine.h`, add:
```cpp
QMap<QString, Profile> m_cache;  // all profiles loaded into memory
QString m_displayProfile;        // which profile the UI is showing
QString m_hardwareProfile;       // which profile is applied to hardware
```

- [ ] **Step 2: Load all profiles into cache on `setDeviceConfigDir`**

When the device config dir is set, scan all `.conf` files and load every profile into `m_cache`. This runs once on startup.

```cpp
void ProfileEngine::setDeviceConfigDir(const QString &dir) {
    m_configDir = dir;
    m_cache.clear();
    QDir d(dir);
    for (const auto &f : d.entryList({"*.conf"}, QDir::Files)) {
        QString name = QFileInfo(f).baseName();
        m_cache[name] = loadProfile(d.filePath(f));
    }
    // ... load app bindings
}
```

- [ ] **Step 3: Add new methods**

```cpp
// Get a cached profile (no disk read)
Profile& cachedProfile(const QString &name);

// Switch which profile the UI displays (no hardware writes, no disk reads)
void setDisplayProfile(const QString &name);

// Apply a profile to hardware (fire-and-forget HID++ writes)
void applyToHardware(const QString &name);

// Save a specific profile to disk (background, no signals)
void saveProfileToDisk(const QString &name);
```

- [ ] **Step 4: Add new signals**

```cpp
signals:
    void displayProfileChanged(const Profile &profile);  // UI should update to show this
    void hardwareProfileChanged(const Profile &profile);  // hardware needs these settings
```

- [ ] **Step 5: Keep `switchToProfile` for backward compat but deprecate**

Make `switchToProfile` call both `setDisplayProfile` + `applyToHardware`.

- [ ] **Step 6: Update `updateActiveProfile`**

When the user changes a setting in the UI, update the cached profile + save to disk. Don't emit hardware signals (the setting is already applied via the UI control's direct call to DeviceManager).

- [ ] **Step 7: Build and test**

```bash
cd /home/mina/repos/logitune/build && cmake --build . 2>&1
./tests/logitune-tests
```

- [ ] **Step 8: Commit**

```bash
git add -A && git commit -m "refactor: add profile cache — all profiles in memory, separate display vs hardware"
```

---

## Task 2: Decouple AppController — Tab Switch vs Hardware Apply

**Files:**
- Modify: `src/app/AppController.h`
- Modify: `src/app/AppController.cpp`

- [ ] **Step 1: Split the signal handling**

Currently:
- `ProfileModel::profileSwitched` → `ProfileEngine::switchToProfile` (loads + applies + signals)

Change to:
- `ProfileModel::profileSwitched` → `AppController::onTabSwitched` → only calls `ProfileEngine::setDisplayProfile`
- `IDesktopIntegration::activeWindowChanged` → `AppController::onWindowFocusChanged` → calls `ProfileEngine::applyToHardware`

- [ ] **Step 2: Implement `onTabSwitched`**

```cpp
void AppController::onTabSwitched(const QString &profileName) {
    // Just update what the UI shows — no hardware writes
    m_profileEngine.setDisplayProfile(profileName);

    const Profile &p = m_profileEngine.cachedProfile(profileName);

    // Update all UI models to reflect this profile's settings
    m_deviceModel.setCurrentDPI(p.dpi);          // UI display only
    m_deviceModel.setSmartShiftState(p.smartShiftEnabled, p.smartShiftThreshold);
    // ... update all UI-facing properties

    // Update button model
    restoreButtonModelFromProfile(p);  // renamed from restoreProfileToDevice

    // Update thumb wheel label
    updateThumbWheelLabel(p.thumbWheelMode);
}
```

- [ ] **Step 3: Implement hardware apply on window focus**

```cpp
void AppController::onWindowFocusChanged(const QString &wmClass, const QString &title) {
    if (wmClass == "logitune") return;

    QString profileName = m_profileEngine.profileForApp(wmClass);
    if (profileName == m_profileEngine.hardwareProfile()) return;  // already active

    m_profileEngine.applyToHardware(profileName);

    const Profile &p = m_profileEngine.cachedProfile(profileName);

    // Fire-and-forget HID++ writes (non-blocking)
    m_deviceManager.setDPI(p.dpi);
    m_deviceManager.setSmartShift(p.smartShiftEnabled, p.smartShiftThreshold);
    m_deviceManager.setScrollConfig(p.hiResScroll, p.scrollDirection == "natural");
    m_deviceManager.setThumbWheelMode(p.thumbWheelMode);

    // Re-divert buttons as needed
    applyButtonDiversions(p);

    // Update UI to show the new active profile
    m_profileModel.setActiveByWmClass(wmClass);
}
```

- [ ] **Step 4: Implement setting change save**

When the user changes a setting in the UI (e.g., DPI slider):
```cpp
void AppController::onDeviceStateChanged() {
    // Save to the DISPLAY profile (the one the user is editing), not the hardware profile
    QString name = m_profileEngine.displayProfile();
    Profile &p = m_profileEngine.cachedProfile(name);
    p.dpi = m_deviceManager.currentDPI();
    // ... update all fields
    m_profileEngine.saveProfileToDisk(name);  // async disk write
}
```

- [ ] **Step 5: Remove signal cascades**

Remove:
- `m_savingProfile` guard — no longer needed
- The `profileDelta` → `onProfileDelta` connection — hardware apply is explicit, not signal-driven
- The deferred `QTimer::singleShot` writes — writes are already non-blocking

- [ ] **Step 6: Remove stale DeviceModel caches**

DeviceModel should read from the display profile, not from DeviceManager directly (for UI display). DeviceManager is only for hardware state. This might need DeviceModel to hold a reference to ProfileEngine or receive the display profile data directly.

Actually, the simpler approach: when a tab is switched, AppController pushes the profile's values to DeviceModel. DeviceModel doesn't need to know about profiles — it just holds the current display values.

- [ ] **Step 7: Build, test, smoke test**

```bash
cd /home/mina/repos/logitune/build && cmake --build . 2>&1
./tests/logitune-tests
pkill -9 logitune; nohup ./src/app/logitune > /tmp/logitune.log 2>&1 &
```

Test:
1. Click between profile tabs — UI should update instantly, no HID++ writes
2. Change a setting — saves to the displayed profile only
3. Switch windows — hardware applies the correct profile
4. Switch back to Logitune — UI stays on whatever tab you last selected

- [ ] **Step 8: Commit**

```bash
git add -A && git commit -m "refactor: decouple UI tabs from hardware — tabs show cached config, focus change applies to device"
```

---

## Task 3: Clean Up Thumb Wheel Handling

**Files:**
- Modify: `src/app/AppController.cpp`

- [ ] **Step 1: Remove all the scattered thumb wheel label workarounds**

With the new architecture, the thumb wheel label is just another field in the cached profile. When a tab is displayed, `onTabSwitched` sets all labels including thumb wheel. No special cases needed.

- [ ] **Step 2: Remove `skip i==7` hacks**

The button restore loop can include button 7 since it only updates ButtonModel (UI), not hardware. The thumb wheel HID++ write happens separately in `applyToHardware`.

- [ ] **Step 3: Commit**

```bash
git add -A && git commit -m "chore: remove thumb wheel workarounds — clean profile cache handles it"
```

---

## Expected Result

| Action | Before | After |
|--------|--------|-------|
| Click profile tab | 200-500ms (disk read + HID++ writes + signal cascade) | <5ms (memory read, UI update) |
| Change a setting | Saves immediately, triggers re-save loops | Updates cache + async disk write |
| Switch windows | Profile switch + HID++ writes (slow) | Cached profile → fire-and-forget HID++ (~5ms) |
| App startup | Loads default profile only | Loads ALL profiles into cache |
