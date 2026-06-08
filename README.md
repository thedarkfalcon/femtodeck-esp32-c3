# FemtoDeck C3

FemtoDeck C3 is a tiny one-button app and game collection for the ESP32-C3
0.42 inch OLED module. The screen is only `72x40`, so most apps use a `70x40`
play area and lean hard into simple timing, single-button decisions, and chunky
monochrome pixel art.

This project was inspired by Atomic14's
[esp32-c3-oled-single-button-games](https://github.com/atomic14/esp32-c3-oled-single-button-games).
It originally started as adding new games, but then became a complete rewrite
from the ground up.

Repo shortcut: [git.new/esp32games](https://git.new/esp32games)

## Hardware

- ESP32-C3 0.42 inch OLED module
- OLED on I2C pins `GPIO5` / `GPIO6`
- BOOT button on `GPIO9`
- Optional controllable LED on `GPIO8`

The always-on `POWER` LED is not controlled by the firmware. Games and apps
that use an LED target `GPIO8`.

## Controls

- Menu tap: move to the next item
- Menu hold, then release: open the selected item
- App or game tap: primary action
- App or game hold: secondary action, back, or return to menu depending on context
- End screens usually ignore input briefly so accidental button presses do not skip them

## Games

- Alien Raiders
- Blackjack
- Breakout '76
- Cave Chopper
- City Racer
- Femto Field
- Fishing Flick
- Knife Throw
- Maze Collector
- Maze Runner
- Mini Lander
- Need Speed
- Noon Shooter
- Pet Simulator
- Pipe Mania
- Reactor
- Simon
- Tiny Golf
- Tower Stacker

## Utilities

- Stopwatch
- Countdown
- Counter
- Dice Roller
- Coin Flipper
- Random Number
- Metronome
- Mouse Emulator
- Reading
- Options
- About

## Build

Install the ESP32 core and the U8g2 display library:

```sh
arduino-cli core install esp32:esp32
arduino-cli lib install U8g2
```

Compile directly:

```sh
arduino-cli compile --fqbn esp32:esp32:esp32c3 femtodeck-c3
```

Or use the build helper, which increments the build number before compiling:

```powershell
.\build.ps1
```

```sh
./build.sh
```

## Upload

Find the connected serial port:

```sh
arduino-cli board list
```

On Windows, upload to a COM port such as `COM9`:

```sh
arduino-cli upload --fqbn esp32:esp32:esp32c3 --port COM9 femtodeck-c3
```

On macOS or Linux, use the matching `/dev/...` port:

```sh
arduino-cli upload --fqbn esp32:esp32:esp32c3 --port /dev/ttyACM0 femtodeck-c3
```

## Browser Simulator

The `sim/` folder contains a browser-based simulator for quick iteration on
some apps without flashing the ESP32-C3 every time. It is not a perfect hardware
emulator, but it is useful for checking menu flow, text fit, and basic gameplay
ideas.

When GitHub Pages is deployed, the simulator is available at:

```text
https://thedarkfalcon.github.io/femtodeck-esp32-c3/simulator/
```

## Distribution

GitHub Actions builds release-ready firmware on every validation run.

- The workflow artifact `femtodeck-c3-firmware` contains the merged binary,
  split binaries, and fallback flashing scripts.
- Push a tag such as `v1.1-b44` to create a GitHub Release with the firmware zip.
- Push to the default branch to publish the ESP Web Tools browser installer on
  GitHub Pages, including the browser simulator at `/simulator/`.

The browser installer is the recommended way for most users to flash a device.
They only need Chrome or Edge on desktop and a USB cable.

## License

FemtoDeck C3 is released under WTFPL + No Warranty Disclaimer. See `LICENSE` for
the full terms.
