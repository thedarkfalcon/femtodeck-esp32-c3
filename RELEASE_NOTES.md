# Release Notes

## v1.1 b46

- Renamed the utility to `WiFi Settings` and moved testing/deleting saved networks onto the device UI instead of the web portal.
- Expanded WiFi storage to save up to five profiles for users who move between home, work, school, or other networks.
- Added on-device actions to test one saved network, test all saved networks until one connects, or delete one/all saved profiles.
- GPIO8 now stays solid when a WiFi test connects and blinks after a failed test.

## v1.1 b45

- Added a `WiFi Setup` utility that starts a captive portal access point for configuring saved WiFi credentials.
- The setup portal runs at `192.168.4.1`, shows nearby networks, saves credentials to NVS, and can forget saved WiFi.
- Added `WiFi Setup` to Utilities, Credits, simulator placeholders, and README.
- Switched builds to the ESP32-C3 Huge APP partition scheme to fit the WiFi stack and leave room for future network apps.

## v1.1 b44

- Rewrote the README around FemtoDeck C3, removed the inherited game image, and documented current build/upload flow.
- Renamed the on-device `Credits` menu item to `About` with separate License and Credits sections.
- Added concise on-device WTFPL + No Warranty license details.
- Fixed credits pages for Reactor, Reading, and Random Number so they show their developer instead of the repo URL.
- Kept long-press from About detail pages as a return-to-menu shortcut.

## v1.1 b43

- Corrected Femto Field attempts to use one shared five-miss bank across the whole round, rather than resetting attempts for every event.
- After the shared bank is exhausted, the current failed event scores zero and each remaining event in that round gets one pressure try.
- Reset the attempt bank only when a clean round advances to the next harder round.

## v1.1 b42

- Changed Simon hold input to use a shorter local hold threshold; releasing before that counts as tap.
- Cleaned up per-save delete confirmation layout in Save Manager so the selected save and erase/cancel controls do not overlap.

## v1.1 b41

- Changed Femto Field progression from debug mode to five attempts per event until a zero-score event, then one attempt for the rest of that round.
- Made Femto Field end after a round with any zero-score event, otherwise advance to the next harder round while keeping cumulative score.
- Added a hammer throw field animation, including backward throws when the direction is wrong.
- Changed Javelin angle selection to hold-and-release timing instead of tapping an automatically sweeping angle.
- Sorted the Games menu alphabetically while leaving Utilities in their existing order.

## v1.1 b40

- Consumed the boot splash skip tap so it no longer advances the main menu from Games to Utilities.
- Shortened Coin Flipper instructions so they fit on screen.
- Simplified Simon playback and input screens to text-only prompts instead of flashing boxes.
- Made Save Manager confirmations explicitly say `DELETE` before erasing a selected save.

## v1.1 b39

- Changed Femto Field debug flow so qualifying an event advances immediately instead of forcing all five attempts.
- Hid the Hurdles `TAP JUMP` hint after the first jump in an attempt.
- Added fallen hurdle rendering after a failed jump and clamped hurdle drawing to avoid off-screen line flashes.

## v1.1 b38

- Moved Coin Flipper result text away from the coin, with the coin on the right and larger result text on the left.
- Reworked Metronome with instruction pages, a cleaner BPM-only live screen, and a much longer hold-to-exit timeout.
- Moved Pet Simulator into Games, shortened its controls text, and added an idle pet-only wander screen after inactivity.
- Rebuilt Knife Throw around a spinning board with a person, moving reticle, delayed knife travel, and person-hit failure.
- Reworked Reactor display to show current kW and total kWh separately, added stronger rod cooling, cold stall failure, and a radioactive splash symbol.
- Made Simon start with an easy tap cue and show clearer `TAP` / `HOLD` prompts and failure feedback.
- Added an Options Save Manager with per-save deletion and two-step delete-all confirmation.
- Reordered Utilities so Mouse Emulator is second-to-last and Reading is last.

## v1.1 b37

- Made Alien Raiders shield layers visibly disappear as shielded enemies take damage.
- Reduced diagonal spread fire, added stronger late-game enemies, tougher bosses, and boss return fire that the player must dodge.
- Made bomb pickups look like cartoon bombs with a visible fuse and clarified the Alien Raiders intro with `Defend Station` text.
- Reworked Femto Field hurdles into a Jump Run style tap-to-jump event and removed the separate Jump Run source/menu entry.
- Enabled Femto Field debug flow with five attempts per event before advancing, so all events can be tested without qualifying first.

## v1.1 b36

- Added utility apps for dice rolling, coin flipping, random number generation, metronome timing, and a persistent pet simulator.
- Added simple games `Knife Throw`, `Reactor`, and `Simon`, each with start/high-score/prompt screens where applicable.
- Wired the new apps into the Games and Utilities menus and added credits entries.

## v1.1 b35

- Made the FemtoDeck C3 boot splash skippable by pressing the button.
- Retuned City Racer to keep faster road speed while adding enough traffic rows to feel active, with cars-passed scoring and clearer level progression.
- Extended Alien Raiders late-game scaling with stronger shielded enemies, stacking player shields, limited smart-bomb damage, higher weapon tiers, and slow boss waves every 100 kills.
- Fixed Femto Field hurdles so hurdles move toward the runner, with clearer hold/release prompts.

## v1.1 b34

- Retuned City Racer toward faster racing with far fewer traffic rows and much larger gaps between cars.
- Changed Alien Raiders weapon pickups into permanent additive weapon levels so collecting an upgrade cannot downgrade a good weapon.
- Moved Cave Chopper's splash cave line away from the title text.
- Made Femto Field hurdles show the hurdle approaching the runner and changed the airborne prompt away from the confusing `JUMP` label.

## v1.1 b33

- Rebalanced City Racer with slower, sparser early traffic and guaranteed one-direction-safe lane progression.
- Clarified Alien Raiders by moving the base launch animation into a one-time game intro, flipping the player ship forward, reshaping enemy ships, reducing base fire rate, and making powerups rarer/more distinct.
- Made Cave Chopper more forgiving with a smaller chopper collision profile, larger cave gaps, slower initial speed, and gentler vertical acceleration.
- Relaxed Femto Field's first-round qualifying targets and timing cadence so the 100m dash and later events can be tested without requiring near-perfect timing.

## v1.1 b32

- Added `Femto Field`, a one-button multi-event track-and-field game with 100m dash, hurdles, long jump, hammer throw, javelin, and high jump.
- Added cumulative scoring, lives, harder looping rounds, a hard-to-earn extra life threshold, total high score, and per-event point records.
- Added timing cue penalties where early presses count double, plus GPIO8 cue lighting during ideal `PUSH` windows.

## v1.1 b31

- Added an Alien Raiders establishing splash where the player ship launches from the defended station before the view pans toward incoming raiders.
- Changed Breakout '76 paddle control to match the old single-button feel more closely: tap reverses direction and the paddle clamps at screen edges instead of auto-reversing.

## v1.1 b30

- Rebuilt the original five-game set from new source files and removed the old game implementations.
- Replaced Breakout with `Breakout '76`, adding varied launches, paddle-influenced bounces, level patterns, and powerups.
- Replaced Micro Racer with `City Racer`, using forward-facing traffic and fairer level progression.
- Replaced Defender Mini with `Alien Raiders`, adding shielded enemies, progressive difficulty, and temporary upgrades.
- Replaced Heli Cave with `Cave Chopper`, adding a chopper sprite with animated rotors and readable scoring.
- Recreated `Jump Run` from scratch so the original game source set is fully removed.

## v1.1 b29

- Reorganized firmware source into Arduino-compatible `src/games` and `src/apps` folders.
- Kept shared runtime files in the sketch root and updated include paths for the new layout.

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
