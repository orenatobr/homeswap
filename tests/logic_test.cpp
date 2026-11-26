#include "../source/logic.h"
#include <cassert>

int main()
{
    // The plugin swaps only in state A, in the Home, enabled, with the config menu closed.
    assert(shouldSwap(true, false, true, XState::PluginSwap));
    assert(!shouldSwap(false, false, true, XState::PluginSwap)); // plugin disabled
    assert(!shouldSwap(true, true, true, XState::PluginSwap));   // Aroma config menu open
    assert(!shouldSwap(true, false, false, XState::PluginSwap)); // game or other app
    assert(!shouldSwap(true, false, true, XState::Original));    // B: original screens
    assert(!shouldSwap(true, false, true, XState::NativeSwap));  // C: the menu swaps by itself

    assert(swappedTarget(true, kScanTV) == kScanDRC);
    assert(swappedTarget(true, kScanDRC) == kScanTV);
    assert(swappedTarget(false, kScanTV) == kScanTV);
    assert(swappedTarget(false, kScanDRC) == kScanDRC);
    assert(swappedTarget(true, kScanTV | kScanDRC) == (kScanTV | kScanDRC)); // combined targets untouched

    // X cycles A -> B -> C -> A.
    assert(nextXState(XState::PluginSwap) == XState::Original);
    assert(nextXState(XState::Original) == XState::NativeSwap);
    assert(nextXState(XState::NativeSwap) == XState::PluginSwap);

    // A press in the Home: A hides X from the menu, B and C pass it so the menu swaps natively.
    {
        XTracker t;
        uint32_t hold = kButtonX | 0x1, trigger = kButtonX, release = 0;
        trackX(t, true, false, hold, trigger, release);
        assert(t.state == XState::Original);
        assert(hold == 0x1 && trigger == 0);                  // A -> B: hidden
        hold = kButtonX; trigger = kButtonX; release = 0;
        trackX(t, true, false, hold, trigger, release);
        assert(t.state == XState::NativeSwap);
        assert(hold == kButtonX && trigger == kButtonX);      // B -> C: the menu gets it
        hold = kButtonX; trigger = kButtonX; release = 0;
        trackX(t, true, false, hold, trigger, release);
        assert(t.state == XState::PluginSwap);
        assert(hold == kButtonX && trigger == kButtonX);      // C -> A: the menu gets it
    }

    // A hidden press stays hidden while X is held and on release, even though the state moved on.
    {
        XTracker t;
        uint32_t hold = kButtonX, trigger = kButtonX, release = 0;
        trackX(t, true, false, hold, trigger, release);       // A -> B, hidden
        hold = kButtonX; trigger = 0; release = 0;
        trackX(t, true, false, hold, trigger, release);       // still held
        assert(hold == 0 && t.state == XState::Original);
        hold = 0; trigger = 0; release = kButtonX;
        trackX(t, true, false, hold, trigger, release);       // let go
        assert(release == 0 && t.state == XState::Original);
        hold = kButtonX; trigger = kButtonX; release = 0;
        trackX(t, true, false, hold, trigger, release);       // next press (B -> C) passes
        assert(hold == kButtonX && trigger == kButtonX && t.state == XState::NativeSwap);
    }

    // A passed press stays passed while X is held and on release.
    {
        XTracker t{XState::Original, false};
        uint32_t hold = kButtonX, trigger = kButtonX, release = 0;
        trackX(t, true, false, hold, trigger, release);
        hold = kButtonX; trigger = 0; release = 0;
        trackX(t, true, false, hold, trigger, release);
        assert(hold == kButtonX);
        hold = 0; trigger = 0; release = kButtonX;
        trackX(t, true, false, hold, trigger, release);
        assert(release == kButtonX && t.state == XState::NativeSwap);
    }

    // Other buttons and the idle frame change nothing.
    {
        XTracker t;
        uint32_t hold = 0x1, trigger = 0x1, release = 0;
        trackX(t, true, false, hold, trigger, release);
        assert(t.state == XState::PluginSwap && hold == 0x1 && trigger == 0x1);
        hold = 0; trigger = 0;
        trackX(t, true, false, hold, trigger, release);
        assert(t.state == XState::PluginSwap);
    }

    // Outside the Home, or with the config menu open, X is untouched and the state stays.
    {
        XTracker t;
        uint32_t hold = kButtonX, trigger = kButtonX, release = 0;
        trackX(t, false, false, hold, trigger, release);      // game or other app
        assert(t.state == XState::PluginSwap && hold == kButtonX && trigger == kButtonX);
        trackX(t, true, true, hold, trigger, release);        // config menu open
        assert(t.state == XState::PluginSwap && hold == kButtonX && trigger == kButtonX);
    }

    // One scratch buffer is reused across frames while it is big enough and aligned enough.
    assert(canReuseScratch(8192, 256, 8192, 256));   // same size
    assert(canReuseScratch(8192, 256, 2048, 256));   // smaller request fits
    assert(canReuseScratch(8192, 512, 2048, 256));   // stricter alignment satisfies a weaker one
    assert(!canReuseScratch(2048, 256, 8192, 256));  // too small: grow
    assert(!canReuseScratch(8192, 256, 2048, 512));  // alignment too weak: reallocate
    assert(!canReuseScratch(0, 0, 2048, 256));       // nothing allocated yet
    return 0;
}
