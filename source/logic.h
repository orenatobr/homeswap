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

// X, just pressed in the Wii U Menu, flips the toggle; everything else leaves it alone.
inline bool toggleAfterInput(bool xToggleOn, bool inHome, bool configOpen, uint32_t trigger)
{
    if (inHome && !configOpen && (trigger & kButtonX)) {
        return !xToggleOn;
    }
    return xToggleOn;
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
