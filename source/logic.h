#pragma once
#include <cstdint>

// GX2 scan targets (values checked against wut with static_assert in main.cpp).
constexpr uint32_t kScanTV  = 1u << 0;
constexpr uint32_t kScanDRC = 1u << 2;

// The swap only runs in the Wii U Menu, and never while the Aroma config menu is open.
inline bool shouldSwap(bool enabled, bool configOpen, bool inHome)
{
    return enabled && !configOpen && inHome;
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
