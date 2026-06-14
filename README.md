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

## Apps, Games, And Radio Features

FemtoDeck includes arcade-style games, small utilities, settings tools, and radio utilities. Most apps share their core logic between the C3 and T-Display builds, with board-specific rendering for the tiny OLED or larger color TFT.

Radio-capable utilities:

- **WiFi Setup** stores multiple WiFi networks from a captive portal, then lets the device test or delete saved networks from the settings app.
- **ESP Contacts** manages a shared ESP-NOW contact list. Use **Listen for Contacts** to save nearby FemtoDeck devices, **Send My Contact** to broadcast your saved initials, and **Manage Contacts** to remove saved peers.
- **Communicator** sends predefined ESP-NOW messages. Messages use compact dictionary path IDs rather than full strings, then the receiver maps the path back to local text. After choosing a message, select **ALL** or a saved contact. Incoming messages are shown only when addressed to **ALL** or to this device's saved initials.

The communicator packet has a small magic header (`FC`) and a protocol version so FemtoDeck devices can reject unrelated ESP-NOW traffic. It also carries the sender initials, recipient initials, and firmware build number. If a received message comes from a different FemtoDeck build, the app still shows the message but marks it with a version warning.

Contact data is stored separately from game scores. The **Options / Save Manager** screen includes a **Contacts** entry for clearing the saved ESP-NOW contact list, and **Delete All** also clears it.

## Build

Install the ESP32 core and required libraries:

```sh
arduino-cli core install esp32:esp32
arduino-cli lib install U8g2 "NimBLE-Arduino" "TFT_eSPI"
```

### Compile for ESP32-C3:
```sh
arduino-cli compile --fqbn esp32:esp32:esp32c3:PartitionScheme=huge_app femtodeck-c3
```

### Compile for T-Display:
```sh
arduino-cli compile --fqbn esp32:esp32:esp32:PartitionScheme=huge_app femtodeck-t-display
```

The T-Display target includes a repo-local `tft_setup.h` for TFT_eSPI, so you should not need to edit the installed TFT_eSPI library setup files.

## Browser Installer & Simulator

When GitHub Pages is deployed, you can flash your device directly from the browser or try the C3 simulator:

- **Web Installer**: `https://thedarkfalcon.github.io/femtodeck-esp32-c3/`
- **Simulator**: `https://thedarkfalcon.github.io/femtodeck-esp32-c3/simulator/`

The browser installer supports both board types and is the recommended way for most users to flash their device.

## License

FemtoDeck is released under WTFPL + No Warranty Disclaimer. See `LICENSE` for the full terms.
