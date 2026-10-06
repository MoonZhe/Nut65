"""Convert a VIA saved layout (.layout.json) into QMK LAYOUT(...) blocks for the Nut65.

VIA stores each layer in matrix order (rows * cols); QMK's LAYOUT macro takes the
82 physical keys in keyboard.json order, so we remap through that list.
Usage: python gen_layers.py <via_layout.json> <keyboard.json> > keymap/snaptap/layers.inc
"""
import json
import sys

# VIA names that differ from the keycode names this QMK version accepts.
RENAMES = {
    "KC_GESC": "QK_GESC",
    "KC_SLCK": "KC_SCRL",
    "MAGIC_TOGGLE_NKRO": "NK_TOGG",
    "RESET": "QK_BOOT",
    "KC_TRNS": "_______",
}

# Extra bindings layered on top of the saved layout: (layer, row, col) -> keycode.
# Fn+G toggles Snap Tap on both the Windows (1) and Mac (3) Fn layers.
OVERRIDES = {
    (1, 2, 5): "SNAP_TOG",
    (3, 2, 5): "SNAP_TOG",
    # Fn+, / Fn+. slow down / speed up the Knight Rider light bar.
    (1, 3, 9): "KITT_SLOW",
    (3, 3, 9): "KITT_SLOW",
    (1, 3, 10): "KITT_FAST",
    (3, 3, 10): "KITT_FAST",
    # Fn+Left Win toggles game mode (replaces the old TG(2) gaming layer).
    (1, 4, 1): "GAME_TOG",
    (3, 4, 1): "GAME_TOG",
    # Fn+RShift -> layer 4 (stock behaviour). The saved layout left this
    # transparent, which made layer 4's QK_BOOT (Fn+RShift+Esc) unreachable.
    (1, 3, 12): "MO(4)",
    (3, 3, 12): "MO(4)",
}


def main(via_path, kb_path):
    via = json.load(open(via_path))
    kb = json.load(open(kb_path))
    cols = len(kb["matrix_pins"]["cols"])
    custom = [k["key"] for k in kb["keycodes"]]
    positions = [tuple(k["matrix"]) for k in kb["layouts"]["LAYOUT"]["layout"]]

    def name(code):
        if code.startswith("CUSTOM("):
            return custom[int(code[7:-1])]
        return RENAMES.get(code, code)

    out = []
    for li, layer in enumerate(via["layers"]):
        keys = []
        for r, c in positions:
            keys.append(OVERRIDES.get((li, r, c)) or name(layer[r * cols + c]))
        rows, row = [], []
        last_r = positions[0][0]
        for (r, _), k in zip(positions, keys):
            if r != last_r:
                rows.append(row)
                row, last_r = [], r
            row.append(f"{k + ',':<10}")
        rows.append(row)
        body = "\n".join("        " + " ".join(rw).rstrip() for rw in rows).rstrip(",")
        out.append(f"    [{li}] = LAYOUT(\n{body}\n    ),")
    print("\n".join(out))


if __name__ == "__main__":
    main(*sys.argv[1:3])
