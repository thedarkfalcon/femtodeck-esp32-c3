# Release Notes

Version `1.0` is the original atomic14 baseline. Build numbers are tracked from the local expanded build history onward.

To create a new build with an incremented build number, run:

```powershell
.\build.ps1
```

If compiling manually with `arduino-cli compile`, bump `femtodeck-c3/Version.h` first and add a note here.

## v1.1 b28

- Fixed Reading text wrapping so full lines no longer cause unsigned space calculations to swallow following words.
- Changed the Reading footer to `Tap more` until the final page, then `Tap END`.
- Removed `BSP` suffixes from reading titles and added an `About BSP` attribution/public-domain reading.

## v1.1 b27

- Updated Reading pages to use three body lines plus a footer showing page progress or `Tap END`.
- Moved Reading body text away from the bottom edge to avoid clipping on the physical OLED.

## v1.1 b26

- Replaced the Reading utility placeholders with the selected BSP reading set.
- Synced the browser simulator Reading utility with the firmware readings.

## v1.1 b25

- Added a `Reading` utility app with selectable reading slots and automatic text wrapping/paging.
- Added neutral placeholder reading text so final long-form text can be pasted locally later.

## v1.1 b24

- Lowered Blackjack's `2 deck shoe` splash text so it no longer overlaps the card/chip art.
- Added a `Femto OS` credits page for `thedarkfalcon`.
- Changed the boot splash to `FemtoDeck` with `C3` as the model line and the build string on-screen.
- Shortened the root menu title to `FemtoDeck` and renamed the sketch folder/file to `femtodeck-c3`.
- Renamed the GitHub Actions workflow to `build-femtodeck-c3.yml` and removed the old sponsor funding file.

## v1.1 b23

- Renamed the shared launchable base from `Game` to `App` and renamed utility/system files and classes to `*App`.
- Replaced the flat firmware menu with a root menu containing `Games`, `Utilities`, `Options`, and `Credits`.
- Added a short `C3 FemtoDeck` boot splash screen.
- Updated the browser simulator to mirror the boot splash and folder-style menu.

## v1.1 b22

- Moved `Mouse Emulator` vertical wiggle tuning values into top-level constants and increased the bounded vertical wander limit to 20 pixels.

## v1.1 b21

- Added bounded vertical jitter to `Mouse Emulator` movement so the cursor wiggles more like the original S3 mouse script while keeping the C3 update loop non-blocking.

## v1.1 b20

- Renamed `Mouse Jiggler` to `Mouse Emulator` in the device menu, splash screen, credits, and browser simulator.

## v1.1 b19

- Reworked `Mouse Jiggler` to use a small local NimBLE HID mouse implementation instead of the older `ESP32 BLE Mouse` wrapper, reducing BLE task overhead on the ESP32-C3.
- Reset Mouse Jiggler movement state whenever the game launches so stale movement phases cannot carry between runs.

## v1.1 b18

- Added `Counter` utility: a simple counting app (tap increments, hold resets). Currently the only change in this build; more updates planned for future builds.

## v1.1 b17

- Added and refined two timer utilities: `Stopwatch` and `Countdown` (selection UI, top/bottom hints, hold/tap behaviors).
- Fixed menu overlay drawing over utility UIs by giving timer apps a custom overlay; start screens now match other utility apps (`Tap to start` prompts).
- Adjusted hint placement and sizing to avoid clipping on the 72x40 OLED.
- Updated the browser simulator (`sim/`) to mirror the above UI and build-text to `v1.1 b17`.
- Updated repository agent instructions to include a compact project overview and contributor guidance.

## v1.1 b16

- Fixed Tower Stacker's near-perfect placement forgiveness so it snaps cleanly onto the supported block instead of drawing unsupported overhang.

## v1.1 b15

- Made Pipe Mania much more forgiving by increasing build time before goo starts, slowing early goo flow, and reducing hold-to-place delay.
- Added persistent Mini Lander best-level tracking and a best-level intro page.
- Added splash/top-score/prompt pages and persistent scores to Micro Racer, Heli Cave, Breakout, Defender Mini, and Jump Run.
- Added Micro Racer levels that increase speed every 50 cars passed.
- Added Heli Cave survival-time scoring and a stronger gradual speed ramp.
- Added Breakout level progression and cumulative brick-broken scoring.
- Added Defender Mini health so enemies passing in any lane damage the player.
- Reworked Tower Stacker into 10-block tower stages with wider resets, faster later stages, and gentler width loss.
- Updated Credits to show atomic14 as creator and thedarkfalcon as modifier for the original games, plus `git.new/esp32games`.

## v1.1 b14

- Fixed intro page rotation so every game starts on its splash screen when selected, then advances to best score, then `Press to Start`.
- Removed the old uptime-based intro page helper to prevent future screens from starting mid-cycle.

## v1.1 b13

- Changed game intro rotation to three standalone pages: splash art/title, top score/best info, then `Press to Start`.
- Removed start prompts from splash and high-score pages so they no longer clutter the art or score layout.
- Added persistent Need Speed best-run tracking with best level cleared, run time, and player initials.
- Updated the browser simulator to match the new intro flow and Need Speed best-run behavior.

## v1.1 b12

- Fixed Options initials save and Credits long-hold behavior so both return directly to the main menu.
- Added clearer `Tap start`/`Tap retry` prompts to splash, score, and utility screens.
- Made Need Speed level 1 more reachable, added explicit `Lost` result text, and shows the target speed/time when you miss.
- Added Fishing Flick's hook-the-fish timing phase and softened tension recovery so fight spikes are less punishing.
- Moved Blackjack hand totals left to avoid the OLED border and added clearer start prompts.
- Widened Tiny Golf's tight early wall gap.
- Added a desktop level editor at `sim/editor.html` for Tiny Golf and maze wall layouts.
- Synced the browser simulator with the gameplay/prompt changes.

## v1.1 b11

- Added `build.sh`, a Bash build helper for Linux and macOS that increments `Version.h` before running `arduino-cli compile`.
- Bumped the firmware build for device upload.

## v1.1 b10

- Added `Version.h` with `BuildInfo::VERSION_TEXT` and `BuildInfo::BUILD_TEXT`.
- Displayed the current build number on the Credits start and end screens.
- Added `build.ps1` to increment the build number before running `arduino-cli compile`.
- Added this release notes file.

## v1.1 b9

- Mini Lander now shows the safe landing velocity on the crash/end screen as a speed-limit style sign.
- Mini Lander safe landing velocity now varies by level.
- Mini Lander level briefing now ignores input for 1 second, then waits for a fresh tap before starting.
- Need Speed now has 5 levels, increasing target speed and rev difficulty between levels.
- Need Speed now shows a level-clear interstitial and total run time.
- Fishing Flick now has level progression with larger, harder fish.
- Fishing Flick tension now spikes during fish fights, can break at high tension, and can lose the fish at zero slack.

## v1.1 b8

- Added an `Options` menu entry for two-character player initials.
- Added shared `PlayerProfile` storage for initials.
- High-score games now save initials alongside new best scores.
- Intro screens for score-saving games now alternate between splash art and top-score display.
- Added `Maze Collector`, a key-collection variant of Maze Runner with a locked exit.
- Improved Maze Runner defensiveness around no-option choice states.
- Reworked Pipe Mania to make the selected square clearer, slow early levels, and invert filled liquid cells.
- Reworked Blackjack with bet selection, animated dealing, a persistent two-deck shoe, reshuffle behavior, and delayed dealer actions.
- Updated Credits with per-game pages for new games and helpers.

## v1.1 b7

- Added browser emulator under `sim/`.
- Added a crisp bitmap font to the emulator so the scaled OLED text remains readable.
- Emulator supports the one-button menu, GPIO8 LED indicator, local score storage, and playable versions of the new games.

## v1.1 b6

- Added Noon Shooter.
- Added Fishing Flick with GPIO8 tension warning.
- Added Maze Runner with generated mazes and one-button intersection choices.
- Added Pipe Mania adapted for one-button pipe placement.
- Added Blackjack.

## v1.1 b5

- Added Need Speed, a one-button drag-racing dashboard game.
- Added traffic-light launch countdown.
- Added round tachometer, shift quality messages, and GPIO8 shift-light behavior.

## v1.1 b4

- Added Credits as a menu entry.
- Added per-game credit pages for the original atomic14 games and new local games.
- Added GPIO8 support patterns for games that use the onboard LED.

## v1.1 b3

- Added Mini Lander.
- Added moon splash screen, level briefings, fuel/altitude/velocity HUD, landing pad, thrust physics, crash/landing outcomes, and GPIO8 burn/low-fuel cues.

## v1.1 b2

- Added Tiny Golf.
- Added rotating aim, power selection, walls, cups, bounce guides, multi-hole course flow, and best-score persistence.
- Tuned early golf courses, bumper placement, cup forgiveness, aim speed near the cup, and shot behavior.

## v1.1 b1

- Added Tower Stacker.
- Added score display and persistent high score.
- Added end-screen score display.
- Added shared end-screen input lockout to reduce accidental restarts across games.

## v1.0 b0

- Original atomic14 project baseline.
- Included Breakout, Micro Racer, Defender Mini, Jump Run, and Heli Cave.
- Included the original one-button menu flow and ESP32-C3 OLED display setup.
