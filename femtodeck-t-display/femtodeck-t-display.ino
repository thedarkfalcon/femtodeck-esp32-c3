#include <TFT_eSPI.h>
#include <SPI.h>
#include <Wire.h>

#include "App.h"
#include "src/apps/CounterApp.h"
#include "src/apps/MouseEmulatorApp.h"
#include "src/apps/DiceRollerApp.h"
#include "src/apps/CoinFlipperApp.h"
#include "src/apps/RandomNumberApp.h"
#include "src/apps/StopwatchApp.h"
#include "src/apps/CountdownApp.h"
#include "src/apps/ClockApp.h"
#include "src/apps/MetronomeApp.h"
#include "src/apps/WiFiSetupApp.h"
#include "src/apps/EspContactsApp.h"
#include "src/apps/CommunicatorApp.h"
#include "src/apps/CreditsApp.h"
#include "src/apps/OptionsApp.h"
#include "src/apps/PetSimulatorApp.h"
#include "src/apps/ReadingApp.h"

#include "src/games/BlackjackGame.h"
#include "src/games/AlienRaidersGame.h"
#include "src/games/Breakout76Game.h"
#include "src/games/CaveChopperGame.h"
#include "src/games/CityRacerGame.h"
#include "src/games/FemtoFieldGame.h"
#include "src/games/FishingFlickGame.h"
#include "src/games/KnifeThrowGame.h"
#include "src/games/MazeRunnerGame.h"
#include "src/games/MiniLanderGame.h"
#include "src/games/NeedSpeedGame.h"
#include "src/games/NoonShooterGame.h"
#include "src/games/NuclearReactorGame.h"
#include "src/games/PipeManiaGame.h"
#include "src/games/SimonGame.h"
#include "src/games/TinyGolfGame.h"
#include "src/games/TowerStackerGame.h"

#include "TDisplayUi.h"
#include "Version.h"

TFT_eSPI tft = TFT_eSPI();

constexpr uint8_t BUTTON_1 = 0;
constexpr uint8_t BUTTON_2 = 35;

constexpr uint16_t SCREEN_WIDTH = 240;
constexpr uint16_t SCREEN_HEIGHT = 135;
constexpr uint8_t VISIBLE_MENU_ROWS = 6;
constexpr uint16_t MENU_ROW_Y = 42;
constexpr uint16_t MENU_ROW_H = 13;
constexpr uint16_t MENU_LEFT = 12;
constexpr uint16_t MENU_RIGHT = SCREEN_WIDTH - 12;
constexpr uint16_t BOOT_SPLASH_MS = 2000;
constexpr uint16_t APP_RUNNING_RENDER_MS = 33;
constexpr uint16_t APP_STATIC_RENDER_MS = 2000;

CounterApp counterApp(SCREEN_WIDTH, SCREEN_HEIGHT);
MouseEmulatorApp mouseEmulatorApp(SCREEN_WIDTH, SCREEN_HEIGHT);
DiceRollerApp diceRollerApp(SCREEN_WIDTH, SCREEN_HEIGHT);
CoinFlipperApp coinFlipperApp(SCREEN_WIDTH, SCREEN_HEIGHT);
RandomNumberApp randomNumberApp(SCREEN_WIDTH, SCREEN_HEIGHT);
StopwatchApp stopwatchApp(SCREEN_WIDTH, SCREEN_HEIGHT);
ClockApp clockApp(SCREEN_WIDTH, SCREEN_HEIGHT);
CountdownApp countdownApp(SCREEN_WIDTH, SCREEN_HEIGHT);
MetronomeApp metronomeApp(SCREEN_WIDTH, SCREEN_HEIGHT);
WiFiSetupApp wifiSetupApp(SCREEN_WIDTH, SCREEN_HEIGHT);
EspContactsApp espContactsApp(SCREEN_WIDTH, SCREEN_HEIGHT);
CommunicatorApp communicatorApp(SCREEN_WIDTH, SCREEN_HEIGHT);
CreditsApp creditsApp(SCREEN_WIDTH, SCREEN_HEIGHT);
OptionsApp optionsApp(SCREEN_WIDTH, SCREEN_HEIGHT);
PetSimulatorApp petSimulatorApp(SCREEN_WIDTH, SCREEN_HEIGHT);
ReadingApp readingApp(SCREEN_WIDTH, SCREEN_HEIGHT);

AlienRaidersGame alienRaidersGame(SCREEN_WIDTH, SCREEN_HEIGHT);
BlackjackGame blackjackGame(SCREEN_WIDTH, SCREEN_HEIGHT);
Breakout76Game breakout76Game(SCREEN_WIDTH, SCREEN_HEIGHT);
CaveChopperGame caveChopperGame(SCREEN_WIDTH, SCREEN_HEIGHT);
CityRacerGame cityRacerGame(SCREEN_WIDTH, SCREEN_HEIGHT);
FemtoFieldGame femtoFieldGame(SCREEN_WIDTH, SCREEN_HEIGHT);
FishingFlickGame fishingFlickGame(SCREEN_WIDTH, SCREEN_HEIGHT);
KnifeThrowGame knifeThrowGame(SCREEN_WIDTH, SCREEN_HEIGHT);
MazeRunnerGame mazeRunnerGame(SCREEN_WIDTH, SCREEN_HEIGHT, 0);
MazeRunnerGame mazeCollectorGame(SCREEN_WIDTH, SCREEN_HEIGHT, 0, true);
MiniLanderGame miniLanderGame(SCREEN_WIDTH, SCREEN_HEIGHT);
NeedSpeedGame needSpeedGame(SCREEN_WIDTH, SCREEN_HEIGHT);
NoonShooterGame noonShooterGame(SCREEN_WIDTH, SCREEN_HEIGHT);
NuclearReactorGame nuclearReactorGame(SCREEN_WIDTH, SCREEN_HEIGHT);
PipeManiaGame pipeManiaGame(SCREEN_WIDTH, SCREEN_HEIGHT);
SimonGame simonGame(SCREEN_WIDTH, SCREEN_HEIGHT);
TinyGolfGame tinyGolfGame(SCREEN_WIDTH, SCREEN_HEIGHT);
TowerStackerGame towerStackerGame(SCREEN_WIDTH, SCREEN_HEIGHT);

enum class MenuView {
  Root,
  Games,
  Apps,
  Settings
};

enum class MenuAction {
  OpenGames,
  OpenApps,
  OpenSettings,
  Launch,
  Back
};

struct MenuEntry {
  const char* label;
  MenuAction action;
  App* app;
};

MenuEntry rootMenu[] = {
    {"Games", MenuAction::OpenGames, nullptr},
    {"Apps", MenuAction::OpenApps, nullptr},
    {"Settings", MenuAction::OpenSettings, nullptr},
    {nullptr, MenuAction::Launch, &creditsApp},
};

MenuEntry gamesMenu[] = {
    {nullptr, MenuAction::Launch, &alienRaidersGame},
    {nullptr, MenuAction::Launch, &blackjackGame},
    {nullptr, MenuAction::Launch, &breakout76Game},
    {nullptr, MenuAction::Launch, &caveChopperGame},
    {nullptr, MenuAction::Launch, &cityRacerGame},
    {nullptr, MenuAction::Launch, &femtoFieldGame},
    {nullptr, MenuAction::Launch, &fishingFlickGame},
    {nullptr, MenuAction::Launch, &knifeThrowGame},
    {nullptr, MenuAction::Launch, &mazeCollectorGame},
    {nullptr, MenuAction::Launch, &mazeRunnerGame},
    {nullptr, MenuAction::Launch, &miniLanderGame},
    {nullptr, MenuAction::Launch, &needSpeedGame},
    {nullptr, MenuAction::Launch, &noonShooterGame},
    {nullptr, MenuAction::Launch, &petSimulatorApp},
    {nullptr, MenuAction::Launch, &pipeManiaGame},
    {nullptr, MenuAction::Launch, &nuclearReactorGame},
    {nullptr, MenuAction::Launch, &simonGame},
    {nullptr, MenuAction::Launch, &tinyGolfGame},
    {nullptr, MenuAction::Launch, &towerStackerGame},
    {"Back", MenuAction::Back, nullptr},
};

MenuEntry appsMenu[] = {
    {nullptr, MenuAction::Launch, &clockApp},
    {nullptr, MenuAction::Launch, &stopwatchApp},
    {nullptr, MenuAction::Launch, &countdownApp},
    {nullptr, MenuAction::Launch, &counterApp},
    {nullptr, MenuAction::Launch, &diceRollerApp},
    {nullptr, MenuAction::Launch, &coinFlipperApp},
    {nullptr, MenuAction::Launch, &randomNumberApp},
    {nullptr, MenuAction::Launch, &metronomeApp},
    {nullptr, MenuAction::Launch, &espContactsApp},
    {nullptr, MenuAction::Launch, &communicatorApp},
    {nullptr, MenuAction::Launch, &mouseEmulatorApp},
    {nullptr, MenuAction::Launch, &readingApp},
    {"Back", MenuAction::Back, nullptr},
};

MenuEntry settingsMenu[] = {
    {nullptr, MenuAction::Launch, &optionsApp},
    {nullptr, MenuAction::Launch, &wifiSetupApp},
    {"Back", MenuAction::Back, nullptr},
};

constexpr uint8_t ROOT_MENU_COUNT = sizeof(rootMenu) / sizeof(rootMenu[0]);
constexpr uint8_t GAMES_MENU_COUNT = sizeof(gamesMenu) / sizeof(gamesMenu[0]);
constexpr uint8_t APPS_MENU_COUNT = sizeof(appsMenu) / sizeof(appsMenu[0]);
constexpr uint8_t SETTINGS_MENU_COUNT = sizeof(settingsMenu) / sizeof(settingsMenu[0]);

SingleButton menuButton1;
SingleButton menuButton2;
MenuView currentMenu = MenuView::Root;
MenuView activeReturnMenu = MenuView::Root;
uint8_t menuIndex = 0;
uint8_t scrollOffset = 0;
bool menuSelectArmed = false;
bool menuDirty = true;
bool menuInputLockedUntilRelease = false;
bool bootSplashActive = true;
bool bootSkipReleasePending = false;
uint32_t bootStartedAtMs = 0;
App* activeApp = nullptr;
bool appRenderDue = false;
uint32_t nextAppRenderMs = 0;
AppPhase lastRenderedAppPhase = AppPhase::Start;

bool isButton1Down() { return digitalRead(BUTTON_1) == LOW; }
bool isButton2Down() { return digitalRead(BUTTON_2) == LOW; }

MenuEntry* currentMenuEntries() {
  switch (currentMenu) {
    case MenuView::Games:
      return gamesMenu;
    case MenuView::Apps:
      return appsMenu;
    case MenuView::Settings:
      return settingsMenu;
    case MenuView::Root:
    default:
      return rootMenu;
  }
}

uint8_t currentMenuCount() {
  switch (currentMenu) {
    case MenuView::Games:
      return GAMES_MENU_COUNT;
    case MenuView::Apps:
      return APPS_MENU_COUNT;
    case MenuView::Settings:
      return SETTINGS_MENU_COUNT;
    case MenuView::Root:
    default:
      return ROOT_MENU_COUNT;
  }
}

const char* currentMenuTitle() {
  switch (currentMenu) {
    case MenuView::Games:
      return "Games";
    case MenuView::Apps:
      return "Apps";
    case MenuView::Settings:
      return "Settings";
    case MenuView::Root:
    default:
      return "FemtoDeck";
  }
}

const char* entryTitle(const MenuEntry& entry) {
  return entry.app != nullptr ? entry.app->appTitle() : entry.label;
}

void markMenuDirty() {
  menuDirty = true;
}

void openMenu(MenuView view) {
  currentMenu = view;
  menuIndex = 0;
  scrollOffset = 0;
  menuSelectArmed = false;
  markMenuDirty();
}

void launchApp(App& app, uint32_t nowMs) {
  activeReturnMenu = currentMenu;
  activeApp = &app;
  activeApp->begin(nowMs, false, false);
  menuSelectArmed = false;
  appRenderDue = true;
  nextAppRenderMs = 0;
  lastRenderedAppPhase = activeApp->phase();
  tft.fillScreen(TFT_BLACK);
}

void selectMenuEntry(uint32_t nowMs) {
  const MenuEntry& entry = currentMenuEntries()[menuIndex];
  switch (entry.action) {
    case MenuAction::OpenGames:
      openMenu(MenuView::Games);
      break;
    case MenuAction::OpenApps:
      openMenu(MenuView::Apps);
      break;
    case MenuAction::OpenSettings:
      openMenu(MenuView::Settings);
      break;
    case MenuAction::Back:
      openMenu(MenuView::Root);
      break;
    case MenuAction::Launch:
      if (entry.app != nullptr) {
        launchApp(*entry.app, nowMs);
      }
      break;
  }
}

void moveMenu(int8_t direction) {
  const uint8_t count = currentMenuCount();
  if (direction > 0) {
    menuIndex = (menuIndex + 1) % count;
  } else if (menuIndex == 0) {
    menuIndex = count - 1;
  } else {
    menuIndex--;
  }
  menuSelectArmed = false;
  markMenuDirty();
}

void updateMenu(uint32_t nowMs, bool b1, bool b2) {
  if (menuInputLockedUntilRelease) {
    menuButton1.reset(b1, nowMs);
    menuButton2.reset(b2, nowMs);
    if (!b1 && !b2) {
      menuInputLockedUntilRelease = false;
      menuButton1.reset(false, nowMs);
      menuButton2.reset(false, nowMs);
    }
    return;
  }

  const ButtonInput input1 = menuButton1.update(b1, nowMs);
  const ButtonInput input2 = menuButton2.update(b2, nowMs);

  if (input1.longPress) {
    menuSelectArmed = true;
    markMenuDirty();
  }
  if (input1.click && !menuSelectArmed) {
    moveMenu(1);
  }
  if (menuSelectArmed && input1.released) {
    selectMenuEntry(nowMs);
    menuButton1.reset(false, nowMs);
  }

  if (input2.click) {
    moveMenu(-1);
  }
  if (input2.longPress && currentMenu != MenuView::Root) {
    openMenu(MenuView::Root);
    menuButton2.reset(b2, nowMs);
  }
}

void exitActiveAppToMenu(uint32_t nowMs, bool b1, bool b2) {
  activeApp->clearExitRequest();
  activeApp = nullptr;
  currentMenu = activeReturnMenu;
  menuSelectArmed = false;
  menuInputLockedUntilRelease = true;
  menuButton1.reset(b1, nowMs);
  menuButton2.reset(b2, nowMs);
  markMenuDirty();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
}

void renderActiveAppIfDue(uint32_t nowMs) {
  const AppPhase currentPhase = activeApp->phase();
  const uint16_t interval = currentPhase == AppPhase::Running ? activeApp->runningRenderIntervalMs() : APP_STATIC_RENDER_MS;
  if (!appRenderDue && currentPhase == lastRenderedAppPhase && nowMs < nextAppRenderMs) {
    return;
  }
  activeApp->render(tft);
  appRenderDue = false;
  lastRenderedAppPhase = currentPhase;
  nextAppRenderMs = nowMs + interval;
}

void drawCenteredText(const char* text, int16_t y, uint8_t size, uint16_t color) {
  tft.setTextDatum(TL_DATUM);
  tft.setTextSize(size);
  tft.setTextColor(color, TFT_BLACK);
  tft.drawString(text, (SCREEN_WIDTH - tft.textWidth(text)) / 2, y);
}

void drawBootSplash() {
  tft.fillScreen(TFT_BLACK);
  tft.drawRoundRect(5, 5, SCREEN_WIDTH - 10, SCREEN_HEIGHT - 10, 8, TFT_DARKGREY);
  tft.drawFastHLine(24, 31, SCREEN_WIDTH - 48, TFT_CYAN);
  tft.drawFastHLine(42, 101, SCREEN_WIDTH - 84, TFT_ORANGE);

  drawCenteredText("FemtoDeck", 42, 3, TFT_WHITE);
  drawCenteredText("T-Display", 72, 2, TFT_CYAN);
  drawCenteredText(BuildInfo::BUILD_TEXT, 114, 1, TFT_LIGHTGREY);
}

template <typename Canvas>
void drawMenuFrame(Canvas& canvas, uint8_t count, TDisplayUi::TextSize textSize) {
  const char* footer = menuSelectArmed
                           ? (currentMenuEntries()[menuIndex].action == MenuAction::Back ? "Release: back" : "Release: open")
                           : "B1 next/open  B2 prev/back";
  TDisplayUi::menuFrame(canvas, currentMenuTitle(), menuIndex, count, scrollOffset,
                        [](uint8_t index) -> const char* {
                          return entryTitle(currentMenuEntries()[index]);
                        },
                        footer, textSize, TFT_GREEN);
}

void drawMenu() {
  if (!menuDirty) {
    return;
  }
  menuDirty = false;

  const uint8_t count = currentMenuCount();
  const TDisplayUi::TextSize textSize = TDisplayUi::loadTextSize();
  scrollOffset = TDisplayUi::menuScrollOffset(menuIndex, count, TDisplayUi::menuStyle(textSize));

  tft.setRotation(1);
  static TFT_eSprite menuFrame(&tft);
  static bool menuFrameTried = false;
  static bool menuFrameReady = false;
  if (!menuFrameTried) {
    menuFrameTried = true;
    menuFrame.setColorDepth(8);
    menuFrameReady = menuFrame.createSprite(SCREEN_WIDTH, SCREEN_HEIGHT) != nullptr;
  }

  if (menuFrameReady) {
    drawMenuFrame(menuFrame, count, textSize);
    menuFrame.pushSprite(0, 0);
  } else {
    drawMenuFrame(tft, count, textSize);
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_1, INPUT_PULLUP);
  pinMode(BUTTON_2, INPUT_PULLUP);

  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  const uint32_t nowMs = millis();
  bootStartedAtMs = nowMs;
  menuButton1.reset(isButton1Down(), nowMs);
  menuButton2.reset(isButton2Down(), nowMs);
  drawBootSplash();
  markMenuDirty();
}

void loop() {
  uint32_t nowMs = millis();
  bool b1 = isButton1Down();
  bool b2 = isButton2Down();

  if (bootSplashActive && (b1 || b2 || (nowMs - bootStartedAtMs) >= BOOT_SPLASH_MS)) {
    bootSplashActive = false;
    bootSkipReleasePending = b1 || b2;
    menuButton1.reset(b1, nowMs);
    menuButton2.reset(b2, nowMs);
    markMenuDirty();
    tft.fillScreen(TFT_BLACK);
  }

  if (bootSplashActive) {
    delay(10);
    return;
  }

  if (bootSkipReleasePending) {
    menuButton1.reset(b1, nowMs);
    menuButton2.reset(b2, nowMs);
    if (!b1 && !b2) {
      bootSkipReleasePending = false;
      menuButton1.reset(false, nowMs);
      menuButton2.reset(false, nowMs);
    }
    drawMenu();
    delay(10);
    return;
  }

  if (activeApp == nullptr) {
    updateMenu(nowMs, b1, b2);
    drawMenu();
  } else {
    activeApp->tick(nowMs, b1, b2);
    if (activeApp->shouldExitToMenu()) {
        exitActiveAppToMenu(nowMs, b1, b2);
    } else {
        renderActiveAppIfDue(nowMs);
    }
  }
  delay(10);
}
