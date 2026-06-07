#include <U8g2lib.h>
#include <Wire.h>
#include <string.h>

#include "App.h"
#include "BlackjackGame.h"
#include "BreakoutGame.h"
#include "CountdownApp.h"
#include "CounterApp.h"
#include "CreditsApp.h"
#include "DefenderMiniGame.h"
#include "FishingFlickGame.h"
#include "HeliCaveGame.h"
#include "JumpGame.h"
#include "MazeRunnerGame.h"
#include "MicroRacerGame.h"
#include "MiniLanderGame.h"
#include "MouseEmulatorApp.h"
#include "NeedSpeedGame.h"
#include "NoonShooterGame.h"
#include "OptionsApp.h"
#include "PipeManiaGame.h"
#include "PlayerProfile.h"
#include "ReadingApp.h"
#include "StopwatchApp.h"
#include "TinyGolfGame.h"
#include "TowerStackerGame.h"
#include "Version.h"

class U8G2_SSD1306_72X40_NONAME_F_HW_I2C : public U8G2 {
  public:
    U8G2_SSD1306_72X40_NONAME_F_HW_I2C(
        const u8g2_cb_t* rotation, uint8_t reset = U8X8_PIN_NONE, uint8_t clock = U8X8_PIN_NONE, uint8_t data = U8X8_PIN_NONE)
        : U8G2() {
      u8g2_Setup_ssd1306_i2c_72x40_er_f(&u8g2, rotation, u8x8_byte_arduino_hw_i2c, u8x8_gpio_and_delay_arduino);
      u8x8_SetPin_HW_I2C(getU8x8(), reset, clock, data);
    }
};

U8G2_SSD1306_72X40_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE, 6, 5);

constexpr uint8_t BUTTON_PIN = 9;
constexpr uint8_t SCREEN_WIDTH = 72;
constexpr uint8_t SCREEN_HEIGHT = 40;
constexpr uint8_t APP_WIDTH = 70;
constexpr uint8_t APP_HEIGHT = 40;
constexpr uint8_t APP_LEFT = 1;
constexpr uint16_t BOOT_SPLASH_MS = 2000;

BreakoutGame breakoutGame(APP_WIDTH, APP_HEIGHT, APP_LEFT);
MicroRacerGame microRacerGame(APP_WIDTH, APP_HEIGHT, APP_LEFT);
DefenderMiniGame defenderMiniGame(APP_WIDTH, APP_HEIGHT, APP_LEFT);
JumpGame jumpGame(APP_WIDTH, APP_HEIGHT, APP_LEFT);
HeliCaveGame heliCaveGame(APP_WIDTH, APP_HEIGHT, APP_LEFT);
MiniLanderGame miniLanderGame(APP_WIDTH, APP_HEIGHT, APP_LEFT);
NeedSpeedGame needSpeedGame(APP_WIDTH, APP_HEIGHT, APP_LEFT);
NoonShooterGame noonShooterGame(APP_WIDTH, APP_HEIGHT, APP_LEFT);
FishingFlickGame fishingFlickGame(APP_WIDTH, APP_HEIGHT, APP_LEFT);
MazeRunnerGame mazeRunnerGame(APP_WIDTH, APP_HEIGHT, APP_LEFT);
MazeRunnerGame mazeCollectorGame(APP_WIDTH, APP_HEIGHT, APP_LEFT, true);
PipeManiaGame pipeManiaGame(APP_WIDTH, APP_HEIGHT, APP_LEFT);
BlackjackGame blackjackGame(APP_WIDTH, APP_HEIGHT, APP_LEFT);
TinyGolfGame tinyGolfGame(APP_WIDTH, APP_HEIGHT, APP_LEFT);
TowerStackerGame towerStackerGame(APP_WIDTH, APP_HEIGHT, APP_LEFT);

StopwatchApp stopwatchApp(APP_WIDTH, APP_HEIGHT, APP_LEFT);
CountdownApp countdownApp(APP_WIDTH, APP_HEIGHT, APP_LEFT);
CounterApp counterApp(APP_WIDTH, APP_HEIGHT, APP_LEFT);
MouseEmulatorApp mouseEmulatorApp(APP_WIDTH, APP_HEIGHT, APP_LEFT);
ReadingApp readingApp(APP_WIDTH, APP_HEIGHT, APP_LEFT);

OptionsApp optionsApp(APP_WIDTH, APP_HEIGHT);
CreditsApp creditsApp(APP_WIDTH, APP_HEIGHT);

enum class MenuView {
  Root,
  Games,
  Utilities
};

enum class MenuAction {
  OpenGames,
  OpenUtilities,
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
    {"Utilities", MenuAction::OpenUtilities, nullptr},
    {nullptr, MenuAction::Launch, &optionsApp},
    {nullptr, MenuAction::Launch, &creditsApp},
};

MenuEntry gamesMenu[] = {
    {nullptr, MenuAction::Launch, &breakoutGame},
    {nullptr, MenuAction::Launch, &microRacerGame},
    {nullptr, MenuAction::Launch, &defenderMiniGame},
    {nullptr, MenuAction::Launch, &jumpGame},
    {nullptr, MenuAction::Launch, &heliCaveGame},
    {nullptr, MenuAction::Launch, &miniLanderGame},
    {nullptr, MenuAction::Launch, &needSpeedGame},
    {nullptr, MenuAction::Launch, &noonShooterGame},
    {nullptr, MenuAction::Launch, &fishingFlickGame},
    {nullptr, MenuAction::Launch, &mazeRunnerGame},
    {nullptr, MenuAction::Launch, &mazeCollectorGame},
    {nullptr, MenuAction::Launch, &pipeManiaGame},
    {nullptr, MenuAction::Launch, &blackjackGame},
    {nullptr, MenuAction::Launch, &tinyGolfGame},
    {nullptr, MenuAction::Launch, &towerStackerGame},
    {"Back", MenuAction::Back, nullptr},
};

MenuEntry utilitiesMenu[] = {
    {nullptr, MenuAction::Launch, &stopwatchApp},
    {nullptr, MenuAction::Launch, &countdownApp},
    {nullptr, MenuAction::Launch, &counterApp},
    {nullptr, MenuAction::Launch, &mouseEmulatorApp},
    {nullptr, MenuAction::Launch, &readingApp},
    {"Back", MenuAction::Back, nullptr},
};

constexpr uint8_t ROOT_MENU_COUNT = sizeof(rootMenu) / sizeof(rootMenu[0]);
constexpr uint8_t GAMES_MENU_COUNT = sizeof(gamesMenu) / sizeof(gamesMenu[0]);
constexpr uint8_t UTILITIES_MENU_COUNT = sizeof(utilitiesMenu) / sizeof(utilitiesMenu[0]);

SingleButton menuButton;
MenuView currentMenu = MenuView::Root;
MenuView activeReturnMenu = MenuView::Root;
uint8_t menuIndex = 0;
bool menuSelectArmed = false;
bool bootSplashActive = true;
uint32_t bootStartedAtMs = 0;
App* activeApp = nullptr;

bool isButtonDown() {
  return digitalRead(BUTTON_PIN) == LOW;
}

const MenuEntry* currentMenuEntries() {
  switch (currentMenu) {
    case MenuView::Games:
      return gamesMenu;
    case MenuView::Utilities:
      return utilitiesMenu;
    case MenuView::Root:
    default:
      return rootMenu;
  }
}

uint8_t currentMenuCount() {
  switch (currentMenu) {
    case MenuView::Games:
      return GAMES_MENU_COUNT;
    case MenuView::Utilities:
      return UTILITIES_MENU_COUNT;
    case MenuView::Root:
    default:
      return ROOT_MENU_COUNT;
  }
}

const char* currentMenuTitle() {
  switch (currentMenu) {
    case MenuView::Games:
      return "Games";
    case MenuView::Utilities:
      return "Utilities";
    case MenuView::Root:
    default:
      return "FemtoDeck";
  }
}

const char* entryTitle(const MenuEntry& entry) {
  if (entry.app != nullptr) {
    return entry.app->appTitle();
  }
  return entry.label;
}

void drawTextFit(int x, int y, const char* text) {
  u8g2.setFont(u8g2_font_5x8_tr);
  if (u8g2.getStrWidth(text) > (SCREEN_WIDTH - x - 3)) {
    u8g2.setFont(u8g2_font_4x6_tr);
  }
  u8g2.drawStr(x, y, text);
}

void drawBootSplash() {
  u8g2.drawFrame(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
  u8g2.setFont(u8g2_font_6x10_tr);

  const char* top = "FemtoDeck";
  const char* bottom = "C3";
  const int topX = (SCREEN_WIDTH - u8g2.getStrWidth(top)) / 2;
  const int bottomX = (SCREEN_WIDTH - u8g2.getStrWidth(bottom)) / 2;

  u8g2.drawStr(topX, 11, top);
  u8g2.drawStr(bottomX, 24, bottom);

  u8g2.setFont(u8g2_font_4x6_tr);
  const int buildX = (SCREEN_WIDTH - u8g2.getStrWidth(BuildInfo::BUILD_TEXT)) / 2;
  u8g2.drawStr(buildX, 38, BuildInfo::BUILD_TEXT);
}

void drawMenu() {
  const MenuEntry* entries = currentMenuEntries();
  const uint8_t count = currentMenuCount();
  const MenuEntry& entry = entries[menuIndex];

  u8g2.drawFrame(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
  u8g2.setFont(u8g2_font_4x6_tr);
  u8g2.drawStr(3, 8, currentMenuTitle());

  char position[6];
  snprintf(position, sizeof(position), "%u/%u", menuIndex + 1, count);
  u8g2.drawStr(SCREEN_WIDTH - u8g2.getStrWidth(position) - 3, 8, position);

  drawTextFit(3, 21, entryTitle(entry));

  u8g2.setFont(u8g2_font_5x8_tr);
  if (menuSelectArmed) {
    u8g2.drawStr(3, 31, "Release");
    u8g2.drawStr(3, 39, entry.action == MenuAction::Back ? "to back" : "to open");
  } else {
    u8g2.drawStr(3, 31, "Tap next");
    u8g2.drawStr(3, 39, "Hold open");
  }
}

void openMenu(MenuView view) {
  currentMenu = view;
  menuIndex = 0;
  menuSelectArmed = false;
}

void launchApp(App& app, uint32_t nowMs, bool buttonDown) {
  activeReturnMenu = currentMenu;
  activeApp = &app;
  activeApp->begin(nowMs, buttonDown);
  menuSelectArmed = false;
}

void selectMenuEntry(uint32_t nowMs, bool buttonDown) {
  const MenuEntry& entry = currentMenuEntries()[menuIndex];
  switch (entry.action) {
    case MenuAction::OpenGames:
      openMenu(MenuView::Games);
      break;
    case MenuAction::OpenUtilities:
      openMenu(MenuView::Utilities);
      break;
    case MenuAction::Back:
      openMenu(MenuView::Root);
      break;
    case MenuAction::Launch:
      if (entry.app != nullptr) {
        launchApp(*entry.app, nowMs, buttonDown);
      }
      break;
  }
}

void updateMenu(uint32_t nowMs, bool buttonDown) {
  const ButtonInput input = menuButton.update(buttonDown, nowMs);
  if (input.longPress) {
    menuSelectArmed = true;
  }
  if (input.click && !menuSelectArmed) {
    menuIndex = (menuIndex + 1) % currentMenuCount();
  }
  if (menuSelectArmed && input.released) {
    selectMenuEntry(nowMs, buttonDown);
    menuButton.reset(buttonDown, nowMs);
  }
}

void drawAppOverlay(App& app) {
  u8g2.drawFrame(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
  u8g2.setFont(u8g2_font_5x8_tr);
  if (app.phase() == AppPhase::Start) {
    if (app.showStartPromptPage()) {
      u8g2.drawStr(20, 16, "Press");
      u8g2.drawStr(13, 29, "to Start");
    } else if (app.showStartScorePage()) {
      u8g2.drawStr(3, 10, "Top Score");
      u8g2.setFont(u8g2_font_4x6_tr);
      u8g2.drawStr(3, 24, "--");
    } else {
      drawTextFit(3, 10, app.appTitle());
    }
  } else if (app.phase() == AppPhase::End) {
    u8g2.drawStr(3, 10, "Game Over");
    u8g2.drawStr(3, 24, "Tap retry");
    u8g2.drawStr(3, 36, "Hold menu");
  }
}

void setup(void) {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  u8g2.begin();
  u8g2.setContrast(255);
  u8g2.setBusClock(400000);
  randomSeed(micros());

  const uint32_t nowMs = millis();
  bootStartedAtMs = nowMs;
  menuButton.reset(isButtonDown(), nowMs);
}

void loop(void) {
  const uint32_t nowMs = millis();
  const bool buttonDown = isButtonDown();

  u8g2.clearBuffer();

  if (bootSplashActive) {
    drawBootSplash();
    if ((nowMs - bootStartedAtMs) >= BOOT_SPLASH_MS) {
      bootSplashActive = false;
      menuButton.reset(buttonDown, nowMs);
    }
  } else if (activeApp == nullptr) {
    updateMenu(nowMs, buttonDown);
    drawMenu();
  } else {
    activeApp->tick(nowMs, buttonDown);
    activeApp->render(u8g2);
    if (activeApp->phase() != AppPhase::Running && !activeApp->hasCustomOverlay()) {
      drawAppOverlay(*activeApp);
    }
    if (activeApp->shouldExitToMenu()) {
      activeApp->clearExitRequest();
      activeApp = nullptr;
      currentMenu = activeReturnMenu;
      menuSelectArmed = false;
      menuButton.reset(buttonDown, nowMs);
    }
  }

  u8g2.sendBuffer();
}
