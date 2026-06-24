# Flashing FemtoDeck

The easiest way to install FemtoDeck is the browser flasher:

[https://thedarkfalcon.github.io/femtodeck-esp32-c3/](https://thedarkfalcon.github.io/femtodeck-esp32-c3/)

Use Chrome or Edge on a desktop computer. Mobile browsers usually do not expose USB serial flashing.

## Which Build To Pick

- **FemtoDeck C3**: ESP32-C3 with the 0.42 inch `72x40` OLED.
- **FemtoDeck T-Display**: classic ESP32 T-Display with `240x135` color TFT.
- **Femto C3 Headless**: no-screen ESP32-C3 used as a Distributed Miner slave.

## Browser Flashing

1. Connect the board over USB.
2. Open the [web flasher](https://thedarkfalcon.github.io/femtodeck-esp32-c3/).
3. Select the correct build.
4. Click **Install**.
5. Choose the board serial port.
6. Wait for flashing to complete, then reset or reconnect the board if needed.

If the board is not detected or flashing fails, hold **BOOT** while plugging the board in, then try again.

## Releases Versus Web Flasher

The web flasher and GitHub Releases are related but separate:

- **Web flasher**: uses firmware from the latest successful GitHub Pages deployment from `main`.
- **GitHub Releases**: versioned ZIP files attached to tags such as `v2.0-b77`.

For most users, the web flasher is simpler. Releases are useful when you want a specific version, offline copy, or manual `esptool` flashing.

## Manual Flashing From A Release

Download a firmware ZIP from the [latest release](https://github.com/thedarkfalcon/femtodeck-esp32-c3/releases/latest), extract it, and use the merged binary.

Windows release ZIPs include a helper:

```bat
flash-windows.bat COM9
```

Replace `COM9` with your board's actual serial port.

Manual `esptool` commands:

```powershell
python -m esptool --chip esp32c3 --port COM9 --baud 460800 write_flash -z 0x0 femtodeck-c3-merged.bin
```

```powershell
python -m esptool --chip esp32c3 --port COM9 --baud 460800 write_flash -z 0x0 femto-c3-headless-merged.bin
```

```powershell
python -m esptool --chip esp32 --port COM6 --baud 460800 write_flash -z 0x0 femtodeck-t-display-merged.bin
```

On Linux/macOS, use the same commands with `python3` and a serial device such as `/dev/ttyACM0` or `/dev/ttyUSB0`.

## Finding The Serial Port

On Windows, common options:

- Arduino IDE: **Tools / Port**.
- Device Manager: **Ports (COM & LPT)**.
- PowerShell:

```powershell
[System.IO.Ports.SerialPort]::getportnames()
```

If multiple ports are shown, unplug the board, run the command, plug it back in, and run it again. The new port is usually the board.

## Troubleshooting

- Hold **BOOT** while connecting USB if flashing cannot enter bootloader mode.
- Try a different USB cable. Charge-only cables are very common.
- Close Arduino Serial Monitor or any other program using the COM port.
- For ESP32-C3 boards with a tiny OLED, make sure you selected **FemtoDeck C3**, not the T-Display build.
- For T-Display boards, make sure you selected **FemtoDeck T-Display**, not an ESP32-C3 build.
