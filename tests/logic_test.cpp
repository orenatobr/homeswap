#include "../source/logic.h"
#include <cassert>

int main()
{
    // The swap runs in the Home whenever the plugin is enabled and the config menu is closed.
    // X no longer touches it: the Home stays on the TV.
    assert(shouldSwap(true, false, true));
    assert(!shouldSwap(false, false, true)); // plugin disabled in the config menu
    assert(!shouldSwap(true, true, true));   // Aroma config menu open
    assert(!shouldSwap(true, false, false)); // game or other app

    assert(swappedTarget(true, kScanTV) == kScanDRC);
    assert(swappedTarget(true, kScanDRC) == kScanTV);
    assert(swappedTarget(false, kScanTV) == kScanTV);
    assert(swappedTarget(false, kScanDRC) == kScanDRC);
    assert(swappedTarget(true, kScanTV | kScanDRC) == (kScanTV | kScanDRC)); // combined targets untouched

    // X flips the GamePad control only when it was just pressed, in the Home, with the config menu closed.
    assert(toggleAfterInput(true, true, false, kButtonX) == false);          // on -> off
    assert(toggleAfterInput(false, true, false, kButtonX) == true);          // off -> on
    assert(toggleAfterInput(true, true, false, kButtonX | 0x1) == false);    // other buttons in the same frame
    assert(toggleAfterInput(true, true, false, 0) == true);                  // nothing pressed
    assert(toggleAfterInput(true, true, false, 0x1) == true);                // another button
    assert(toggleAfterInput(true, false, false, kButtonX) == true);          // game or other app: ignored
    assert(toggleAfterInput(true, true, true, kButtonX) == true);            // config menu open: ignored

    // The GamePad is locked only in the Home, with the config menu closed and the toggle off.
    assert(padLocked(false, true, false));   // toggled off in the Home
    assert(!padLocked(true, true, false));   // toggle on: normal control
    assert(!padLocked(false, false, false)); // game or other app: never locked
    assert(!padLocked(false, true, true));   // config menu open: never locked

    // maskedButtons(buttons, padOn, inHome, configOpen).
    // X belongs to the plugin in the Home, so the menu never sees it; elsewhere it passes through.
    assert(maskedButtons(kButtonX | 0x1, true, true, false) == 0x1);   // control on: X removed, rest kept
    assert(maskedButtons(kButtonX, true, true, false) == 0);
    assert(maskedButtons(kButtonX | 0x1, false, true, false) == 0);    // locked: every button removed
    assert(maskedButtons(kButtonHome | kButtonX | 0x1, false, true, false) == kButtonHome); // HOME still passes
    assert(maskedButtons(kButtonX | 0x1, true, false, false) == (kButtonX | 0x1));  // game or other app: untouched
    assert(maskedButtons(kButtonX | 0x1, false, false, false) == (kButtonX | 0x1)); // flag off outside the Home: untouched
    assert(maskedButtons(kButtonX | 0x1, true, true, true) == (kButtonX | 0x1));    // config menu open: untouched
    assert(maskedButtons(kButtonX | 0x1, false, true, true) == (kButtonX | 0x1));   // config menu open, flag off: untouched

    // One scratch buffer is reused across frames while it is big enough and aligned enough.
    assert(canReuseScratch(8192, 256, 8192, 256));   // same size
    assert(canReuseScratch(8192, 256, 2048, 256));   // smaller request fits
    assert(canReuseScratch(8192, 512, 2048, 256));   // stricter alignment satisfies a weaker one
    assert(!canReuseScratch(2048, 256, 8192, 256));  // too small: grow
    assert(!canReuseScratch(8192, 256, 2048, 512));  // alignment too weak: reallocate
    assert(!canReuseScratch(0, 0, 2048, 256));       // nothing allocated yet
    return 0;
}
