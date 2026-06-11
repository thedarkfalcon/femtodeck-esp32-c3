#include "NuclearReactorGame.h"

#include <Arduino.h>
#include <Preferences.h>
#include <TFT_eSPI.h>

#include "../../PlayerProfile.h"

namespace {
Preferences reactorPrefs;
}

NuclearReactorGame::NuclearReactorGame(uint32_t width, uint32_t height)
    : App("Reactor", width, height) {}

bool NuclearReactorGame::hasCustomOverlay() const {
  return true;
}

void NuclearReactorGame::onAppReset() {
  loadBest();
  temp_ = 58.0f;
  rodsMs_ = 0;
  instability_ = 0.0f;
  currentPowerKw_ = 0.0f;
  scoreWh_ = 0;
  secondMs_ = 0;
  survivedSec_ = 0;
  meltdown_ = false;
  stalled_ = false;
}

void NuclearReactorGame::updateRunning(uint32_t deltaMs, const ButtonInput& b1, const ButtonInput& b2) {
  if (b1.pressed) {
    rodsMs_ = 520;
  }

  const float dt = static_cast<float>(deltaMs) * 0.001f;
  if (rodsMs_ > 0) {
    rodsMs_ = deltaMs >= rodsMs_ ? 0 : rodsMs_ - deltaMs;
  }

  instability_ += dt * 0.018f;
  const float noise = static_cast<float>(random(-18, 19)) * 0.020f * (1.0f + instability_);
  const float heat = 9.0f + instability_ * 2.8f + noise + max(0.0f, (temp_ - 70.0f) * 0.035f);
  const float cooling = rodsMs_ > 0 ? 58.0f : 3.0f;
  temp_ += (heat - cooling) * dt;
  if (temp_ < 0.0f) temp_ = 0.0f;
  currentPowerKw_ = max(0.0f, (temp_ - 28.0f) * 24.0f);

  secondMs_ += deltaMs;
  while (secondMs_ >= 1000) {
    secondMs_ -= 1000;
    survivedSec_++;
    scoreWh_ += static_cast<uint32_t>(currentPowerKw_ / 3.6f);
    if (scoreWh_ > bestWh_) {
      bestWh_ = scoreWh_;
      saveBest();
    }
  }

  if (temp_ >= 100.0f) {
    meltdown_ = true;
    endApp();
  } else if (temp_ <= 20.0f) {
    stalled_ = true;
    endApp();
  }
}

void void NuclearReactorGame::drawRunning(TFT_eSPI& tft) { tft.fillScreen(TFT_BLACK); { tft.fillScreen(TFT_BLACK);
  tft.drawRect(, 0, width, height);

  tft.setCursor(3, 7);
  tft.print(static_cast<unsigned long>(scoreWh_ / 1000));
  tft.print("kWh");
  tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.drawString(49, 7, rodsMs_ > 0 ? "ROD" : "HOT");

  tft.drawRect(4, 12, 10, 25);
  const uint8_t fill = constrain(static_cast<int>(temp_ * 0.23f), 0, 23);
  tft.fillRect(5, 36 - fill, 8, fill);
  tft.drawRect(18, 11, 50, 7);
  tft.fillRect(19, 12, constrain(static_cast<int>(temp_ * 0.48f), 0, 48), 5);


  tft.setCursor(18, 29);
  tft.print(static_cast<unsigned long>(currentPowerKw_));
  tft.print("kW");

  tft.setCursor(18, 38);
  tft.print(static_cast<int>(temp_));
  tft.print("C Tap rods");
}

void NuclearReactorGame::drawStart(TFT_eSPI& tft) { tft.fillScreen(TFT_BLACK);
  loadBest();
  tft.drawRect(0, 0, width + 2, height);
  if (showStartPromptPage()) {

    tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.drawString(20, 16, "Press");
    tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.drawString(13, 29, "to Start");
  } else if (showStartScorePage()) {
    char initials[4];
    PlayerProfile::unpackDottedInitials(bestInitials_, initials);

    tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.drawString(3, 10, "Top Power");

    tft.setCursor(3, 24);
    if (bestWh_ == 0) tft.print("--");
    else {
      tft.print(initials);
      tft.print(" ");
      tft.print(static_cast<unsigned long>(bestWh_ / 1000));
      tft.print("kWh");
    }
  } else {
    const int cx = 53;
    const int cy = 23;
    tft.drawCircle(cx, cy, 3);
    for (uint8_t i = 0; i < 3; i++) {
      const float a = i * 2.094f;
      const int x1 = cx + static_cast<int>(cosf(a) * 5);
      const int y1 = cy + static_cast<int>(sinf(a) * 5);
      const int x2 = cx + static_cast<int>(cosf(a - 0.55f) * 13);
      const int y2 = cy + static_cast<int>(sinf(a - 0.55f) * 13);
      const int x3 = cx + static_cast<int>(cosf(a + 0.55f) * 13);
      const int y3 = cy + static_cast<int>(sinf(a + 0.55f) * 13);
      tft.drawTriangle(x1, y1, x2, y2, x3, y3);
    }

    tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.drawString(3, 10, "Reactor");
  }
}

void NuclearReactorGame::drawEnd(TFT_eSPI& tft) { tft.fillScreen(TFT_BLACK);
  tft.drawRect(0, 0, width + 2, height);

  tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.drawString(3, 9, meltdown_ ? "Meltdown" : stalled_ ? "Stalled" : "Shutdown");

  tft.setCursor(3, 20);
  tft.print("Power ");
  tft.print(static_cast<unsigned long>(scoreWh_ / 1000));
  tft.print("kWh");
  tft.setCursor(3, 29);
  tft.print("Time ");
  tft.print(survivedSec_);
  tft.print("s");
  tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.drawString(3, 38, "Tap retry Hold menu");
}

void NuclearReactorGame::loadBest() {
  if (bestLoaded_) return;
  reactorPrefs.begin("reactor", true);
  bestWh_ = reactorPrefs.getULong("best", 0);
  bestInitials_ = reactorPrefs.getUShort("init", PlayerProfile::defaultInitials());
  reactorPrefs.end();
  bestLoaded_ = true;
}

void NuclearReactorGame::saveBest() {
  bestInitials_ = PlayerProfile::loadInitials();
  reactorPrefs.begin("reactor", false);
  reactorPrefs.putULong("best", bestWh_);
  reactorPrefs.putUShort("init", bestInitials_);
  reactorPrefs.end();
}
