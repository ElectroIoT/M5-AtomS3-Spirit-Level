#!/usr/bin/env python3
"""Real screenshots from a connected AtomS3 over USB (firmware `shot` + `sim` commands).

  python tools/shot.py COM9 gallery docs/screenshots        README pictures
  python tools/shot.py COM9 tour docs/images/tour.gif       animated tour

The `sim ax ay az` console command feeds the firmware a synthetic accelerometer reading (in g)
so the screens can be photographed without physically tilting the device. The pictures are
real frames from the device's own renderer.

Needs: pip install pyserial pillow
"""
import math
import sys
import time

import serial
from PIL import Image


def open_port(port):
    s = serial.Serial(port, 115200, timeout=2)
    s.dtr = False
    s.rts = False
    time.sleep(0.3)
    s.reset_input_buffer()
    return s


def cmd(s, text, wait=0.05):
    s.write((text + "\n").encode())
    time.sleep(wait)


def grab(s):
    s.reset_input_buffer()
    cmd(s, "shot", 0.0)
    head = b""
    t0 = time.time()
    while True:
        c = s.read(1)
        if c:
            head += c
            if head.endswith(b"\n"):
                if head.strip().startswith(b"SHOT"):
                    break
                head = b""
        if time.time() - t0 > 5:
            raise TimeoutError("no SHOT header (is the firmware flashed?)")
    w, h = (int(x) for x in head.split()[1:3])
    raw = b""
    while len(raw) < w * h * 2:
        chunk = s.read(w * h * 2 - len(raw))
        if not chunk:
            raise TimeoutError("short frame")
        raw += chunk
    img = Image.new("RGB", (w, h))
    px = img.load()
    for i in range(w * h):
        v = (raw[2 * i] << 8) | raw[2 * i + 1]
        r, g, b = (v >> 11) & 31, (v >> 5) & 63, v & 31
        px[i % w, i // w] = ((r * 255) // 31, (g * 255) // 63, (b * 255) // 31)
    return img


def tilt_flat(roll_deg, pitch_deg):
    """Accelerometer reading (g) for a device lying screen-up, tilted by roll about y / pitch about x."""
    return math.sin(math.radians(roll_deg)), math.sin(math.radians(pitch_deg)), 1.0


def tilt_upright(rot_deg):
    """Reading for a device standing up, rotated in the screen plane by rot_deg."""
    return math.sin(math.radians(rot_deg)), math.cos(math.radians(rot_deg)), 0.0


def tilt_slope(slope_deg):
    """Reading for a device lying on a slope of slope_deg."""
    return math.sin(math.radians(slope_deg)), 0.0, math.cos(math.radians(slope_deg))


def sim(s, v):
    cmd(s, "sim %.4f %.4f %.4f" % v, 0.12)


SCENES = [
    ("bubble_level", 0, tilt_flat(0, 0)),
    ("bubble_near", 0, tilt_flat(1.4, -1.0)),
    ("bubble_tilted", 0, tilt_flat(4.5, 3.0)),
    ("tube_level", 1, tilt_upright(0)),
    ("tube_tilted", 1, tilt_upright(4.0)),
    ("angle_flat", 2, tilt_slope(0)),
    ("angle_slope", 2, tilt_slope(23.5)),
]


def gallery(s, outdir):
    for name, mode, v in SCENES:
        cmd(s, f"mode {mode}")
        sim(s, v)
        time.sleep(0.3)
        grab(s).save(f"{outdir}/{name}.png")
        print("saved", name)
    cmd(s, "sim off")


def tour(s, out):
    frames = []

    def snap(n=1):
        for _ in range(n):
            frames.append(grab(s).resize((256, 256), Image.NEAREST))

    cmd(s, "mode 0")
    for i in range(36):  # bubble circles in, then settles level
        r = 6.0 * (1 - i / 36.0)
        a = i * 0.45
        sim(s, tilt_flat(r * math.cos(a), r * math.sin(a)))
        snap()
    sim(s, tilt_flat(0, 0))
    snap(6)
    cmd(s, "mode 1")
    for i in range(30):
        sim(s, tilt_upright(7.0 * math.sin(i / 30.0 * math.pi * 2) * (1 - i / 36.0)))
        snap()
    sim(s, tilt_upright(0))
    snap(6)
    cmd(s, "mode 2")
    for i in range(30):
        sim(s, tilt_slope(40.0 * math.sin(i / 30.0 * math.pi)))
        snap()
    cmd(s, "sim off")
    frames[0].save(out, save_all=True, append_images=frames[1:], duration=90, loop=0, optimize=True)
    print("saved", out, len(frames), "frames")


if __name__ == "__main__":
    if len(sys.argv) < 4:
        sys.exit(__doc__)
    ser = open_port(sys.argv[1])
    {"gallery": gallery, "tour": tour}.get(sys.argv[2], lambda *_: sys.exit(__doc__))(ser, sys.argv[3])
