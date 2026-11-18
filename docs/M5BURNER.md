# Publishing to M5Burner

How to share this firmware in [M5Burner](https://docs.m5stack.com/en/uiflow/m5burner/publish)
(*USER CUSTOM > Publish*, logged in with an M5Stack community account).

> **Do not use M5Burner's "Export" on your own AtomS3.** Export reads the whole flash from the
> device, including your saved zero points. Upload the clean `-full.bin` from the Releases page
> instead: it is built from source and its settings area (NVS) is blank.

## Fields

| Field | Value |
|---|---|
| Name | `AtomS3 Spirit Level` |
| Version | `v1.0.0` |
| Device Type | `ATOMS3` (AtomS3 with display + MPU6886; not AtomS3 Lite) |
| Github | `https://github.com/ElectroIoT/M5-AtomS3-Spirit-Level` |
| FirmWare | `M5-AtomS3-Spirit-Level-v1.0.0-full.bin` (single image, flashed at `0x0`) |
| Cover | [`docs/images/m5burner_cover.png`](images/m5burner_cover.png) (800 x 600) |

## Firmware description (markdown, paste as is)

```markdown
# AtomS3 Spirit Level

A pocket **spirit level** for your **M5Stack AtomS3**, using the built-in motion sensor.

## Modes (click the screen to switch)
- **Bubble**: lay it flat, a 2-axis bubble shows X and Y tilt in degrees
- **Tube**: stand it up, a horizontal vial shows the angle in the screen plane (shelves, pictures)
- **Angle**: lay it on a ramp or roof, shows the slope in degrees and percent

The ring turns **amber** within 2 degrees and **green** within 0.5 degrees.

## Calibrate
Hold the screen 0.6 s and release to **zero** the current mode on a surface you trust.
Hold 2.2 s and release to reset. Zero points are remembered after power off.

No Wi-Fi, no setup. For the **AtomS3 with display + MPU6886** (not AtomS3 Lite).
A handy pocket tool: zero it on a surface you trust. Not a calibrated instrument.

Source and guide: https://github.com/ElectroIoT/M5-AtomS3-Spirit-Level
```

## Version description (markdown, paste as is)

```markdown
## v1.0.0 - first release

- Bubble level (flat), tube level (standing) and inclinometer (slope degrees + percent)
- Green / amber / blue level indication, live X / Y angles
- Hold the screen to zero a mode, hold longer to reset; zero points are saved
- No Wi-Fi, no setup. For the **AtomS3 with display + MPU6886** (not AtomS3 Lite)

Source and guide: https://github.com/ElectroIoT/M5-AtomS3-Spirit-Level
```

## Making the image (maintainers)

```bash
pio run
python ~/.platformio/packages/tool-esptoolpy/esptool.py --chip esp32s3 merge_bin \
  -o release/M5-AtomS3-Spirit-Level-vX.Y.Z-full.bin --flash_mode dio --flash_freq 80m --flash_size 8MB \
  0x0 .pio/build/atoms3/bootloader.bin \
  0x8000 .pio/build/atoms3/partitions.bin \
  0xe000 ~/.platformio/packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin \
  0x10000 .pio/build/atoms3/firmware.bin
```

This firmware never touches Wi-Fi, so the image holds no personal data; the settings area is blank.
