#include "../source/logic.h"
#include <cassert>

int main()
{
    // Only swaps in the Home, enabled, with the config menu closed and the X toggle on.
    assert(shouldSwap(true, false, true, true));
    assert(!shouldSwap(false, false, true, true)); // plugin disabled
    assert(!shouldSwap(true, true, true, true));   // Aroma config menu open
    assert(!shouldSwap(true, false, false, true)); // game or other app
    assert(!shouldSwap(true, false, true, false)); // toggled off with X: original screens

    assert(swappedTarget(true, kScanTV) == kScanDRC);
    assert(swappedTarget(true, kScanDRC) == kScanTV);
    assert(swappedTarget(false, kScanTV) == kScanTV);
    assert(swappedTarget(false, kScanDRC) == kScanDRC);
    assert(swappedTarget(true, kScanTV | kScanDRC) == (kScanTV | kScanDRC)); // combined targets untouched

    // X flips the toggle only when it was just pressed, in the Home, with the config menu closed.
    assert(toggleAfterInput(true, true, false, kButtonX) == false);          // on -> off
    assert(toggleAfterInput(false, true, false, kButtonX) == true);          // off -> on
    assert(toggleAfterInput(true, true, false, kButtonX | 0x1) == false);    // other buttons in the same frame
    assert(toggleAfterInput(true, true, false, 0) == true);                  // nothing pressed
    assert(toggleAfterInput(true, true, false, 0x1) == true);                // another button
    assert(toggleAfterInput(true, false, false, kButtonX) == true);          // game or other app: ignored
    assert(toggleAfterInput(true, true, true, kButtonX) == true);            // config menu open: ignored

    // X belongs to the plugin in the Home, so the menu never sees it; elsewhere it passes through.
    assert(withoutX(kButtonX | 0x1, true, false) == 0x1);        // Home: X removed, other buttons kept
    assert(withoutX(kButtonX, true, false) == 0);
    assert(withoutX(kButtonX | 0x1, false, false) == (kButtonX | 0x1)); // game or other app: untouched
    assert(withoutX(kButtonX | 0x1, true, true) == (kButtonX | 0x1));   // config menu open: untouched

    // One scratch buffer is reused across frames while it is big enough and aligned enough.
    assert(canReuseScratch(8192, 256, 8192, 256));   // same size
    assert(canReuseScratch(8192, 256, 2048, 256));   // smaller request fits
    assert(canReuseScratch(8192, 512, 2048, 256));   // stricter alignment satisfies a weaker one
    assert(!canReuseScratch(2048, 256, 8192, 256));  // too small: grow
    assert(!canReuseScratch(8192, 256, 2048, 512));  // alignment too weak: reallocate
    assert(!canReuseScratch(0, 0, 2048, 256));       // nothing allocated yet
    return 0;
}
