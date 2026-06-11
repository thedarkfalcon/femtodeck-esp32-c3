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
#include "src/apps/MetronomeApp.h"
#include "src/apps/WiFiSetupApp.h"
#include "src/apps/CommunicatorApp.h"

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

#include "Version.h"

TFT_eSPI tft = TFT_eSPI();

constexpr uint8_t BUTTON_1 = 0;
constexpr uint8_t BUTTON_2 = 35;

constexpr uint16_t SCREEN_WIDTH = 240;
constexpr uint16_t SCREEN_HEIGHT = 135;

CounterApp counterApp(SCREEN_WIDTH, SCREEN_HEIGHT);
MouseEmulatorApp mouseEmulatorApp(SCREEN_WIDTH, SCREEN_HEIGHT);
DiceRollerApp diceRollerApp(SCREEN_WIDTH, SCREEN_HEIGHT);
CoinFlipperApp coinFlipperApp(SCREEN_WIDTH, SCREEN_HEIGHT);
RandomNumberApp randomNumberApp(SCREEN_WIDTH, SCREEN_HEIGHT);
StopwatchApp stopwatchApp(SCREEN_WIDTH, SCREEN_HEIGHT);
CountdownApp countdownApp(SCREEN_WIDTH, SCREEN_HEIGHT);
MetronomeApp metronomeApp(SCREEN_WIDTH, SCREEN_HEIGHT);
WiFiSetupApp wifiSetupApp(SCREEN_WIDTH, SCREEN_HEIGHT);
CommunicatorApp communicatorApp(SCREEN_WIDTH, SCREEN_HEIGHT);

AlienRaidersGame alienRaidersGame(SCREEN_WIDTH, SCREEN_HEIGHT);
BlackjackGame blackjackGame(SCREEN_WIDTH, SCREEN_HEIGHT);
Breakout76Game breakout76Game(SCREEN_WIDTH, SCREEN_HEIGHT);
CaveChopperGame caveChopperGame(SCREEN_WIDTH, SCREEN_HEIGHT);
CityRacerGame cityRacerGame(SCREEN_WIDTH, SCREEN_HEIGHT);
FemtoFieldGame femtoFieldGame(SCREEN_WIDTH, SCREEN_HEIGHT);
FishingFlickGame fishingFlickGame(SCREEN_WIDTH, SCREEN_HEIGHT);
KnifeThrowGame knifeThrowGame(SCREEN_WIDTH, SCREEN_HEIGHT);
MazeRunnerGame mazeRunnerGame(SCREEN_WIDTH, SCREEN_HEIGHT);
MiniLanderGame miniLanderGame(SCREEN_WIDTH, SCREEN_HEIGHT);
NeedSpeedGame needSpeedGame(SCREEN_WIDTH, SCREEN_HEIGHT);
NoonShooterGame noonShooterGame(SCREEN_WIDTH, SCREEN_HEIGHT);
NuclearReactorGame nuclearReactorGame(SCREEN_WIDTH, SCREEN_HEIGHT);
PipeManiaGame pipeManiaGame(SCREEN_WIDTH, SCREEN_HEIGHT);
SimonGame simonGame(SCREEN_WIDTH, SCREEN_HEIGHT);
TinyGolfGame tinyGolfGame(SCREEN_WIDTH, SCREEN_HEIGHT);
TowerStackerGame towerStackerGame(SCREEN_WIDTH, SCREEN_HEIGHT);

App* apps[] = {
    &alienRaidersGame,
    &blackjackGame,
    &breakout76Game,
    &caveChopperGame,
    &cityRacerGame,
    &femtoFieldGame,
    &fishingFlickGame,
    &knifeThrowGame,
    &mazeRunnerGame,
    &miniLanderGame,
    &needSpeedGame,
    &noonShooterGame,
    &nuclearReactorGame,
    &pipeManiaGame,
    &simonGame,
    &tinyGolfGame,
    &towerStackerGame,
    &counterApp,
    &mouseEmulatorApp,
    &diceRollerApp,
    &coinFlipperApp,
    &randomNumberApp,
    &stopwatchApp,
    &countdownApp,
    &metronomeApp,
    &wifiSetupApp,
    &communicatorApp
};
constexpr uint8_t APP_COUNT = sizeof(apps) / sizeof(apps[0]);
uint8_t appIndex = 0;
uint8_t scrollOffset = 0;
App* activeApp = nullptr;

bool isButton1Down() { return digitalRead(BUTTON_1) == LOW; }
bool isButton2Down() { return digitalRead(BUTTON_2) == LOW; }

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_1, INPUT_PULLUP);
  pinMode(BUTTON_2, INPUT_PULLUP);

  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
}

void loop() {
  uint32_t nowMs = millis();
  bool b1 = isButton1Down();
  bool b2 = isButton2Down();

  if (activeApp == nullptr) {
    tft.setCursor(10, 5);
    tft.setTextColor(TFT_ORANGE, TFT_BLACK);
    tft.setTextSize(2);
    tft.println("FemtoDeck T-Display");
    tft.setTextSize(1);
    tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    tft.setCursor(10, 25);
    tft.println("B1: Next, Hold B1: Launch, B2: Prev");

    if (appIndex < scrollOffset) scrollOffset = appIndex;
    if (appIndex >= scrollOffset + 8) scrollOffset = appIndex - 7;

    for (int i=0; i<8 && (scrollOffset + i) < APP_COUNT; i++) {
        int idx = scrollOffset + i;
        tft.setCursor(10, 40 + i*11);
        if (idx == appIndex) {
            tft.setTextColor(TFT_BLACK, TFT_GREEN);
            tft.print("> ");
        } else {
            tft.setTextColor(TFT_WHITE, TFT_BLACK);
            tft.print("  ");
        }
        tft.println(apps[idx]->appTitle());
    }

    static uint32_t lastB1Press = 0;
    static bool b1WasDown = false;
    if (b1 && !b1WasDown) {
        lastB1Press = nowMs;
        b1WasDown = true;
    }
    if (b1WasDown && !b1) {
        if (nowMs - lastB1Press < 500) {
            appIndex = (appIndex + 1) % APP_COUNT;
        } else {
            activeApp = apps[appIndex];
            activeApp->begin(nowMs, b1, b2);
            tft.fillScreen(TFT_BLACK);
        }
        b1WasDown = false;
    }

    static uint32_t lastB2Press = 0;
    static bool b2WasDown = false;
    if (b2 && !b2WasDown) {
        lastB2Press = nowMs;
        b2WasDown = true;
    }
    if (b2WasDown && !b2) {
        if (nowMs - lastB2Press < 500) {
            if (appIndex == 0) appIndex = APP_COUNT - 1;
            else appIndex--;
        }
        b2WasDown = false;
    }
  } else {
    activeApp->tick(nowMs, b1, b2);
    activeApp->render(tft);
    if (activeApp->shouldExitToMenu()) {
        activeApp->clearExitRequest();
        activeApp = nullptr;
        tft.fillScreen(TFT_BLACK);
    }
  }
  delay(10);
}
