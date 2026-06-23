# FemtoDeck

FemtoDeck is a multi-app and game collection for tiny ESP32 development boards. It supports two main hardware targets:

- **FemtoDeck C3**: Targeted at the ESP32-C3 0.42 inch OLED module (`72x40` resolution).
- **FemtoDeck T-Display**: Targeted at the TENSTAR/LilyGO T-Display ESP32 (`240x135` color resolution).
- **Femto C3 Headless**: A generic no-display ESP32-C3 build. The current role is a miner slave for Mining Manager clusters.

Repo shortcut: [git.new/esp32games](https://git.new/esp32games)

## Supported Hardware

### ESP32-C3 0.42 inch OLED
- OLED on I2C pins `GPIO5` / `GPIO6`
- BOOT button on `GPIO9`
- Optional controllable LED on `GPIO8`

### ESP32-C3 Headless
- BOOT button on `GPIO9`
- Status LED on `GPIO8`
- No OLED or menu required

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
- **Femto Miner** is a small solo mining utility with a dedicated setup portal for wallet and pool settings. It reuses saved FemtoDeck WiFi profiles and defaults to `public-pool.io`.
- **Distributed Miner** is a small ESP-NOW mining cluster mode. A master connects to the pool once, optionally mines locally, pairs slave boards, assigns nonce ranges, and submits valid shares for the cluster.

The communicator packet has a small magic header (`FC`) and a protocol version so FemtoDeck devices can reject unrelated ESP-NOW traffic. It also carries the sender initials, recipient initials, and firmware build number. If a received message comes from a different FemtoDeck build, the app still shows the message but marks it with a version warning.

Contact data is stored separately from game scores. The **Options / Save Manager** screen includes a **Contacts** entry for clearing the saved ESP-NOW contact list, and **Delete All** also clears it.

### Femto Miner Setup

Femto Miner is configured separately from WiFi. First add one or more WiFi networks in **WiFi Setup**, then open **Femto Miner** and launch its setup portal.

Default miner settings:

- Pool: `public-pool.io`
- Port: `21496`
- Password: `x`
- Worker: `Femto<MACADDRESS>`, for example `Femto001A2B3C4D5E`
- Stratum username: `<wallet>.<worker>`

The setup portal SSID is `FemtoMiner Setup` with password `femtominer`. It saves wallet, pool host, pool port, and pool password into the `miner` preferences namespace. The **Options / Save Manager** screen includes a **Miner** entry for clearing these settings.

### Distributed Miner

Distributed Miner is separate from Femto Miner. **Femto Miner** is the normal solo-mining app. **Distributed Miner** coordinates a local ESP-NOW cluster where one board is the master and one or more nearby boards act as slaves.

The master:

- uses the saved FemtoDeck WiFi profiles to connect to your network
- uses the saved Femto Miner wallet and pool settings
- connects to the mining pool once
- broadcasts a pairing beacon while pairing is enabled
- assigns nonce ranges to paired slaves over ESP-NOW
- submits valid shares found by local mining or by slaves

The slaves:

- do not need WiFi credentials or pool settings
- auto-hop WiFi channels until they hear a master pairing beacon
- remember the paired master and cluster ID
- receive compact work assignments containing a block header, start nonce, and nonce count
- return hashrate, best difficulty, and any found share back to the master

Supported roles:

- **T-Display**: open **Distributed Miner**, tap Button 1 on the start screen to choose **Master** or **Slave**, then press Button 2 or hold Button 1 to start.
- **ESP32-C3 0.42 inch OLED**: open **Distributed Miner**, tap to choose **Master** or **Slave**, then hold to start. The C3 UI is compact and mainly useful for testing or lightweight slave use.
- **Femto C3 Headless**: flash the `femto-c3-headless` sketch. It boots directly into slave mode and is the simplest build for no-screen ESP32-C3 boards.

Typical setup:

1. Configure WiFi on the master through **WiFi Setup**.
2. Configure wallet and pool settings through **Femto Miner** setup if you do not want the defaults.
3. Flash slave boards with `femto-c3-headless`, or launch **Distributed Miner** in **Slave** mode on a screened board.
4. Launch **Distributed Miner** on the master in **Master** mode.
5. Start the cluster, then go to the pairing page and start **Pair Slaves**.
6. Wait for slaves to appear on the slave list.
7. Turn **Local Mining** on or off depending on whether the master should also hash.

T-Display master controls:

- **Dashboard page**: Button 1 starts or stops the cluster. Button 2 changes page.
- **Pair Slaves page**: Button 1 starts pairing.
- **Cluster Control page**: Button 1 toggles local mining on/off. Button 1 hold starts or stops the cluster.
- **Reset Cluster page**: Button 1 resets the cluster ID. Slaves must pair again after this.
- Button 2 hold exits back to the FemtoDeck menu.

T-Display slave controls:

- Button 2 changes status/detail pages.
- On **Clear Pairing**, Button 1 hold forgets the saved master.
- On **Exit Slave**, Button 1 hold exits to the FemtoDeck menu.
- Button 2 hold also exits back to the FemtoDeck menu.

Headless C3 slave controls and LED:

- Hold BOOT for about five seconds to clear the saved master/cluster pairing.
- Solid LED: actively or recently mining.
- Slow blink: searching or not paired.
- Very slow blink: paired but idle.
- Fast blink: error.

The tested ESP32-C3 SuperMini GPIO8 LED is active-low, so the LED behavior may need adjusting for different boards.

Serial debug on the T-Display master runs at `115200` baud:

```text
cluster start
cluster stop
cluster stats
cluster pair
cluster reset
cluster local on
cluster local off
```

`cluster stats` prints total, local, and slave hashrate, slave count, jobs, submitted/accepted/rejected shares, channel, and the last error if present.

Headless C3 serial also runs at `115200` baud and prints a status line every two seconds:

```text
[headless] build=<build> state=<state> paired=<yes/no> channel=<n> khps=<rate> jobs=<jobs> completed=<count> total_kh=<hashes>
```

Saved cluster data can be cleared from **Options / Save Manager / Cluster** on FemtoDeck builds. On headless C3 boards, use the BOOT long-hold reset.

Performance notes:

- A T-Display or classic ESP32 slave uses the ESP32 hardware SHA path in Distributed Miner slave mode.
- ESP32-C3 slaves use the C3-compatible path and are much slower than the T-Display/classic ESP32 boards.
- Local mining can be disabled on the master if you want it to spend more time coordinating slaves.

## Build

Use the build script for your platform. It installs/updates the ESP32 Arduino core, installs the required libraries, copies the root `shared/` source into each sketch's generated `src/shared/` folder, and compiles all three firmware targets.

Windows:

```powershell
.\build.ps1
```

Linux/macOS:

```sh
./build.sh
```

The build scripts do not bump `Version.h`; release/version bumps should be made explicitly.

The root `shared/` folder is the single tracked source of truth for shared logic. The board-local `src/shared/` folders are generated during local and GitHub Actions builds and are ignored by Git.

The T-Display target includes a repo-local `tft_setup.h` for TFT_eSPI, so you should not need to edit the installed TFT_eSPI library setup files.

## Browser Installer & Simulator

When GitHub Pages is deployed, you can flash your device directly from the browser or try the C3 simulator:

- **Web Installer for ESP32-C3 and T-Display**: `https://thedarkfalcon.github.io/femtodeck-esp32-c3/`
  - Select `ESP32-C3` for the 0.42 inch OLED C3 build.
  - Select `ESP32` for the FemtoDeck T-Display build.
- **Simulator**: `https://thedarkfalcon.github.io/femtodeck-esp32-c3/simulator/`

The browser installer supports the C3, T-Display, and headless C3 builds and is the recommended way for most users to flash their device.

## License

FemtoDeck is released under WTFPL + No Warranty Disclaimer. See `LICENSE` for the full terms.

Femto Miner includes MIT-attributed mining/Stratum logic adapted from NerdMiner_v2. See `licenses/NerdMiner_v2-MIT.txt`.
