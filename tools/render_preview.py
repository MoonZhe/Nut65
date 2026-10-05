"""Render animated GIF previews of the KITT light-bar scanner.

Uses the same integer maths as kitt_render() in keymap/snaptap/keymap.c, so the
preview matches the keyboard frame for frame (the real bar is dimmer: it follows
the Fn+PgUp/PgDn light-bar brightness).

Usage: python tools/render_preview.py [out_dir]   (default: docs/)
Needs Pillow.
"""
import colorsys
import os
import sys

from PIL import Image, ImageDraw, ImageFilter

# --- Firmware constants (keymap/snaptap/keymap.c) ---------------------------
LEDS, HALF, SOFT = 80, 40, 8
TRAVEL = (HALF + SOFT) * 256
FLOOD_MS, DRAIN_MS, TRANSITION_MS = 1200, 1800, -300
SPEED_PCT = [175, 130, 100, 78, 60]
DEFAULT_SPEED = 1
HUES = {"Red": 0, "Amber": 20, "Blue": 170, "Green": 85, "Purple": 200, "Rainbow": None}


def ease(x):
    return x * x * (3 * 1024 - 2 * x) // (1024 * 1024)


def move(t, start, dur):
    x = int((t - start) * 1024 / dur)  # C integer division truncates toward zero
    x = max(0, min(1024, x))
    return ease(x) * TRAVEL // 1024


def soft(v):
    v = int(v / SOFT)
    return max(0, min(255, v))


def levels(t_ms, speed=DEFAULT_SPEED):
    pct = SPEED_PCT[speed]
    fl, dr = FLOOD_MS * pct // 100, DRAIN_MS * pct // 100
    gap = int(TRANSITION_MS * pct / 100)
    s_lo_in = fl + gap
    s_lo_out = s_lo_in + dr + gap
    s_hi_out = s_lo_out + fl + gap
    period = s_hi_out + dr + gap
    t = t_ms % period
    hi = TRAVEL - move(t + period, s_hi_out, dr) + move(t, 0, fl) - move(t, s_hi_out, dr)
    lo = -SOFT * 256 + move(t, s_lo_in, dr) - move(t, s_lo_out, fl)
    half = [soft(hi - d * 256) * soft(d * 256 - lo) // 255 for d in range(HALF)]
    return half + half[::-1], period


def qmk_hsv_to_rgb(h, s, v):
    """quantum/color.c hsv_to_rgb_impl without the CIE curve (as built)."""
    if s == 0:
        return v, v, v
    region = h * 6 // 255
    rem = (h * 2 - region * 85) * 3
    p = (v * (255 - s)) >> 8
    q = (v * (255 - ((s * rem) >> 8))) >> 8
    t = (v * (255 - ((s * (255 - rem)) >> 8))) >> 8
    return [(v, t, p), (q, v, p), (p, v, t), (p, q, v), (t, p, v), (v, p, q)][min(region, 5)]


def draw_bar(draw, x0, y0, width, height, lv, hue, tick):
    pitch = width / LEDS
    for i, level in enumerate(lv):
        h = (tick + i * 3) % 256 if hue is None else hue
        r, g, b = qmk_hsv_to_rgb(h, 255, level)
        x = x0 + i * pitch
        draw.rounded_rectangle([x + 1, y0, x + pitch - 1, y0 + height], radius=2, fill=(r, g, b))


def render(rows, path, width, bar_h, row_gap, frame_ms, label=False):
    pad = 24
    label_w = 90 if label else 0
    height = pad * 2 + len(rows) * bar_h + (len(rows) - 1) * row_gap
    _, period = levels(0)
    frames = []
    for t in range(0, period, frame_ms):
        lv, _ = levels(t)
        tick = (t // 20) % 256
        led = Image.new("RGB", (width, height), (8, 8, 8))
        d = ImageDraw.Draw(led)
        for n, (name, hue) in enumerate(rows):
            y = pad + n * (bar_h + row_gap)
            draw_bar(d, pad + label_w, y, width - 2 * pad - label_w, bar_h, lv, hue, tick)
        # Soft bloom so it reads like light, not paint.
        glow = led.filter(ImageFilter.GaussianBlur(9))
        img = Image.blend(led, glow, 0.35)
        img = Image.composite(led, img, led.convert("L").point(lambda p: 255 if p > 40 else 0))
        q = img.quantize(colors=200, method=Image.Quantize.MEDIANCUT)
        if label:
            # Draw labels after quantising, in a reserved palette slot, so the
            # grey text isn't snapped to the nearest LED colour.
            pal = q.getpalette()[: 256 * 3]
            pal += [0] * (256 * 3 - len(pal))
            pal[255 * 3 : 256 * 3] = [150, 150, 145]
            q.putpalette(pal)
            dq = ImageDraw.Draw(q)
            for n, (name, _) in enumerate(rows):
                dq.text((pad, pad + n * (bar_h + row_gap) + bar_h / 2 - 6), name, fill=255)
        frames.append(q)
    frames[0].save(path, save_all=True, append_images=frames[1:], duration=frame_ms, loop=0, optimize=True, disposal=1)
    print(f"{path}: {len(frames)} frames, {os.path.getsize(path) // 1024} KB, cycle {period} ms")


def main(out_dir="docs"):
    os.makedirs(out_dir, exist_ok=True)
    render([("Red", HUES["Red"])], os.path.join(out_dir, "kitt-red.gif"), 900, 26, 0, 40)
    render(list(HUES.items()), os.path.join(out_dir, "kitt-variants.gif"), 760, 16, 14, 60, label=True)


if __name__ == "__main__":
    main(*sys.argv[1:2])
