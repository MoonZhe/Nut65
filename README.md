# Nut65 Snap Tap

Razer-style Snap Tap (last-input-priority SOCD) on **A/D** for the Weikav/LEKU Nut65, built on the OEM QMK source
([hangshengkeji/qmk_firmware](https://github.com/hangshengkeji/qmk_firmware) `keyboards/leku/nut65`). VIA keeps working.

| Path | What |
| --- | --- |
| `firmware/leku_nut65_snaptap.bin` | Snap Tap firmware, your VIA layout baked in as the default |
| `firmware/leku_nut65_default.bin` | Unmodified OEM source build, used as the fallback |
| `keymap/snaptap/` | Keymap source (copied into `qmk_firmware/keyboards/leku/nut65/keymaps/snaptap`) |
| `patches/nut65-indicators-user-hook.patch` | Board patch the keymap needs: user RGB hook plus light-bar accessors. Apply it to the OEM tree with `git apply` |
| `via/NUT65_snaptap.json` | VIA definition: load it under VIA's Settings → "Design" tab so the toggle shows as `SNAP` (CUSTOM(30)) |
| `via/nut65.snaptap.layout.json` | Your saved layout plus Fn+G = SNAP, Fn+, / Fn+. = KITT-/KITT+ and Fn+RShift = MO(4) |
| `nut65.layout.json` | Original VIA layout backup (source for `layers.inc`) |
| `tools/gen_layers.py` | Regenerates `keymap/snaptap/layers.inc` from a VIA layout |

## Use
- **Fn+G** toggles Snap Tap. A and D flash **green** when it turns on and **red** when it turns off. The setting survives power cycles and is **off** after a fresh flash.
- Turn it off for CS2 on Valve servers.
- **Fn+Insert** cycles the light bar. After the board's solid white it steps through a 2008-style KITT scanner in red, amber, blue, green, purple and rainbow, then off. The scanner runs a four-phase cycle: light floods in from both ends, drains into the middle, floods back out from the middle, then drains out to the ends. Each moving edge is soft. It follows Fn+PgUp/PgDn brightness and is remembered across power cycles.
- **Fn+,** / **Fn+.** make the scanner slower / faster (5 speeds, default 2nd slowest, remembered). The key blinks white once per step, or red three times at the slowest / fastest speed. Tune `KITT_FLOOD_MS`, `KITT_DRAIN_MS`, `KITT_TRANSITION_MS` (negative = phases overlap, currently -300), `kitt_speed_pct`, `KITT_SOFT` and `kitt_hues` in `keymap.c`.
- **Fn+D** (hold 3 s) toggles low-latency debounce: the D key blinks red x3 for 1 ms (low latency) or white x3 for 8 ms (normal, the default).
- **Fn+Left Win** toggles layer 2, the gaming layer. There Left Win becomes Fn, so Win+1 sends F1.

## Flash (wired mode, Fn+T)
1. Install the WB32 driver from https://github.com/WestberryTech/wb32-dfu-updater.
2. Enter DFU with **Fn + Right Shift + Esc**. The keyboard should show up as `342D:DFA0`.
3. Run:
   ```
   wb32-dfu-updater_cli.exe -t -s 0x08000000 -D firmware\leku_nut65_snaptap.bin
   wb32-dfu-updater_cli.exe -R
   ```
4. Flashing resets VIA's EEPROM, so the keyboard boots into the layout baked into the firmware. RGB settings go back to their defaults.

## Build from source (QMK MSYS)
```
git clone https://github.com/hangshengkeji/qmk_firmware.git
cd qmk_firmware
git checkout 3164abd3            # OEM tree this was built and tested against
git submodule update --init --depth 1 lib/chibios lib/chibios-contrib lib/printf lib/lufa lib/vusb
git apply ../Nut65/patches/nut65-indicators-user-hook.patch
cp -r ../Nut65/keymap/snaptap keyboards/leku/nut65/keymaps/
make leku/nut65:snaptap
```
To change the base layout, export it from VIA and regenerate the layers:
`python tools/gen_layers.py nut65.layout.json <qmk_firmware>/keyboards/leku/nut65/keyboard.json > keymap/snaptap/layers.inc`
