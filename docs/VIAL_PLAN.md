# Vial port plan (Nut65)

> **Status: done (2026-10-09).** This is the plan as written before the port. How it went:
> - The approach held: the board moved onto vial-qmk with a handful of mechanical API fixes (EEPROM datablock address, USB protocol state, UART names, keymap config accessors, the LED buffer name). The wireless stack needed no logic changes.
> - Raw HID: `md_raw.h`'s line-number redirect was replaced by a `send_raw_hid` on the wireless host driver. Vial works over USB and 2.4 GHz.
> - The debounce change became a board-local `hs_debounce.c`. The OEM edit had never applied the 1 ms setting at all.
> - Settings resetting on every flash turned out not to be VIA's or Vial's stamps. The OEM `QK_BOOT` handler called `eeconfig_disable()`.
> - Two upstream QMK bugs turned up and are patched in `patches/`: the reactive effects' offset overflow and Pixel Fractal writing to `NO_LED`.
> - See the README's "Bugs fixed along the way" for the details.

Goal: use Vial instead of VIA, so tap-hold keys, combos, key overrides, tap dance and macros can be changed live in the Vial app without reflashing. Snap Tap, KITT, the WPM meter, game mode, wireless and the light bar all keep working.

## What we're starting from (checked 2026-10-09)
- **The OEM tree** (hangshengkeji/qmk_firmware) is QMK from about Aug 2024 (0.26). Its git history is squashed into one commit, so the exact base can't be confirmed.
- **vial-qmk** (`vial` branch) last had a commit in Jul 2026. Its base is QMK May 2025 or later, about three QMK breaking-change cycles newer than the OEM tree. It already supports the `GENERIC_WB32_FQ95XX` board.
- **Good news:** the OEM wireless stack is self-contained in `keyboards/linker/wireless` and `keyboards/leku/nut65`. Outside the keyboard folder there are only two OEM core changes: `quantum/debounce/sym_defer_g.c` and `sym_eager_pr.c`, both for `HS_DEBOUNCE` (the Fn+D 1 ms / 8 ms toggle).
- **EEPROM** (4 KB logical, wear-levelled SPI flash):
  - Already used, about 1.5 KB: RGB record 540 B, board settings 30 B, our 3 B, VIA keymap 900 B, core settings.
  - That leaves about 2.5 KB for Vial's tap dance, combos, key overrides and QMK settings. Each entry is about 10 B, so 16 of each fits easily.
- **Recovery:** bootmagic is enabled, and the OEM `.bin` is in `firmware/`. Even a broken keymap can be reflashed.

## Approach
Move the **board** onto vial-qmk (option B) rather than moving Vial into the OEM tree (option A). Vial touches many core files, while the board code is self-contained. Option B also gets us a year of upstream QMK fixes.

## Phases

### 1. Setup
- Clone vial-qmk next to this repo and init its submodules.
- Copy in `keyboards/leku/nut65` (with `rgb_record/` and `wls/`) and `keyboards/linker/wireless`.
- Apply `patches/nut65-indicators-user-hook.patch`.
- Re-apply the two `HS_DEBOUNCE` debounce edits. Better: move them into a board-local custom debounce so the core stays untouched.

### 2. Make the stock board build on new QMK (the risky part)
- Build the board's `via` / `default` keymap and fix whatever QMK API changes break. Expected:
  - RGB keycode renames (`RGB_MOD` → `RM_NEXT`/`UG_NEXT`, and so on).
  - `RGB`/`HSV` → `rgb_t`/`hsv_t`.
  - `keyboard.json` schema changes.
  - Changed hook signatures.
- No feature changes in this phase, only getting it to compile.

### 3. Hardware check on the stock port (milestone A)
- Flash it and test:
  - **Connections:** wired, 2.4G, BT1–3 pairing and switching.
  - **Lights and power:** light-bar modes, battery indicator, charging LEDs, music mode, sleep and wake on each connection.
  - **Settings and memory:** Fn-layer functions, the Fn+D debounce toggle, factory reset, EEPROM persistence.
- If wireless or sleep misbehaves here, stop and compare against the OEM build before going further.

### 4. Turn on Vial
- **New Vial keymap:** create `keymaps/vial` with `VIAL_ENABLE = yes` in rules.mk.
- **Vial config:** add `vial.json` (convert from `NUT65.json`, same KLE layout), a random `VIAL_KEYBOARD_UID`, and `VIAL_UNLOCK_COMBO` (proposed: Esc + Enter).
- **Feature sizes:** pick counts for tap dance, combos and key overrides that fit the EEPROM budget.
- Confirm the board's `via_command_kb` (in `wls/wls.c`) passes Vial's `0xFE` commands through instead of swallowing them.

### 5. Port our features
- Bring over the Snap Tap / KITT / WPM / game-mode keymap code and the fixed build date.
- Keep the wake fix: `suspend_wakeup_init_user()` calls `lpwr_set_timeout_manual(false)`.
- `md_raw.h` is force-included and macro-redirects `raw_hid_send`, so check that Vial's raw-HID replies still reach the host over USB.
- Declare SNAP, KITT-/+ and GAME as `customKeycodes` in `vial.json` so they show by name in Vial.

### 6. Nice to have
- Vial working over 2.4G through the dongle (raw HID is already routed in `md_raw.c`); wired-only Vial is fine.
- Move parts of the Snap Tap settings into Vial's QMK Settings tab.

## Effort and risk
- Phases 1, 4 and 5 are routine.
- Phases 2 and 3 are the unknowns. Compile fixes are mechanical, but the wireless, low-power and sleep code was written against older ChibiOS/QMK behaviour and could act differently even when it compiles.
- **Estimate:** a day or two of work, several flash-and-test rounds, and you on hand to test wireless and sleep.

## Decisions (2026-10-09)
- Unlock combo: Esc + Enter.
- Keep the current VIA build in `firmware/` as the fallback.
