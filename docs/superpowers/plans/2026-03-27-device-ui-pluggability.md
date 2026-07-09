# Device UI Pluggability — Follow-up Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make the QML UI layer device-agnostic so adding a keyboard, trackpad, or any Logitech peripheral requires zero QML changes — only a device descriptor.

**Architecture:** IDevice declares which pages and feature panels apply. SideNav, DeviceView, and settings panels are driven by device capabilities, not hardcoded for the MX Master 3S.

**Tech Stack:** C++20, Qt 6 Quick, existing IDevice interface

**Status:** Deferred — implement when adding a second device.

---

## Summary of Changes Needed

### 1. Add `DevicePage` enum and `pages()` to IDevice

```cpp
enum class DevicePage { Buttons, PointScroll, EasySwitch, Backlight, FunctionKeys, Settings };
virtual QList<DevicePage> pages() const = 0;
```

MX Master 3S: `{Buttons, PointScroll, EasySwitch, Settings}`
MX Keys: `{FunctionKeys, Backlight, EasySwitch, Settings}`

### 2. Add feature-driven settings panel descriptors to IDevice

```cpp
struct SettingsPanelDescriptor {
    QString panelType;    // "scrollwheel", "thumbwheel", "pointerspeed", "backlight", etc.
    QString title;
    int hotspotIndex;     // which scrollHotspot this panel corresponds to (-1 if none)
};
virtual QList<SettingsPanelDescriptor> settingsPanels() const = 0;
```

### 3. Expose pages to QML via DeviceModel

```cpp
Q_PROPERTY(QVariantList devicePages READ devicePages NOTIFY deviceConnectedChanged)
Q_PROPERTY(QVariantList settingsPanels READ settingsPanels NOTIFY deviceConnectedChanged)
```

### 4. Make SideNav dynamic

Replace hardcoded nav items with a Repeater bound to `DeviceModel.devicePages`. Each DevicePage enum value maps to an icon + label.

### 5. Make DeviceView page switching dynamic

Replace the hardcoded `contentStack.replace()` switch statement with a mapping from page enum to QML component URL. Load components dynamically via `Qt.createComponent()` or a Loader.

### 6. Make DetailPanel feature-driven

Replace the hardcoded `scrollWheelContent`/`thumbWheelContent`/`pointerSpeedContent` components with a Loader that selects content based on `panelType` string. New panel types (e.g., "backlight") can be added as new Component definitions without modifying existing ones.

### 7. Make PointScrollPage generic

Rename to `FeaturePanelsPage` or similar. Instead of hardcoding 3 InfoCallouts for scroll/thumb/pointer, use a Repeater driven by `DeviceModel.settingsPanels`. Each callout's title, settings summary, and position come from the descriptor.

---

## What This Enables

| To add... | You implement... |
|-----------|-----------------|
| MX Keys keyboard | `MxKeysDescriptor` with pages={FunctionKeys, Backlight, EasySwitch, Settings} |
| MX Anywhere mouse | `MxAnywhereDescriptor` with same pages as MX Master 3S but different hotspots/CIDs |
| MX Ergo trackball | `MxErgoDescriptor` with pages={Buttons, PointScroll, EasySwitch, Settings} + trackball-specific panels |
| Backlight page | One new QML component + register the `DevicePage::Backlight` → component mapping |
| Function keys page | One new QML component + register the `DevicePage::FunctionKeys` → component mapping |

Zero changes to existing QML pages when adding a new device of an existing type.
