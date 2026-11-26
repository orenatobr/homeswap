#pragma once
#include <cstdint>

// GX2 scan targets (values checked against wut with static_assert in main.cpp).
constexpr uint32_t kScanTV  = 1u << 0;
constexpr uint32_t kScanDRC = 1u << 2;

// GamePad X button (value checked against wut with static_assert in main.cpp).
constexpr uint32_t kButtonX = 0x2000;

// What X does in the Wii U Menu. Each press moves to the next state: A -> B -> C -> A.
//   PluginSwap (A): the plugin swaps the scan targets, so the TV streams the Home as the GamePad
//                   draws it and the GamePad shows the Mii plaza.
//   Original   (B): no swap; the TV shows the Mii plaza and the GamePad shows the Home.
//   NativeSwap (C): no plugin swap; the menu swaps by itself (its own X), so the TV shows the Home
//                   as the menu draws it for the TV and the GamePad shows the Mii plaza.
enum class XState : uint8_t { PluginSwap, Original, NativeSwap };

inline XState nextXState(XState state)
{
    switch (state) {
    case XState::PluginSwap: return XState::Original;
    case XState::Original:   return XState::NativeSwap;
    case XState::NativeSwap: return XState::PluginSwap;
    }
    return XState::PluginSwap;
}

// The swap only runs in the Wii U Menu, in state A, and never while the Aroma config menu is open.
inline bool shouldSwap(bool enabled, bool configOpen, bool inHome, XState state)
{
    return enabled && !configOpen && inHome && state == XState::PluginSwap;
}

// In the Wii U Menu, with the config menu closed, X belongs to the plugin.
inline bool xIsOurs(bool inHome, bool configOpen)
{
    return inHome && !configOpen;
}

// One physical X press reaches the plugin several times (every reader of the GamePad gets the
// same trigger, so a single press counts three times on the console). Presses closer than this
// are the same press.
constexpr uint64_t kPressDebounceMs = 300;

// The X state, when the last press was counted, and what to do with the X of that press.
struct XTracker {
    XState state       = XState::PluginSwap;
    uint64_t lastPress = 0;
    bool havePress     = false;
    bool passToMenu    = false;
};

// Feeds one GamePad sample through the tracker. A fresh X press moves the state on once. Leaving
// A the plugin handles the press and hides X from the menu; leaving B or C the press is passed on,
// so the menu runs its own swap. The decision holds for every copy, hold and release of that
// press. Anywhere X is not ours, nothing changes.
inline void trackX(XTracker &t, uint64_t nowMs, bool inHome, bool configOpen, uint32_t &hold, uint32_t &trigger, uint32_t &release)
{
    if (!xIsOurs(inHome, configOpen)) {
        return;
    }
    if ((trigger & kButtonX) && (!t.havePress || nowMs - t.lastPress >= kPressDebounceMs)) {
        t.havePress  = true;
        t.lastPress  = nowMs;
        t.passToMenu = t.state != XState::PluginSwap;
        t.state      = nextXState(t.state);
    }
    if (!t.passToMenu) {
        hold &= ~kButtonX;
        trigger &= ~kButtonX;
        release &= ~kButtonX;
    }
}

// TV <-> GamePad when swapping; any other target (or no swap) is left as is.
inline uint32_t swappedTarget(bool swap, uint32_t target)
{
    if (!swap) {
        return target;
    }
    if (target == kScanTV) {
        return kScanDRC;
    }
    if (target == kScanDRC) {
        return kScanTV;
    }
    return target;
}

// The AA scratch buffer is kept between frames. It can serve a request that is not bigger
// and not more strictly aligned (alignments are powers of two). Nothing allocated yet: no.
inline bool canReuseScratch(uint32_t haveSize, uint32_t haveAlign, uint32_t needSize, uint32_t needAlign)
{
    return haveSize != 0 && haveSize >= needSize && haveAlign >= needAlign;
}
