# How it works

Everything is in [`src/main.cpp`](../src/main.cpp) (M5Unified only).

## Measuring tilt

The MPU6886 accelerometer reads gravity as a vector `(ax, ay, az)` in g. At rest that vector
points straight up, so the tilt of the device is the angle between its axes and that vector. The
reading is smoothed a little (`alpha 0.25` at ~60 fps) so the bubble does not jitter.

| Mode | Device position | Formula |
|---|---|---|
| Bubble | flat, screen up | `roll = atan2(ax, az)`, `pitch = atan2(ay, az)` |
| Tube | standing, screen toward you | `angle = atan2(ax, ay)` (wrapped to +-90) |
| Angle | lying on a slope | `slope = atan2(sqrt(ax^2 + ay^2), |az|)` |

Slope percent is `tan(slope) * 100`.

## Colours

Green within `LEVEL_DEG` (0.5), amber within `NEAR_DEG` (2.0), blue otherwise. Change them at the top
of `main.cpp`.

## Zero (calibration)

Each mode keeps its own zero offset in degrees, saved with `Preferences`. Hold the button 0.6 s and
release to store the current reading as zero; hold 2.2 s to clear it. Press the device on a surface
you know is level, zero it, flip it 180 degrees and check - a good surface reads the same both ways.

## Bubble direction

The bubble runs to the **high** side. If it runs the other way on your unit, flip
`BUBBLE_X_SIGN` / `BUBBLE_Y_SIGN` (see `platformio.ini`).

## Serial console (115200)

| Command | Effect |
|---|---|
| `ver` | print the version |
| `mode 0-2` | bubble / tube / angle |
| `zero` / `reset` | same as the button holds |
| `sim ax ay az` | feed a fake accelerometer reading (g) - for screenshots and testing |
| `sim off` | back to the real sensor |
| `shot` | send the last frame as raw RGB565 (used by `tools/shot.py`) |

## Button handling

`BtnA` is the screen. A click switches mode; a release after 600 ms zeroes; after 2200 ms resets.
The button is polled every loop iteration (not only once per frame) so no click is missed.
