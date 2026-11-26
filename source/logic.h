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

// The X state plus whether the current X press is being hidden from the menu.
// A press leaving A is hidden (the plugin handles the swap); a press leaving B or C is passed on,
// so the menu runs its own swap. The choice is kept until X is let go, so hold and release agree
// with the press even though the state has moved on.
struct XTracker {
    XState state = XState::PluginSwap;
    bool hiding  = false;
};

// Feeds one GamePad sample through the tracker: moves the state on a fresh X press and removes X
// from hold/trigger/release when it is being hidden. Anywhere X is not ours, nothing changes.
inline void trackX(XTracker &t, bool inHome, bool configOpen, uint32_t &hold, uint32_t &trigger, uint32_t &release)
{
    if (!xIsOurs(inHome, configOpen)) {
        t.hiding = false;
        return;
    }
    const bool held = (hold & kButtonX) != 0;
    if (trigger & kButtonX) {
        t.hiding = t.state == XState::PluginSwap;
        t.state  = nextXState(t.state);
    }
    if (t.hiding) {
        hold &= ~kButtonX;
        trigger &= ~kButtonX;
        release &= ~kButtonX;
    }
    if (!held) {
        t.hiding = false;
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
