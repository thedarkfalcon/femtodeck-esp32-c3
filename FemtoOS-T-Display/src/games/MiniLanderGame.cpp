#include "MiniLanderGame.h"

#include <Arduino.h>
#include <Preferences.h>
#include <TFT_eSPI.h>

#include "../../PlayerProfile.h"
#include "../../TDisplayUi.h"

namespace {
constexpr int HUD_H = 28;
constexpr int GROUND_Y = 124;
constexpr int SHIP_X = 120;
constexpr int LANDER_FOOT_OFFSET = 13;
constexpr uint8_t BURN_HINT_LED_PIN = 8;
constexpr bool BURN_HINT_LED_ACTIVE_LOW = true;
constexpr bool BURN_HINT_LED_AVAILABLE = false;
constexpr float THRUST = 48.0f;
constexpr float FUEL_BURN = 24.0f;
constexpr float BURN_CUE_REACTION_SEC = 0.35f;
constexpr uint16_t BRIEFING_INPUT_LOCK_MS = 1000;
Preferences landerPrefs;
}

MiniLanderGame::MiniLanderGame(uint32_t width, uint32_t height)
    : App("Mini Lander", width, height) {}

bool MiniLanderGame::hasCustomOverlay() const {
  return true;
}

void MiniLanderGame::onAppReset() {
  if (BURN_HINT_LED_AVAILABLE) {
    pinMode(BURN_HINT_LED_PIN, OUTPUT);
  }
  setBurnHintLed(false);
  loadBestLevel();
  level_ = 1;
  crashed_ = false;
  loadLevel();
}

void MiniLanderGame::updateRunning(uint32_t deltaMs, const ButtonInput& b1, const ButtonInput& b2) {
  (void)b2;
  const float deltaSec = static_cast<float>(deltaMs) * 0.001f;

  if (landerState_ == LanderState::Briefing) {
    setBurnHintLed(false);
    briefingTimerMs_ += deltaMs;
    if (briefingTimerMs_ < BRIEFING_INPUT_LOCK_MS) {
      return;
    }
    if (!briefingCanAcceptInput_) {
      if (!b1.down) {
        briefingCanAcceptInput_ = true;
      }
      return;
    }
    if (b1.click) {
      landerState_ = LanderState::Falling;
    }
    return;
  }

  if (landerState_ == LanderState::Landed) {
    setBurnHintLed(false);
    landedTimerMs_ += deltaMs;
    if (landedTimerMs_ >= 900) {
      level_++;
      loadLevel();
    }
    return;
  }

  const bool thrusting = b1.down && fuel_ > 0.0f;
  thrusting_ = thrusting;
  const bool lowFuelBlink = fuel_ <= (startFuel_ * 0.2f) && ((millis() / 180) % 2) == 0;
  setBurnHintLed((shouldShowBurnHint() && !thrusting) || lowFuelBlink);
  float accel = levelGravity();
  if (thrusting) {
    accel -= THRUST;
    fuel_ -= FUEL_BURN * deltaSec;
    if (fuel_ < 0.0f) {
      fuel_ = 0.0f;
    }
  }

  velocity_ += accel * deltaSec;
  altitude_ -= velocity_ * deltaSec;

  if (altitude_ <= 0.0f) {
    altitude_ = 0.0f;
    if (velocity_ <= levelSafeSpeed()) {
      velocity_ = 0.0f;
      landerState_ = LanderState::Landed;
      landedTimerMs_ = 0;
      recordLandedLevel();
      setBurnHintLed(false);
    } else {
      crashed_ = true;
      setBurnHintLed(false);
      endApp();
    }
  }
}

void MiniLanderGame::drawRunning(TFT_eSPI& tft) {
  TDisplayUi::clear(tft);
  if (landerState_ == LanderState::Briefing) {
    drawBriefing(tft);
    return;
  }

  drawHud(tft);
  drawGround(tft);

  const float altitudeRatio = altitude_ / startAltitude_;
  const int altitudePixels = altitude_ <= 0.0f ? 0 : max(1, static_cast<int>(ceilf(altitudeRatio * 78.0f)));
  const int shipY = GROUND_Y - LANDER_FOOT_OFFSET - altitudePixels;
  drawLander(tft, SHIP_X, shipY, thrusting_);

  if (landerState_ == LanderState::Landed) {
    TDisplayUi::centered(tft, "LANDED", 54, 4, TFT_GREEN);
  }
}

void MiniLanderGame::drawStart(TFT_eSPI& tft) { tft.fillScreen(TFT_BLACK);
  TDisplayUi::clear(tft);
  if (showStartPromptPage()) {
    TDisplayUi::header(tft, "Mini Lander", TFT_CYAN);
    TDisplayUi::centered(tft, "Press to Start", 56, 2, TFT_WHITE);
    TDisplayUi::footer(tft, "Hold B1 for thrust");
    return;
  }
  if (showStartScorePage()) {
    char initials[4];
    PlayerProfile::unpackDottedInitials(bestInitials_, initials);
    TDisplayUi::header(tft, "Best Level", TFT_CYAN);
    String score = bestLevel_ == 0 ? "--" : String(initials) + " L" + String(bestLevel_);
    TDisplayUi::largeValue(tft, score, 54, TFT_CYAN);
    TDisplayUi::footer(tft, "Highest landing level");
    return;
  }
  TDisplayUi::header(tft, "Mini Lander", TFT_CYAN);
  tft.fillCircle(56, 72, 28, TFT_DARKGREY);
  tft.drawCircle(56, 72, 28, TFT_LIGHTGREY);
  tft.drawCircle(44, 63, 5, TFT_BLACK);
  tft.drawCircle(65, 81, 7, TFT_BLACK);
  tft.drawCircle(72, 59, 3, TFT_BLACK);
  tft.drawLine(140, 47, 162, 57, TFT_WHITE);
  tft.drawTriangle(166, 58, 181, 51, 176, 70, TFT_WHITE);
  tft.drawPixel(130, 38, TFT_WHITE);
  tft.drawPixel(202, 82, TFT_WHITE);
  tft.drawPixel(166, 102, TFT_WHITE);
  TDisplayUi::footer(tft, "Time the suicide burn");
}

void MiniLanderGame::drawEnd(TFT_eSPI& tft) { tft.fillScreen(TFT_BLACK);
  TDisplayUi::clear(tft);
  TDisplayUi::header(tft, crashed_ ? "Crashed" : "Mission End", crashed_ ? TFT_RED : TFT_GREEN);
  TDisplayUi::labelValue(tft, 45, "Level", String(level_), TFT_YELLOW);
  TDisplayUi::labelValue(tft, 72, "Velocity", String(static_cast<int>(velocity_ + 0.5f)), crashed_ ? TFT_RED : TFT_GREEN);
  drawSafeSpeedSign(tft, 196, 73, static_cast<int>(levelSafeSpeed() + 0.5f));
  TDisplayUi::footer(tft, "B1 retry / B1 hold menu");
}

void MiniLanderGame::loadBestLevel() {
  if (bestLoaded_) {
    return;
  }
  landerPrefs.begin("lander", true);
  bestLevel_ = landerPrefs.getUChar("best", 0);
  bestInitials_ = landerPrefs.getUShort("init", PlayerProfile::defaultInitials());
  landerPrefs.end();
  bestLoaded_ = true;
}

void MiniLanderGame::saveBestLevel() {
  bestInitials_ = PlayerProfile::loadInitials();
  landerPrefs.begin("lander", false);
  landerPrefs.putUChar("best", bestLevel_);
  landerPrefs.putUShort("init", bestInitials_);
  landerPrefs.end();
}

void MiniLanderGame::recordLandedLevel() {
  loadBestLevel();
  if (level_ > bestLevel_) {
    bestLevel_ = level_;
    saveBestLevel();
  }
}

void MiniLanderGame::loadLevel() {
  landerState_ = LanderState::Briefing;
  briefingTimerMs_ = 0;
  briefingCanAcceptInput_ = false;
  landedTimerMs_ = 0;
  startAltitude_ = 90.0f + static_cast<float>(level_ - 1) * 8.0f;
  altitude_ = startAltitude_;
  velocity_ = 3.5f + static_cast<float>(level_ - 1) * 1.6f;
  startFuel_ = max(22.0f, 70.0f - static_cast<float>(level_ - 1) * 6.0f);
  fuel_ = startFuel_;
  thrusting_ = false;
  setBurnHintLed(false);
}

float MiniLanderGame::levelGravity() const {
  if (level_ <= 3) {
    return 6.5f + static_cast<float>(level_ - 1) * 1.4f;
  }
  return 11.5f + static_cast<float>(level_ - 4) * 2.1f;
}

float MiniLanderGame::levelSafeSpeed() const {
  if (level_ == 1) {
    return 14.0f;
  }
  if (level_ == 2) {
    return 13.0f;
  }
  if (level_ == 3) {
    return 12.0f;
  }
  return max(8.0f, 12.0f - static_cast<float>(level_ - 3));
}

bool MiniLanderGame::shouldShowBurnHint() const {
  if (level_ > 3 || landerState_ != LanderState::Falling || fuel_ <= 0.0f) {
    return false;
  }
  const float netDecel = THRUST - levelGravity();
  if (netDecel <= 0.0f || velocity_ <= 1.0f) {
    return false;
  }
  const float gravity = levelGravity();
  const float reactionDistance =
      (velocity_ * BURN_CUE_REACTION_SEC) + (0.5f * gravity * BURN_CUE_REACTION_SEC * BURN_CUE_REACTION_SEC);
  const float velocityAfterReaction = velocity_ + (gravity * BURN_CUE_REACTION_SEC);
  const float stoppingDistance = (velocityAfterReaction * velocityAfterReaction) / (2.0f * netDecel);
  const float cueError = altitude_ - (reactionDistance + stoppingDistance);
  return cueError <= 3.0f && cueError >= -2.0f;
}

void MiniLanderGame::setBurnHintLed(bool on) {
  if (!BURN_HINT_LED_AVAILABLE) {
    return;
  }
  digitalWrite(BURN_HINT_LED_PIN, BURN_HINT_LED_ACTIVE_LOW ? !on : on);
}

void MiniLanderGame::drawSafeSpeedSign(TFT_eSPI& tft, int x, int y, int safeSpeed) {
  tft.fillCircle(x, y, 18, TFT_WHITE);
  tft.drawCircle(x, y, 17, TFT_RED);
  tft.drawCircle(x, y, 16, TFT_RED);
  tft.setTextSize(2);
  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  tft.setCursor(x - (safeSpeed >= 10 ? 11 : 5), y - 7);
  tft.print(safeSpeed);
}

void MiniLanderGame::drawBriefing(TFT_eSPI& tft) {
  TDisplayUi::header(tft, "Landing Brief", TFT_CYAN, (String("L") + String(level_)).c_str());
  TDisplayUi::labelValue(tft, 41, "Fuel", String(static_cast<int>(startFuel_ + 0.5f)), TFT_GREEN);
  TDisplayUi::labelValue(tft, 65, "Gravity", String(static_cast<int>(levelGravity() + 0.5f)), TFT_YELLOW);
  TDisplayUi::labelValue(tft, 89, "Safe V", "<= " + String(static_cast<int>(levelSafeSpeed() + 0.5f)), TFT_CYAN);
  if (briefingTimerMs_ < BRIEFING_INPUT_LOCK_MS || !briefingCanAcceptInput_) {
    TDisplayUi::footer(tft, "Ready...");
  } else {
    TDisplayUi::footer(tft, "B1 start descent");
  }
}

void MiniLanderGame::drawHud(TFT_eSPI& tft) {
  TDisplayUi::header(tft, "Mini Lander", thrusting_ ? TFT_ORANGE : TFT_CYAN, (String("L") + String(level_)).c_str());
  tft.setTextSize(2);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("A " + String(static_cast<int>(altitude_ + 0.5f)), 10, 34);
  tft.drawString("V " + String(static_cast<int>(velocity_ + 0.5f)), 10, 56);
  const uint16_t fuelColor = fuel_ <= startFuel_ * 0.2f ? TFT_RED : TFT_GREEN;
  TDisplayUi::bar(tft, 10, 83, 64, 10, fuel_ / startFuel_, fuelColor);
  tft.setTextSize(1);
  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft.drawString("Fuel", 10, 96);
}

void MiniLanderGame::drawLander(TFT_eSPI& tft, int x, int y, bool thrusting) {
  tft.fillTriangle(x, y - 12, x - 9, y + 5, x + 9, y + 5, TFT_LIGHTGREY);
  tft.drawTriangle(x, y - 12, x - 9, y + 5, x + 9, y + 5, TFT_WHITE);
  tft.drawLine(x - 9, y + 5, x - 15, y + 13, TFT_WHITE);
  tft.drawLine(x + 9, y + 5, x + 15, y + 13, TFT_WHITE);
  tft.drawFastHLine(x - 18, y + 13, 11, TFT_WHITE);
  tft.drawFastHLine(x + 7, y + 13, 11, TFT_WHITE);
  if (thrusting) {
    tft.fillTriangle(x - 5, y + 7, x + 5, y + 7, x, y + 25, TFT_ORANGE);
    tft.drawLine(x - 2, y + 8, x, y + 21, TFT_YELLOW);
    tft.drawLine(x + 2, y + 8, x, y + 21, TFT_YELLOW);
  }
}

void MiniLanderGame::drawGround(TFT_eSPI& tft) {
  tft.drawLine(1, GROUND_Y, width - 2, GROUND_Y, TFT_LIGHTGREY);
  tft.fillRect(SHIP_X - 30, GROUND_Y - 2, 60, 4, TFT_GREEN);
  tft.drawPixel(30, GROUND_Y - 8, TFT_LIGHTGREY);
  tft.drawPixel(205, GROUND_Y - 13, TFT_LIGHTGREY);
}
