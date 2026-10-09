"""Build the Vial keymaps from the shared SOCD keymap sources.

keymap.c, layers.inc and rgb_matrix_user.inc are shared with the VIA build
(keymap/socd). Vial only adds a few config.h lines, its own rules.mk, and
vial.json (the VIA definition, which Vial reads as-is). Two keymaps come out:

  vial           Every lighting effect and light-bar mode, plus KITT Sweep.
  vial_personal  Trimmed lighting: keys get Solid Reactive Simple, KITT Sweep,
                 Solid Color, Breathing and Typing Heatmap; the light bar gets
                 the solid colours, KITT, WPM and off.

Usage: python sync_vial_keymap.py <vial-qmk>/keyboards/leku/nut65
"""
import json
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent.parent
SRC = HERE / 'keymap' / 'socd'

RULES = """VIA_ENABLE = yes
VIAL_ENABLE = yes
ENCODER_MAP_ENABLE = yes
WPM_ENABLE = yes
# Vial's Lighting tab (it ignores VIA's lighting menus). Lists QMK's built-in
# effects only; KITT Sweep stays on Fn+RGB_MOD.
VIALRGB_ENABLE = yes
"""

VIAL_CONFIG = """
// Vial only: EEPROM layout version for keeping the Vial layout across flashes
// (see via_init_kb in keymap.c). Bump it whenever a change moves EEPROM data:
// layer count, Vial feature counts, or these user bytes.
#define VIAL_PERSIST_EEPROM_ADDR (USER_FLAGS_EEPROM_ADDR + 3)
#define VIAL_PERSIST_VERSION 1

#define VIAL_KEYBOARD_UID {0xD4, 0xE6, 0xBC, 0x50, 0x9E, 0x80, 0x79, 0x55}

// Hold Esc + Enter to unlock Vial's protected settings.
#define VIAL_UNLOCK_COMBO_ROWS { 0, 2 }
#define VIAL_UNLOCK_COMBO_COLS { 0, 13 }

// Newer QMK's LED frame buffer, read by the Bar Echo effect.
#define NUT65_LED_BUFFER ws2812_leds
#define NUT65_LED_TYPE   ws2812_led_t
"""

# Key effects the personal build keeps.
PERSONAL_EFFECTS = ('solid_color', 'solid_reactive_simple', 'breathing', 'typing_heatmap')
PERSONAL_CYCLE = ('RGB_MATRIX_SOLID_REACTIVE_SIMPLE', 'RGB_MATRIX_CUSTOM_KITT_REACTIVE', 'RGB_MATRIX_CUSTOM_KITT_SWEEP',
                  'RGB_MATRIX_CUSTOM_KITT_1982', 'RGB_MATRIX_CUSTOM_BAR_ECHO', 'RGB_MATRIX_SOLID_COLOR', 'RGB_MATRIX_BREATHING',
                  'RGB_MATRIX_TYPING_HEATMAP')


def personal_config(board):
    animations = json.loads((board / 'keyboard.json').read_text())['rgb_matrix']['animations']
    dropped = sorted(name for name, on in animations.items() if on and name not in PERSONAL_EFFECTS)
    lines = ['', '// Personal build: trimmed lighting.', '#define NUT65_BAR_SOLIDS_ONLY', '#undef NUT65_RGB_EXTRA_MODE',
             '#define NUT65_RGB_CYCLE {' + ', '.join(PERSONAL_CYCLE) + '}']
    lines += [f'#undef ENABLE_RGB_MATRIX_{name.upper()}' for name in dropped]
    return '\n'.join(lines) + '\n'



def write_if_changed(path, text):
    """Leave unchanged files alone so their timestamps don't force a full rebuild."""
    if not path.exists() or path.read_text() != text:
        path.write_text(text, newline='\n')


def write_keymap(dest, extra_config=''):
    dest.mkdir(parents=True, exist_ok=True)
    for name in ('keymap.c', 'layers.inc', 'rgb_matrix_user.inc'):
        write_if_changed(dest / name, (SRC / name).read_text().replace('\r\n', '\n'))
    definition = json.loads((HERE / 'via' / 'NUT65_socd.json').read_text())
    definition['lighting'] = 'vialrgb'
    write_if_changed(dest / 'vial.json', json.dumps(definition, indent=2) + '\n')
    write_if_changed(dest / 'rules.mk', RULES)

    config = (SRC / 'config.h').read_text().replace('\r\n', '\n')
    size = '(EECONFIG_RGBREC_USE_SIZE + EECONFIG_CONFINFO_USE_SIZE + 3)'
    assert size in config
    config = config.replace(size, size.replace('+ 3)', '+ 4)'))  # One more byte: VIAL_PERSIST_VERSION.
    anchor = '#define USER_MODES_EEPROM_ADDR (USER_FLAGS_EEPROM_ADDR + 2)\n'
    assert anchor in config
    config = config.replace(anchor, anchor + VIAL_CONFIG) + extra_config
    write_if_changed(dest / 'config.h', config)
    print(f'Vial keymap written to {dest}')


def main(board):
    board = Path(board)
    write_keymap(board / 'keymaps' / 'vial')
    write_keymap(board / 'keymaps' / 'vial_personal', personal_config(board))


if __name__ == '__main__':
    main(sys.argv[1])
