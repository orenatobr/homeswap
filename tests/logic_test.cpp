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

    // The plugin owns X in the Home: a physical hold never reaches the menu on its own, in any
    // state. The menu only sees the press the plugin sends (none in A, a fixed one in B and C).
    for (XState from : {XState::PluginSwap, XState::Original, XState::NativeSwap}) {
        XTracker t{from, 0};
        int held = 0, triggers = 0, releases = 0;
        for (int i = 0; i < 40; i++) { // X held for 30 samples, then let go
            uint32_t hold = (i < 30) ? (kButtonX | 0x1) : 0;
            uint32_t trigger = (i == 0) ? kButtonX : 0;
            uint32_t release = (i == 30) ? kButtonX : 0;
            trackX(t, true, false, hold, trigger, release);
            held += (hold & kButtonX) ? 1 : 0;
            triggers += (trigger & kButtonX) ? 1 : 0;
            releases += (release & kButtonX) ? 1 : 0;
            if (i < 30) {
                assert(hold & 0x1); // other buttons pass
            }
        }
        assert(t.state == nextXState(from));
        if (from == XState::PluginSwap) {
            assert(held == 0 && triggers == 0 && releases == 0);
        } else {
            assert(held == kInjectSamples && triggers == 1 && releases == 1);
        }
    }

    // A -> B: nothing is sent to the menu (the plugin only stops swapping).
    {
        XTracker t;
        uint32_t hold = kButtonX, trigger = kButtonX, release = 0;
        trackX(t, true, false, hold, trigger, release);
        assert(t.state == XState::Original && t.inject == 0);
        for (int i = 0; i < 20; i++) {
            hold = 0; trigger = 0; release = 0;
            trackX(t, true, false, hold, trigger, release);
            assert(hold == 0 && trigger == 0 && release == 0);
        }
    }

    // B -> C and C -> A: the plugin presses X for the menu so the menu runs its own swap:
    // one trigger sample, X held for kInjectSamples samples, then one release sample.
    for (XState from : {XState::Original, XState::NativeSwap}) {
        XTracker t{from, 0};
        uint32_t hold = kButtonX, trigger = kButtonX, release = 0;
        trackX(t, true, false, hold, trigger, release);         // first sample of the press
        assert(hold == kButtonX && trigger == kButtonX && release == 0);
        int held = 1, triggers = 1, releases = 0;
        for (int i = 0; i < 40; i++) {
            hold = 0; trigger = 0; release = 0;
            trackX(t, true, false, hold, trigger, release);     // the physical samples afterwards
            held += (hold & kButtonX) ? 1 : 0;
            triggers += (trigger & kButtonX) ? 1 : 0;
            releases += (release & kButtonX) ? 1 : 0;
        }
        assert(held == kInjectSamples && triggers == 1 && releases == 1);
        assert(t.inject == 0 && t.state == nextXState(from));
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
