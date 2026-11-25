#pragma once
#include <cstdint>

// GX2 scan targets (values checked against wut with static_assert in main.cpp).
constexpr uint32_t kScanTV  = 1u << 0;
constexpr uint32_t kScanDRC = 1u << 2;

// GamePad X and HOME buttons (values checked against wut with static_assert in main.cpp).
constexpr uint32_t kButtonX    = 0x2000;
constexpr uint32_t kButtonHome = 0x0002;

// The swap only runs in the Wii U Menu and never while the Aroma config menu is open.
// X does not touch it: the Home stays on the TV.
inline bool shouldSwap(bool enabled, bool configOpen, bool inHome)
{
    return enabled && !configOpen && inHome;
}

// In the Wii U Menu, with the config menu closed, X belongs to the plugin.
inline bool xIsOurs(bool inHome, bool configOpen)
{
    return inHome && !configOpen;
}

// X, just pressed where it is ours, flips the GamePad control; everything else leaves it alone.
inline bool toggleAfterInput(bool padOn, bool inHome, bool configOpen, uint32_t trigger)
{
    if (xIsOurs(inHome, configOpen) && (trigger & kButtonX)) {
        return !padOn;
    }
    return padOn;
}

// The GamePad is switched off for the Home (toggled with X) only where X is ours.
inline bool padLocked(bool padOn, bool inHome, bool configOpen)
{
    return xIsOurs(inHome, configOpen) && !padOn;
}

// Buttons the Wii U Menu gets. The menu has its own X (it swaps the screens too), so X is always
// hidden where the plugin uses it. With the GamePad switched off, only HOME gets through.
inline uint32_t maskedButtons(uint32_t buttons, bool padOn, bool inHome, bool configOpen)
{
    if (padLocked(padOn, inHome, configOpen)) {
        return buttons & kButtonHome;
    }
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
