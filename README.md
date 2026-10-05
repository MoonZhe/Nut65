# KITT scanner + Snap Tap for the Weikav Nut65

A custom QMK firmware for the **Weikav / LEKU Nut65** (65%, tri-mode) that turns the keyboard's front light bar into a **2008-style KITT scanner** (Knight Industries Three Thousand), and adds **Razer-style Snap Tap** for gaming. VIA keeps working, and wired, Bluetooth and 2.4 GHz all still work.

![KITT scanner on the Nut65 light bar](docs/kitt-red.gif)

<sub>Rendered from the firmware's own animation code, frame for frame. On the keyboard it follows the light-bar brightness keys.</sub>

## The scanner

The original 1982 KITT scanner is a single light bouncing back and forth. The 2008 KI3000 scanner instead **floods and drains in both directions**. The timing here was measured frame by frame from a [KI3000 scanner replica](https://www.youtube.com/watch?v=Lz5OhpBDkkE) and rebuilt on the keyboard's 80-LED bar:

1. **Flood in:** light pours in from both ends and meets in the middle.
2. **Drain to the middle:** the ends go dark first, and the light collapses into the centre.
3. **Flood out:** light pours back out from the centre to the ends.
4. **Drain to the ends:** a dark gap opens in the middle and spreads outward.

Each phase starts 300 ms before the previous one finishes, so the movement never stops. Every moving edge has an 8-LED soft fade, and the bar is drawn per LED, so everything meets at the true centre. That matters because the board groups the bar into 15 uneven segments.

It comes in **red, amber, blue, green, purple and rainbow**, with **5 speeds**:

![All six colour variants](docs/kitt-variants.gif)

## Controls

| Keys | What it does |
| --- | --- |
| **Fn + Insert** | Cycles the light bar. After the stock effects and solid colours come KITT red, amber, blue, green, purple and rainbow, then off. Remembered across power cycles. |
| **Fn + ,** / **Fn + .** | KITT slower / faster. The key blinks white for each step and red ×3 at the slowest or fastest speed. Remembered. |
| **Fn + PgUp / PgDn** | Light-bar brightness. The scanner follows it. |
| **Fn + G** | Snap Tap on A/D on or off. A and D flash green (on) or red (off). Remembered. |
| **Fn + D** (hold 3 s) | Low-latency debounce: 1 ms (D blinks red) or 8 ms (D blinks white, the default). |
| **Fn + Right Shift + Esc** | Bootloader (DFU), for flashing. |

### Snap Tap
This is last-input priority on **A** and **D**: pressing D while A is held releases A, and letting go of D re-presses A if it's still held. It's the same behaviour as Razer's Snap Tap and Wooting's SOCD. Turn it off for **CS2 on Valve servers**, which kick players for hardware SOCD.

## Flashing

> Flashing custom firmware is at your own risk and may void your warranty. The Nut65's WB32 bootloader can't be overwritten by a flash, so a bad flash is recoverable: re-flash, or go back with `firmware/leku_nut65_default.bin`.

1. Plug in USB and switch to wired mode (**Fn + T**). Close VIA and the Weikav web driver.
2. If Windows doesn't recognise the bootloader, install the WB32 DFU driver from [WestberryTech/wb32-dfu-updater](https://github.com/WestberryTech/wb32-dfu-updater).
3. Enter DFU with **Fn + Right Shift + Esc**. The keyboard goes dark and shows up as `342D:DFA0`.
4. Flash it. `wb32-dfu-updater_cli` comes with [QMK MSYS](https://msys.qmk.fm/):
   ```
   wb32-dfu-updater_cli -t -s 0x08000000 -D firmware/leku_nut65_snaptap.bin
   wb32-dfu-updater_cli -R
   ```
5. Flashing resets VIA's stored settings, so the keyboard boots into the layout compiled into the firmware. **That layout is mine** (see `nut65.layout.json`). Remap freely in VIA afterwards; load `via/NUT65_snaptap.json` under VIA's Design tab so the new keys show up as `SNAP`, `KITT-` and `KITT+`.

## Building from source

Built on the Nut65's OEM QMK source, using [QMK MSYS](https://msys.qmk.fm/):

```
git clone https://github.com/hangshengkeji/qmk_firmware.git
cd qmk_firmware
git checkout 3164abd3            # OEM tree this was built and tested against
git submodule update --init --depth 1 lib/chibios lib/chibios-contrib lib/printf lib/lufa lib/vusb
git apply ../Nut65/patches/nut65-indicators-user-hook.patch
cp -r ../Nut65/keymap/snaptap keyboards/leku/nut65/keymaps/
make leku/nut65:snaptap
```

To use your own layout as the default, export it from VIA and regenerate the layers:

```
python tools/gen_layers.py your.layout.json <qmk_firmware>/keyboards/leku/nut65/keyboard.json > keymap/snaptap/layers.inc
```

To tune the scanner, edit `keymap/snaptap/keymap.c`:

| Setting | What it changes |
| --- | --- |
| `KITT_FLOOD_MS` / `KITT_DRAIN_MS` | Phase lengths |
| `KITT_TRANSITION_MS` | Overlap between phases (negative) or a pause (positive) |
| `KITT_SOFT` | Edge softness |
| `kitt_speed_pct` | Speed levels |
| `kitt_hues` | Colours |

Afterwards, `python tools/render_preview.py` re-renders the GIFs above from the same maths.

### What's in here

| Path | What |
| --- | --- |
| `firmware/` | Ready-to-flash builds: `leku_nut65_snaptap.bin` (this firmware) and `leku_nut65_default.bin` (unmodified OEM source, the fallback) |
| `keymap/snaptap/` | Keymap source: Snap Tap, the KITT scanner, speed keys and the layout |
| `patches/` | Small board patch: lets the keymap draw over the board's light-bar modes, and adds read-only light-bar getters |
| `via/` | VIA definition with the new keys, plus the matching layout |
| `tools/` | Layout generator, GIF renderer, and a VIA probe that reads the live keymap |

### How it hooks in
- The board already owns `process_record_user`, so the keymap uses `pre_process_record_user`.
- QMK draws `rgb_matrix_indicators_user` before the board's own light-bar code, which then paints over it. The patch makes the board's `rgb_matrix_indicators_advanced_kb` end by calling `rgb_matrix_indicators_advanced_user`, so the scanner is drawn last.
- The KITT variants slot into the stock Fn+Insert cycle between solid white and off. While a variant is showing, the board stays in its white mode underneath, so the brightness keys keep working.
- Settings live in two bytes added to the end of the board's user EEPROM block.

## Credits
- Weikav / [hangshengkeji](https://github.com/hangshengkeji/qmk_firmware) for publishing the Nut65 QMK source.
- [Pascal Getreuer's SOCD Cleaner](https://getreuer.info/posts/keyboards/socd-cleaner/index.html), the basis of the Snap Tap logic.
- [Knight Research](https://www.youtube.com/watch?v=Lz5OhpBDkkE) for the KI3000 scanner replica the animation was measured from.
- [bhctsntrk/nut65-signalrgb](https://github.com/bhctsntrk/nut65-signalrgb) for documenting Nut65 flashing and recovery.

## License
GPL-2.0-or-later, the same as QMK. See [LICENSE](LICENSE).

<sub>A fan project. Not affiliated with or endorsed by Weikav, Razer, or the owners of Knight Rider. "Snap Tap" is Razer's name for the feature; KITT and Knight Rider belong to their respective owners.</sub>
