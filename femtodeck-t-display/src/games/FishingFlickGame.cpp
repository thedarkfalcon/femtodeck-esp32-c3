#include "FishingFlickGame.h"

#include <Arduino.h>
#include <Preferences.h>
#include <TFT_eSPI.h>

#include "../../PlayerProfile.h"
#include "../../TDisplayUi.h"

namespace {
constexpr uint8_t FISH_LED_PIN = 8;
constexpr bool FISH_LED_ACTIVE_LOW = true;
constexpr bool FISH_LED_AVAILABLE = false;
Preferences fishPrefs;
}

FishingFlickGame::FishingFlickGame(uint32_t width, uint32_t height)
    : App("Fishing Flick", width, height) {}

bool FishingFlickGame::hasCustomOverlay() const {
  return true;
}

void FishingFlickGame::onAppReset() {
  if (FISH_LED_AVAILABLE) {
    pinMode(FISH_LED_PIN, OUTPUT);
  }
  setWarningLed(false);
  loadBestFish();
  level_ = 1;
  caughtWeight_ = 0;
  loadLevel();
}

void FishingFlickGame::loadLevel() {
  fishState_ = FishState::Waiting;
  waitMs_ = static_cast<uint16_t>(random(700, 1900));
  elapsedMs_ = 0;
  hookTimerMs_ = 0;
  fightTimerMs_ = static_cast<uint16_t>(random(500, 1000));
  caughtTimerMs_ = 0;
  fishWeight_ = static_cast<uint16_t>(180 + (level_ * 120) + random(0, 140 + level_ * 25));
  progress_ = 0.0f;
  tension_ = 34.0f;
  fighting_ = false;
  setWarningLed(false);
}

void FishingFlickGame::updateRunning(uint32_t deltaMs, const ButtonInput& b1, const ButtonInput& b2) {
  (void)b2;
  const float deltaSec = static_cast<float>(deltaMs) * 0.001f;

  if (fishState_ == FishState::Waiting) {
    elapsedMs_ += deltaMs;
    if (elapsedMs_ >= waitMs_) {
      fishState_ = FishState::Hooking;
      hookTimerMs_ = 0;
    }
    setWarningLed(false);
    return;
  }

  if (fishState_ == FishState::Hooking) {
    hookTimerMs_ += deltaMs;
    if (b1.pressed) {
      fishState_ = FishState::Reeling;
      tension_ = 36.0f;
      progress_ = 6.0f;
      fighting_ = false;
      fightTimerMs_ = 650;
      return;
    }
    if (hookTimerMs_ >= 950) {
      fishState_ = FishState::Escaped;
      setWarningLed(false);
      endApp();
    }
    return;
  }

  if (fishState_ != FishState::Reeling) {
    if (fishState_ == FishState::Caught) {
      caughtTimerMs_ += deltaMs;
      if (caughtTimerMs_ >= 1100) {
        level_++;
        loadLevel();
      }
    }
    setWarningLed(false);
    return;
  }

  if (fightTimerMs_ > deltaMs) {
    fightTimerMs_ -= deltaMs;
  } else {
    fighting_ = !fighting_;
    if (fighting_) {
      startFightBurst();
    }
    fightTimerMs_ = static_cast<uint16_t>(random(fighting_ ? 360 : 550, fighting_ ? 760 : 1200));
  }

  const float hard = difficulty();
  if (b1.down) {
    progress_ += (fighting_ ? (4.0f / hard) : (32.0f / hard)) * deltaSec;
    tension_ += (fighting_ ? (42.0f + hard * 12.0f) : (7.0f + hard * 2.0f)) * deltaSec;
  } else {
    progress_ -= (fighting_ ? 2.0f : 0.8f) * deltaSec;
    tension_ -= (fighting_ ? (74.0f + hard * 7.0f) : (42.0f + hard * 4.0f)) * deltaSec;
  }

  tension_ = max(0.0f, min(110.0f, tension_));
  progress_ = max(0.0f, min(100.0f, progress_));
  updateWarningLed();

  if (tension_ <= 0.0f) {
    fishState_ = FishState::Escaped;
    setWarningLed(false);
    endApp();
    return;
  }

  if (tension_ >= 100.0f) {
    fishState_ = FishState::Broken;
    setWarningLed(false);
    endApp();
    return;
  }

  if (progress_ >= 100.0f) {
    fishState_ = FishState::Caught;
    caughtTimerMs_ = 0;
    caughtWeight_ = fishWeight_;
    if (caughtWeight_ > bestWeight_) {
      bestWeight_ = caughtWeight_;
      saveBestFish();
    }
    setWarningLed(false);
  }
}

void FishingFlickGame::drawRunning(TFT_eSPI& tft) {
  TDisplayUi::clear(tft);
  TDisplayUi::header(tft, "Fishing Flick", TFT_CYAN, (String("L") + String(level_) + " " + String(fishWeight_) + "g").c_str());
  drawWater(tft);

  if (fishState_ == FishState::Waiting) {
    TDisplayUi::centered(tft, "Waiting...", 43, 3, TFT_LIGHTGREY);
    tft.drawLine(54, 42, 115, 88, TFT_YELLOW);
    tft.drawLine(115, 88, 124, 86, TFT_WHITE);
    tft.drawCircle(126, 89, 2, TFT_WHITE);
    TDisplayUi::footer(tft, "Wait for the tug");
    return;
  }

  if (fishState_ == FishState::Hooking) {
    const int tugOffset = ((millis() / 120) % 2 == 0) ? 3 : -3;
    TDisplayUi::centered(tft, "TUG!", 39, 4, TFT_YELLOW);
    tft.drawLine(58, 42, 116 + tugOffset * 3, 88, TFT_YELLOW);
    tft.drawCircle(124 + tugOffset * 3, 91, 3, TFT_WHITE);
    TDisplayUi::footer(tft, "B1 quick hook");
    return;
  }

  if (fishState_ == FishState::Caught) {
    TDisplayUi::centered(tft, "CAUGHT!", 38, 4, TFT_GREEN);
    TDisplayUi::largeValue(tft, String(caughtWeight_) + "g", 79, TFT_CYAN);
    TDisplayUi::footer(tft, "Next fish...");
    return;
  }

  TDisplayUi::labelValue(tft, 40, "Mode", fighting_ ? "FIGHT" : "REEL", fighting_ ? TFT_RED : TFT_GREEN);
  TDisplayUi::labelValue(tft, 64, "Fish", String(fishWeight_) + "g", TFT_CYAN);
  TDisplayUi::bar(tft, 68, 91, 152, 9, progress_ / 100.0f, TFT_GREEN);
  TDisplayUi::bar(tft, 68, 108, 152, 9, tension_ / 100.0f, tension_ > 78.0f ? TFT_RED : (tension_ < 15.0f ? TFT_CYAN : TFT_YELLOW));
  tft.setTextSize(1);
  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft.drawString("Catch", 16, 91);
  tft.drawString("Line", 16, 108);
  if (tension_ > 78.0f) {
    TDisplayUi::footer(tft, "LINE! release before it breaks");
  } else if (tension_ < 15.0f) {
    TDisplayUi::footer(tft, "SLACK! hold to reel");
  } else {
    TDisplayUi::footer(tft, "Hold reel / release during fight");
  }
}

void FishingFlickGame::drawStart(TFT_eSPI& tft) { tft.fillScreen(TFT_BLACK);
  loadBestFish();
  TDisplayUi::clear(tft);
  if (showStartPromptPage()) {
    TDisplayUi::header(tft, "Fishing Flick", TFT_CYAN);
    TDisplayUi::centered(tft, "Press to Start", 56, 2, TFT_WHITE);
    TDisplayUi::footer(tft, "Hook, reel, release");
  } else if (showStartScorePage()) {
    char initials[4];
    PlayerProfile::unpackDottedInitials(bestInitials_, initials);
    TDisplayUi::header(tft, "Top Fish", TFT_CYAN);
    String score = bestWeight_ == 0 ? "--" : String(initials) + " " + String(bestWeight_) + "g";
    TDisplayUi::largeValue(tft, score, 54, TFT_CYAN);
    TDisplayUi::footer(tft, "Heaviest fish");
  } else {
    TDisplayUi::header(tft, "Fishing Flick", TFT_CYAN);
    drawWater(tft);
    tft.drawLine(52, 43, 112, 88, TFT_YELLOW);
    tft.drawLine(112, 88, 126, 86, TFT_WHITE);
    tft.drawCircle(132, 91, 4, TFT_CYAN);
    TDisplayUi::centered(tft, "Tap the tug", 105, 1, TFT_LIGHTGREY);
  }
}

void FishingFlickGame::drawEnd(TFT_eSPI& tft) { tft.fillScreen(TFT_BLACK);
  setWarningLed(false);
  const char* title = fishState_ == FishState::Escaped ? "Escaped" : "Line broke";
  TDisplayUi::clear(tft);
  TDisplayUi::header(tft, title, fishState_ == FishState::Escaped ? TFT_CYAN : TFT_RED);
  String fish = "--";
  if (caughtWeight_ == 0) {
    fish = "--";
  } else {
    fish = String(caughtWeight_) + "g";
  }
  String best = "--";
  if (bestWeight_ == 0) {
    best = "--";
  } else {
    best = String(bestWeight_) + "g";
    char initials[4];
    PlayerProfile::unpackDottedInitials(bestInitials_, initials);
    best += " ";
    best += initials;
  }
  TDisplayUi::labelValue(tft, 49, "Fish", fish, TFT_YELLOW);
  TDisplayUi::labelValue(tft, 78, "Best", best, TFT_GREEN);
  TDisplayUi::footer(tft, "B1 retry / B1 hold menu");
}

float FishingFlickGame::difficulty() const {
  return min(3.3f, 0.75f + static_cast<float>(level_ - 1) * 0.22f);
}

void FishingFlickGame::startFightBurst() {
  tension_ += 7.0f + (difficulty() * 3.5f);
  if (tension_ > 110.0f) {
    tension_ = 110.0f;
  }
}

void FishingFlickGame::loadBestFish() {
  if (bestLoaded_) {
    return;
  }
  fishPrefs.begin("fish", true);
  bestWeight_ = fishPrefs.getUShort("best", 0);
  bestInitials_ = fishPrefs.getUShort("init", PlayerProfile::defaultInitials());
  fishPrefs.end();
  bestLoaded_ = true;
}

void FishingFlickGame::saveBestFish() {
  bestInitials_ = PlayerProfile::loadInitials();
  fishPrefs.begin("fish", false);
  fishPrefs.putUShort("best", bestWeight_);
  fishPrefs.putUShort("init", bestInitials_);
  fishPrefs.end();
}

void FishingFlickGame::setWarningLed(bool on) {
  if (!FISH_LED_AVAILABLE) {
    return;
  }
  digitalWrite(FISH_LED_PIN, FISH_LED_ACTIVE_LOW ? !on : on);
}

void FishingFlickGame::updateWarningLed() {
  if (tension_ < 80.0f) {
    setWarningLed(false);
    return;
  }
  setWarningLed(((millis() / 120) % 2) == 0);
}

void FishingFlickGame::drawWater(TFT_eSPI& tft) {
  tft.fillRect(0, 96, width, height - 96, TFT_NAVY);
  tft.drawFastHLine(0, 96, width, TFT_CYAN);
  for (uint8_t i = 0; i < 9; i++) {
    const int x = 6 + i * 27;
    tft.drawLine(x, 111, x + 9, 107, TFT_CYAN);
    tft.drawLine(x + 9, 107, x + 18, 111, TFT_CYAN);
  }
}
