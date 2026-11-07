#pragma once
#include <cstdint>

// GX2 scan targets (values checked against wut with static_assert in main.cpp).
constexpr uint32_t kScanTV  = 1u << 0;
constexpr uint32_t kScanDRC = 1u << 2;

// GamePad X button (value checked against wut with static_assert in main.cpp).
constexpr uint32_t kButtonX = 0x2000;

// The swap only runs in the Wii U Menu, never while the Aroma config menu is open,
// and not after the user turned it off with X.
inline bool shouldSwap(bool enabled, bool configOpen, bool inHome, bool xToggleOn)
{
    return enabled && !configOpen && inHome && xToggleOn;
}

// In the Wii U Menu, with the config menu closed, X belongs to the plugin.
inline bool xIsOurs(bool inHome, bool configOpen)
{
    return inHome && !configOpen;
}

// X, just pressed where it is ours, flips the toggle; everything else leaves it alone.
inline bool toggleAfterInput(bool xToggleOn, bool inHome, bool configOpen, uint32_t trigger)
{
    if (xIsOurs(inHome, configOpen) && (trigger & kButtonX)) {
        return !xToggleOn;
    }
    return xToggleOn;
}

// The menu has its own X (it swaps the screens too), so hide X from it where the plugin uses it.
inline uint32_t withoutX(uint32_t buttons, bool inHome, bool configOpen)
{
    return xIsOurs(inHome, configOpen) ? (buttons & ~kButtonX) : buttons;
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
