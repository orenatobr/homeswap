#include "../source/logic.h"
#include <cassert>

int main()
{
    // Only swaps in the Home, enabled, with the config menu closed.
    assert(shouldSwap(true, false, true));
    assert(!shouldSwap(false, false, true)); // plugin disabled
    assert(!shouldSwap(true, true, true));   // Aroma config menu open
    assert(!shouldSwap(true, false, false)); // game or other app

    assert(swappedTarget(true, kScanTV) == kScanDRC);
    assert(swappedTarget(true, kScanDRC) == kScanTV);
    assert(swappedTarget(false, kScanTV) == kScanTV);
    assert(swappedTarget(false, kScanDRC) == kScanDRC);
    assert(swappedTarget(true, kScanTV | kScanDRC) == (kScanTV | kScanDRC)); // combined targets untouched
    return 0;
}
