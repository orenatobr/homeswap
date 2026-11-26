#include "../source/logic.h"
#include <cassert>
#include <initializer_list>

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

    // One physical press reaches the plugin several times (every reader of the GamePad gets the
    // same trigger). The copies arrive within a few milliseconds and must move the state once.
    {
        XTracker t;
        for (int copy = 0; copy < 3; copy++) {
            uint32_t hold = kButtonX | 0x1, trigger = kButtonX, release = 0;
            trackX(t, 1000 + copy, true, false, hold, trigger, release);
            assert(!(hold & kButtonX) && !(trigger & kButtonX)); // A: hidden from the menu
            assert(hold & 0x1);                                  // other buttons pass
        }
        assert(t.state == XState::Original);                     // A -> B, once
        uint32_t hold = kButtonX, trigger = 0, release = 0;      // still held
        trackX(t, 1100, true, false, hold, trigger, release);
        assert(hold == 0);
        hold = 0; trigger = 0; release = kButtonX;               // let go
        trackX(t, 1200, true, false, hold, trigger, release);
        assert(release == 0 && t.state == XState::Original);
    }

    // Next presses (well apart): B -> C and C -> A pass X on, so the menu runs its own swap, and
    // every copy of the press, plus its hold and release, is passed the same way.
    {
        XTracker t{XState::Original, 0, false, false};
        for (int press = 0; press < 2; press++) {
            const uint64_t at = 5000 + 1000 * press;
            for (int copy = 0; copy < 3; copy++) {
                uint32_t hold = kButtonX, trigger = kButtonX, release = 0;
                trackX(t, at + copy, true, false, hold, trigger, release);
                assert(hold == kButtonX && trigger == kButtonX);
            }
            uint32_t hold = kButtonX, trigger = 0, release = 0;
            trackX(t, at + 100, true, false, hold, trigger, release);
            assert(hold == kButtonX);
            hold = 0; trigger = 0; release = kButtonX;
            trackX(t, at + 200, true, false, hold, trigger, release);
            assert(release == kButtonX);
        }
        assert(t.state == XState::PluginSwap);                   // B -> C -> A
    }

    // A second press inside the debounce window is the same press: no state change.
    {
        XTracker t;
        uint32_t hold = kButtonX, trigger = kButtonX, release = 0;
        trackX(t, 1000, true, false, hold, trigger, release);
        hold = kButtonX; trigger = kButtonX; release = 0;
        trackX(t, 1000 + kPressDebounceMs - 1, true, false, hold, trigger, release);
        assert(t.state == XState::Original);
        hold = kButtonX; trigger = kButtonX; release = 0;
        trackX(t, 1000 + kPressDebounceMs, true, false, hold, trigger, release);
        assert(t.state == XState::NativeSwap);
    }

    // Other buttons and the idle frame change nothing.
    {
        XTracker t;
        uint32_t hold = 0x1, trigger = 0x1, release = 0;
        trackX(t, 0, true, false, hold, trigger, release);
        assert(t.state == XState::PluginSwap && hold == 0x1 && trigger == 0x1);
        hold = 0; trigger = 0;
        trackX(t, 0, true, false, hold, trigger, release);
        assert(t.state == XState::PluginSwap);
    }

    // Outside the Home, or with the config menu open, X is untouched and the state stays.
    {
        XTracker t;
        uint32_t hold = kButtonX, trigger = kButtonX, release = 0;
        trackX(t, 0, false, false, hold, trigger, release);      // game or other app
        assert(t.state == XState::PluginSwap && hold == kButtonX && trigger == kButtonX);
        trackX(t, 0, true, true, hold, trigger, release);        // config menu open
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
