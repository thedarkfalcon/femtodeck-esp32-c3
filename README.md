# FemtoDeck

FemtoDeck is a multi-app and game collection for tiny ESP32 development boards. It supports two main hardware targets:

- **FemtoDeck C3**: Targeted at the ESP32-C3 0.42 inch OLED module (`72x40` resolution).
- **FemtoDeck T-Display**: Targeted at the TENSTAR/LilyGO T-Display ESP32 (`240x135` color resolution).

Repo shortcut: [git.new/esp32games](https://git.new/esp32games)

## Supported Hardware

### ESP32-C3 0.42 inch OLED
- OLED on I2C pins `GPIO5` / `GPIO6`
- BOOT button on `GPIO9`
- Optional controllable LED on `GPIO8`

### TENSTAR T-Display ESP32
- 1.14" Color TFT (ST7789)
- Buttons on `GPIO0` and `GPIO35`
- Backlight on `GPIO4`

## Controls

### FemtoDeck C3 (Single Button)
- Menu tap: move to the next item
- Menu hold: open the selected item
- App or game tap: primary action
- App or game hold: secondary action or return to menu

### FemtoDeck T-Display (Two Buttons)
- Button 1 tap: move next / primary action
- Button 1 hold: launch / secondary action
- Button 2 tap: move previous / back
- Button 2 hold: global exit to menu

## Build

Install the ESP32 core and required libraries:

```sh
arduino-cli core install esp32:esp32
arduino-cli lib install U8g2 "NimBLE-Arduino" "TFT_eSPI"
```

### Compile for ESP32-C3:
```sh
arduino-cli compile --fqbn esp32:esp32:esp32c3 femtodeck-c3
```

### Compile for T-Display:
```sh
arduino-cli compile --fqbn esp32:esp32:esp32 femtodeck-t-display
```

## Browser Installer & Simulator

When GitHub Pages is deployed, you can flash your device directly from the browser or try the C3 simulator:

- **Web Installer**: `https://thedarkfalcon.github.io/femtodeck-esp32-c3/`
- **Simulator**: `https://thedarkfalcon.github.io/femtodeck-esp32-c3/simulator/`

The browser installer supports both board types and is the recommended way for most users to flash their device.

## License

FemtoDeck is released under WTFPL + No Warranty Disclaimer. See `LICENSE` for the full terms.
