"""Render animated GIF previews of the KITT light-bar scanner.

Uses the same integer maths as kitt_render() and kitt82_begin() (the 1982
scanner) in keymap/socd/keymap.c, so the
preview matches the keyboard frame for frame (the real bar is dimmer: it follows
the Fn+PgUp/PgDn light-bar brightness).

Usage: python tools/render_preview.py [out_dir]   (default: docs/)
Needs Pillow.
"""
import colorsys
import os
import sys

from PIL import Image, ImageDraw, ImageFilter

# --- Firmware constants (keymap/socd/keymap.c) ---------------------------
LEDS, HALF, SOFT = 80, 40, 8
TRAVEL = (HALF + SOFT) * 256
FLOOD_MS, DRAIN_MS, TRANSITION_MS = 1200, 1800, -300
SPEED_PCT = [175, 130, 100, 78, 60]
DEFAULT_SPEED = 1
HUES = {"Red": 0, "Amber": 20, "Blue": 170, "Green": 85, "Purple": 200, "Rainbow": None}
K82_LAMPS, K82_PASS_MS, K82_COOL_MS = 8, 915, 550


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


def levels_1982_frames(frame_ms, speed=DEFAULT_SPEED, step_ms=2):
    """kitt82_begin() / kitt82_led_level(): eight lamps that cool after the light passes.

    Stateful like the firmware (each lamp remembers when the light last left
    it), so this runs the timeline in small steps and yields one bar per frame.
    Simulates two cycles and returns the second, so the trail is warmed up.
    """
    pass_ms = K82_PASS_MS * SPEED_PCT[speed] // 100
    period = 2 * pass_ms
    lit_at = [-10**9] * K82_LAMPS
    frames = []
    for now in range(0, 2 * period, step_ms):
        t = now % period
        x = t if t < pass_ms else 2 * pass_ms - t
        head = ease(x * 1024 // pass_ms) * (K82_LAMPS * 256 - 1) // 1024
        lit_at[head // 256] = now
        if now >= period and (now - period) % frame_ms == 0:
            lamps = [0 if now - l >= K82_COOL_MS else 255 - (now - l) * 255 // K82_COOL_MS for l in lit_at]
            bar = []
            for led in range(LEDS):
                level = lamps[led * K82_LAMPS // LEDS]
                pos = led % (LEDS // K82_LAMPS)
                bar.append(level // 2 if pos in (0, LEDS // K82_LAMPS - 1) else level)
            frames.append(bar)
    return frames, period


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


def render(rows, path, width, bar_h, row_gap, frame_ms, label=False, scanner=levels):
    pad = 24
    label_w = 90 if label else 0
    height = pad * 2 + len(rows) * bar_h + (len(rows) - 1) * row_gap
    if scanner is levels_1982_frames:
        bars, period = levels_1982_frames(frame_ms)
        timeline = list(zip(range(0, period, frame_ms), bars))
    else:
        _, period = scanner(0)
        timeline = [(t, scanner(t)[0]) for t in range(0, period, frame_ms)]
    frames = []
    for t, lv in timeline:
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
    render([("Red", HUES["Red"])], os.path.join(out_dir, "kitt-1982.gif"), 900, 26, 0, 30, scanner=levels_1982_frames)


if __name__ == "__main__":
    main(*sys.argv[1:2])
