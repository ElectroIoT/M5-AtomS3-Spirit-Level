# Troubleshooting

| Problem | What to do |
|---|---|
| No serial port appears | Use a USB-C **data** cable. Hold the screen button while plugging in USB (download mode), release after 2 s, retry. |
| Upload says *Wrong boot mode detected* | Put the device in download mode (hold the button, replug) and flash with `--before no_reset`. |
| Bubble runs the wrong way | Flip `BUBBLE_X_SIGN` / `BUBBLE_Y_SIGN` in `platformio.ini`, rebuild. |
| Reads 0.3-0.5 deg when it is on a level surface | Normal sensor offset. Hold the button 0.6 s to zero it there. |
| Tube mode shows "stand it up" | Tube mode is for a device standing on its edge; lay it flat for Bubble mode. |
| Reading drifts after a temperature change | Zero it again. |
| Want the factory zero back | Hold the button 2.2 s in that mode, or send `reset` on the serial console. |
| Screen is blank after flashing | Unplug and replug USB once. |
