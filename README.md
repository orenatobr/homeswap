# HomeSwap

An [Aroma](https://aroma.foryour.cafe) (WUPS) plugin that swaps the two screens **only in the Wii U Menu**: the Home shows on the TV and the Mii plaza shows on the GamePad. It starts active, with no button combo. Games and apps keep their normal screens, unlike a global screen swap.

## Install
1. Download `homeswap.wps` from the [latest release](https://github.com/orenatobr/homeswap/releases/latest).
2. Copy it to `wiiu/environments/aroma/plugins/` on the SD card.
3. Reboot the console and open the Home.

It needs the `MemoryMappingModule`, which comes with Aroma.

## Cycle the screens with X
In the Wii U Menu, **X** on the GamePad cycles three layouts, in this order:

| State | TV | GamePad |
|-------|----|---------|
| A (every boot starts here) | Home, streamed from the GamePad's own picture (the plugin swaps the screens) | Mii plaza |
| B | Mii plaza | Home (the original behavior) |
| C | Home, drawn by the menu for the TV (the menu's own X swap) | Mii plaza |

A -> B -> C -> A. The plugin owns X in the Home: your press never reaches the menu. In A the plugin swaps the screens itself. In B the plugin stops swapping. Going to C and back to A, the plugin presses X for the menu, so the menu's own swap turns on (B -> C) and off (C -> A). The choice is not saved, so every boot starts in A. The plugin assumes the menu starts in its normal layout; if the two ever drift apart, keep pressing X until the screens match the table. X is untouched in games, in other apps and while the plugin menu is open. X only moves the screens: the GamePad keeps full control of the Home in every state.

## Settings
Open the Aroma plugin menu (L + Down + Minus). There is one item, "Home on the TV (Wii U Menu only)". While that menu is open the swap is off, so the menu shows on its usual screens.

## How it works
The plugin hooks `VPADRead` to catch X (and hide it from the menu in state A) and `GX2CopyColorBufferToScanBuffer` to swap the TV and GamePad scan targets, but only when the running title is the Wii U Menu (`OSGetTitleID()` equals the title id from `_SYSGetSystemApplicationTitleId(SYSTEM_APP_ID_WII_U_MENU)`). Anti-aliased buffers are resolved first, as SwipSwapMe does. The decision logic lives in `source/logic.h` and has a host test.

## Limits
- **Touch is untested.** The GamePad shows the plaza, so there is no cursor on the TV. Navigate the Home with the D-pad, the stick and the buttons. The Home reads touch as GamePad coordinates, so touching the GamePad may act on the matching spot of the TV, blind; nobody has checked this yet.
- **Audio** is not changed.
- If you also use SwipSwapMe, its swap combo stacks with this one: pressing it in the Home cancels the swap, pressing it again brings it back.
- State C depends on the menu's own X swap. It is untested on a console.

## Status
The author tested it on one console. The swap works in the Home, and opening a game, returning to the Home, pressing the HOME button inside a game, and opening the eShop and the browser from the Home all behave normally. Touch is untested (see Limits). v0.3.0 made X lock the GamePad and kept the Home on the TV; that was a misreading and v0.3.1 went back to the screen swap. v0.4.0 adds the third layout (C), which needs a check on hardware: it relies on the menu's own X swapping the screens. v0.4.1 restarts the cycle in A every time the Wii U Menu starts, so a menu reopened after a game no longer inherits a stale layout. v0.4.2 stops passing your own X press to the menu and sends the menu a fixed press instead, so a press can no longer leak and put the menu out of step with the plugin. The plugin also writes what it sees from the GamePad (`seen_calls`, `seen_x_presses`, `seen_x_state`) to its config file `HomeSwap.json` on the SD card, to help check a console that misbehaves. Please open an issue if it misbehaves on your setup.

## Recovery
Turn the item off in the plugin menu, or delete `homeswap.wps` from the plugins folder.

## Build
```
docker build . -t homeswap_builder
docker run --rm -v "$PWD":/project homeswap_builder make
```
Build from an APFS/ext4 folder: `._*` files on exFAT break `docker build`.

Host test for the decision logic: `c++ -std=c++17 tests/logic_test.cpp -o /tmp/lt && /tmp/lt`.

## Credits and license
GPL-3.0. The screen swap and the anti-aliasing workaround follow [SwipSwapMe](https://github.com/Maschell/SwipSwapMe) by Maschell. The "only for one title" filter follows [Better Settings](https://github.com/Fangal-Airbag/Better-Settings) by Fangal.
