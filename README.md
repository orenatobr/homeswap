# HomeSwap

An [Aroma](https://aroma.foryour.cafe) (WUPS) plugin that swaps the two screens **only in the Wii U Menu**: the Home shows on the TV and the Mii plaza shows on the GamePad. It starts active, with no button combo. Games and apps keep their normal screens, unlike a global screen swap.

## Install
1. Download `homeswap.wps` from the [latest release](https://github.com/orenatobr/homeswap/releases/latest).
2. Copy it to `wiiu/environments/aroma/plugins/` on the SD card.
3. Reboot the console and open the Home.

It needs the `MemoryMappingModule`, which comes with Aroma.

## Toggle with X
In the Wii U Menu, press **X** on the GamePad to turn the swap off: the Home goes back to the GamePad and the Mii plaza to the TV, the original behavior. Press **X** again to bring the Home back to the TV. The choice is not saved, so every boot starts with the swap on. X is ignored in games, in other apps and while the plugin menu is open.

## Settings
Open the Aroma plugin menu (L + Down + Minus). There is one item, "Home on the TV (Wii U Menu only)". While that menu is open the swap is off, so the menu shows on its usual screens.

## How it works
The plugin hooks `VPADRead` to catch X and `GX2CopyColorBufferToScanBuffer` to swap the TV and GamePad scan targets, but only when the running title is the Wii U Menu (`OSGetTitleID()` equals the title id from `_SYSGetSystemApplicationTitleId(SYSTEM_APP_ID_WII_U_MENU)`). Anti-aliased buffers are resolved first, as SwipSwapMe does. The decision logic lives in `source/logic.h` and has a host test.

## Limits
- **Touch:** the GamePad now shows the plaza, so touching it does not reach the Home icons that are on the TV. Use the D-pad, the stick and the buttons.
- **Audio** is not changed.
- If you also use SwipSwapMe, its swap combo stacks with this one: pressing it in the Home cancels the swap, pressing it again brings it back.

## Status
The author tested it on one console and the swap works in the Home. Please open an issue if it misbehaves on your setup, especially when moving between the Home, games and the HOME button menu.

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
