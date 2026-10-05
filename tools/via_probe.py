"""Read the Nut65's live keymap over VIA raw HID.

Usage: uv run --with hidapi python tools/via_probe.py
Prints the protocol version and the keycodes on the Fn layers for G, K and RShift.
"""
import hid

VID, PID = 0x342D, 0xE51A
USAGE_PAGE, USAGE = 0xFF60, 0x61

ID_GET_PROTOCOL_VERSION = 0x01
ID_DYNAMIC_KEYMAP_GET_KEYCODE = 0x04


def open_via():
    for d in hid.enumerate(VID, PID):
        if d["usage_page"] == USAGE_PAGE and d["usage"] == USAGE:
            h = hid.device()
            h.open_path(d["path"])
            return h
    raise SystemExit("VIA raw HID interface not found (close VIA in the browser?)")


def xfer(h, *payload):
    buf = bytes([0x00, *payload]) + bytes(32 - len(payload))  # report id + 32 bytes
    h.write(buf)
    return h.read(32, 1000)


def main():
    h = open_via()
    r = xfer(h, ID_GET_PROTOCOL_VERSION)
    print("VIA protocol:", hex((r[1] << 8) | r[2]))
    keys = {"G": (2, 5), "K": (2, 8), "RShift": (3, 12), "Esc": (0, 0)}
    for layer in (0, 1, 3):
        out = []
        for name, (row, col) in keys.items():
            r = xfer(h, ID_DYNAMIC_KEYMAP_GET_KEYCODE, layer, row, col)
            out.append(f"{name}=0x{(r[4] << 8) | r[5]:04X}")
        print(f"layer {layer}: " + "  ".join(out))
    h.close()


if __name__ == "__main__":
    main()
