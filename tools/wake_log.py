"""Read the wake-debug event log from the Nut65 over VIA raw HID (wired mode).

Usage: uv run --with hidapi python tools/wake_log.py [--elf path/to/firmware.elf] [--clear]
Needs the WAKE_DEBUG=yes firmware. With --elf, caller addresses are resolved to
function names using arm-none-eabi-addr2line from QMK MSYS.
"""
import argparse
import os
import shutil
import struct
import subprocess

import hid

VID, PID = 0x342D, 0xE51A
USAGE_PAGE, USAGE = 0xFF60, 0x61
CMD_READ, CMD_CLEAR = 0xF0, 0xF1
ADDR2LINE_CANDIDATES = [
    shutil.which("arm-none-eabi-addr2line"),
    r"C:\QMK_MSYS\mingw64\bin\arm-none-eabi-addr2line.exe",
]

LPWR = {0: "NORMAL", 1: "PRESLEEP", 2: "STOP", 3: "WAKEUP"}
WAKECD = {0: "none", 1: "key", 2: "radio(UART)", 3: "cable", 4: "USB", 5: "onekey", 6: "encoder", 7: "switch"}
MD = {0: "none", 1: "pairing", 2: "connected", 3: "disconnected", 4: "reject"}  # module.h
DEVS = {0: "USB", 1: "BT1", 2: "BT2", 3: "BT3", 4: "BT4", 5: "BT5", 6: "2.4G"}


def open_via():
    for d in hid.enumerate(VID, PID):
        if d["usage_page"] == USAGE_PAGE and d["usage"] == USAGE:
            h = hid.device()
            h.open_path(d["path"])
            return h
    raise SystemExit("VIA raw HID interface not found: plug in USB, switch to wired (Fn+T), close VIA.")


def xfer(h, *payload):
    h.write(bytes([0x00, *payload]) + bytes(32 - len(payload)))
    return bytes(h.read(32, 1000))


def addr2line(elf, addrs):
    tool = next((t for t in ADDR2LINE_CANDIDATES if t and os.path.exists(t)), None)
    if not elf or not tool or not addrs:
        return {}
    out = subprocess.run([tool, "-f", "-s", "-e", elf] + [hex(a & ~1) for a in addrs], capture_output=True, text=True).stdout.split("\n")
    return {a: f"{out[2 * i]} ({out[2 * i + 1]})" for i, a in enumerate(addrs)}


def describe(ev, names):
    t, typ, a, md, devs, extra = ev
    caller = names.get(extra, hex(extra))
    if typ == 1:
        return f"lpwr state -> {LPWR.get(a, a)}   [from {caller}]"
    if typ == 2:
        return f"woken by {WAKECD.get(a, a)}"
    if typ == 3:
        return f"sleep request (manual timeout = {a})   [from {caller}]"
    if typ == 4:
        return f"receiver says host {'RESUMED' if a else 'SUSPENDED'} (lpwr state {LPWR.get(extra, extra)})"
    if typ == 5:
        return f"sleep timeout fired (manual={a}, idle {extra} ms)"
    if typ == 6:
        return f"radio module state {MD.get(extra, extra)} -> {MD.get(a, a)}"
    if typ == 7:
        return f"key {'down' if a else 'up'} at row {extra >> 8}, col {extra & 0xFF}"
    if typ == 8:
        return "suspend_wakeup_init (lights/USB wake path)"
    if typ == 9:
        return "suspend_power_down (lights off)"
    if typ == 10:
        return "BOOT"
    if typ == 11:
        bits = [n for b, n in ((1, "rgb-on"), (2, "rgb-SUSPENDED"), (4, "led-power"), (8, "led-boost")) if a & b]
        return f"lighting: {', '.join(bits) or 'all off'} (val {extra >> 24}, idle {extra & 0xFFFFFF} ms)"
    return f"type {typ} a={a} extra={extra:#x}"


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--elf")
    ap.add_argument("--clear", action="store_true")
    args = ap.parse_args()
    h = open_via()
    if args.clear:
        xfer(h, CMD_CLEAR)
        print("log cleared")
        return
    events, kept, i, total, now = [], 1, 0, 0, 0
    while i < kept:
        r = xfer(h, CMD_READ, i)
        if r[0] != CMD_READ:
            raise SystemExit("Unexpected reply: is the WAKE_DEBUG firmware flashed?")
        total, kept = r[1] | r[2] << 8, r[3]
        now = struct.unpack_from("<I", r, 28)[0]
        for n in range(2):
            if i + n < kept:
                events.append(struct.unpack_from("<IBBBBI", r, 4 + 12 * n))
        i += 2
    h.close()
    callers = sorted({e[5] for e in events if e[1] in (1, 3) and e[5] > 0x08000000})
    names = addr2line(args.elf, callers)
    print(f"{total} events since boot/clear, showing last {len(events)}; keyboard clock now {now} ms\n")
    prev = None
    for ev in events:
        gap = "" if prev is None else f"+{ev[0] - prev:>7} ms"
        prev = ev[0]
        print(f"{ev[0]:>10} {gap:>11}  [{DEVS.get(ev[4], ev[4]):>4} {MD.get(ev[3], ev[3]):>12}]  {describe(ev, names)}")


if __name__ == "__main__":
    main()
