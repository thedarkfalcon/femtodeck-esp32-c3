#include "CityRacerGame.h"

#include <Arduino.h>
#include <Preferences.h>
#include <TFT_eSPI.h>

#include "../../PlayerProfile.h"

namespace {
Preferences cityRacerPrefs;
constexpr int PLAYER_Y = 30;
}

CityRacerGame::CityRacerGame(uint32_t width, uint32_t height)
    : App("City Racer", width, height) {}

bool CityRacerGame::hasCustomOverlay() const {
  return true;
}

void CityRacerGame::onAppReset() {
  loadBestScore();
  playerLane_ = 1;
  plannedSafeLane_ = playerLane_;
  rowsSinceLaneChange_ = 0;
  level_ = 1;
  score_ = 0;
  speed_ = 30.0f;
  spawnIntervalMs_ = 1250;
  spawnTimerMs_ = 900;
  for (uint8_t i = 0; i < ROWS; i++) rows_[i].active = false;
}

void CityRacerGame::updateRunning(uint32_t deltaMs, const ButtonInput& b1, const ButtonInput& b2) {
  const float dt = static_cast<float>(deltaMs) * 0.001f;
  if (b1.click) playerLane_ = (playerLane_ + 1) % LANES;

  level_ = 1 + min<uint16_t>(9, score_ / 35);
  speed_ = 30.0f + static_cast<float>(min<uint8_t>(level_ - 1, 7)) * 3.8f;
  spawnIntervalMs_ = 1250 - min<uint16_t>(420, max<int>(0, level_ - 1) * 45);

  spawnTimerMs_ += deltaMs;
  if (spawnTimerMs_ >= spawnIntervalMs_) {
    spawnTimerMs_ = 0;
    spawnRow();
  }

  for (uint8_t i = 0; i < ROWS; i++) {
    if (!rows_[i].active) continue;
    rows_[i].y += speed_ * dt;
    if (!rows_[i].counted && rows_[i].y > PLAYER_Y + 8) {
      rows_[i].counted = true;
      for (uint8_t lane = 0; lane < LANES; lane++) {
        if (rows_[i].mask & (1 << lane)) score_++;
      }
      if (score_ > bestScore_) {
        bestScore_ = score_;
        saveBestScore();
      }
    }
    if (rows_[i].y > height + 8) rows_[i].active = false;
    if (rows_[i].y >= PLAYER_Y - 7 && rows_[i].y <= PLAYER_Y + 7 && (rows_[i].mask & (1 << playerLane_))) {
      endApp();
      return;
    }
  }
}

uint8_t CityRacerGame::chooseSafeLane() {
  if (rowsSinceLaneChange_ == 0) {
    rowsSinceLaneChange_++;
    return plannedSafeLane_;
  }
  const uint8_t choice = random(0, 100);
  if (rowsSinceLaneChange_ < 3 && choice < 52) {
    rowsSinceLaneChange_++;
    return plannedSafeLane_;
  }
  rowsSinceLaneChange_ = 0;
  return (plannedSafeLane_ + 1) % LANES;
}

void CityRacerGame::spawnRow() {
  for (uint8_t i = 0; i < ROWS; i++) {
    if (rows_[i].active && rows_[i].y < 15.0f) return;
  }
  for (uint8_t i = 0; i < ROWS; i++) {
    if (rows_[i].active) continue;
    plannedSafeLane_ = chooseSafeLane();
    uint8_t mask = 0;
    const uint8_t firstBlocked = (plannedSafeLane_ + (random(0, 100) < 55 ? 1 : 2)) % LANES;
    mask |= (1 << firstBlocked);
    const bool addSecondCar = level_ >= 3 && random(0, 100) < min<uint8_t>(48, 18 + (level_ - 3) * 5);
    if (addSecondCar) {
      const uint8_t secondBlocked = (plannedSafeLane_ + 3 - (firstBlocked - plannedSafeLane_ + 3) % 3) % LANES;
      mask |= (1 << secondBlocked);
    }
    rows_[i].active = true;
    rows_[i].counted = false;
    rows_[i].y = -8.0f;
    rows_[i].mask = mask;
    return;
  }
}

void void CityRacerGame::drawRunning(TFT_eSPI& tft) { tft.fillScreen(TFT_BLACK); { tft.fillScreen(TFT_BLACK);
  tft.drawRect(, 0, width, height);
  tft.drawLine(23, 1, 23, height - 2);
  tft.drawLine(46, 1, 46, height - 2);
  for (uint8_t i = 0; i < ROWS; i++) {
    if (!rows_[i].active) continue;
    for (uint8_t lane = 0; lane < LANES; lane++) {
      if (rows_[i].mask & (1 << lane)) drawCar(tft, laneX(lane), static_cast<int>(rows_[i].y), false);
    }
  }
  drawCar(tft, laneX(playerLane_), PLAYER_Y, true);

  tft.setCursor(2, 6);
  tft.print("L");
  tft.print(level_);
  tft.print(" ");
  tft.print(score_);
}

void CityRacerGame::drawCar(TFT_eSPI& tft, int x, int y, bool player) const {
  if (player) {
    tft.drawRect(x + 1, y, 6, 8);
    tft.fillRect(x + 2, y + 1, 4, 2);
  } else {
    tft.fillRect(x + 1, y, 6, 8);

    tft.drawPixel(x + 2, y + 1);
    tft.drawPixel(x + 5, y + 1);

  }
}

void CityRacerGame::drawStart(TFT_eSPI& tft) { tft.fillScreen(TFT_BLACK);
  loadBestScore();
  tft.drawRect(0, 0, width + 2, height);
  if (showStartPromptPage()) {

    tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.drawString(20, 16, "Press");
    tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.drawString(13, 29, "to Start");
  } else if (showStartScorePage()) {
    char initials[4];
    PlayerProfile::unpackDottedInitials(bestInitials_, initials);

    tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.drawString(3, 10, "Top Cars");

    tft.setCursor(3, 24);
    if (bestScore_ == 0) tft.print("--");
    else {
      tft.print(initials);
      tft.print(" ");
      tft.print(bestScore_);
    }
  } else {
    tft.drawLine(18, 11, 7, 36);
    tft.drawLine(54, 11, 66, 36);
    tft.drawLine(32, 9, 30, 36);
    tft.drawLine(40, 9, 42, 36);
    drawCar(tft, 31, 22, true);

    tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.drawString(3, 9, appTitle());
  }
}

void CityRacerGame::drawEnd(TFT_eSPI& tft) { tft.fillScreen(TFT_BLACK);
  tft.drawRect(0, 0, width + 2, height);

  tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.drawString(3, 9, "Crashed");

  tft.setCursor(3, 20);
  tft.print("Cars ");
  tft.print(score_);
  tft.setCursor(3, 29);
  tft.print("Best ");
  tft.print(bestScore_);
  if (bestScore_ > 0) {
    char initials[4];
    PlayerProfile::unpackDottedInitials(bestInitials_, initials);
    tft.print(" ");
    tft.print(initials);
  }
  tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.drawString(3, 38, "Tap retry Hold menu");
}

int CityRacerGame::laneX(uint8_t lane) const {
  return 7 + lane * 23;
}

void CityRacerGame::loadBestScore() {
  if (bestLoaded_) return;
  cityRacerPrefs.begin("cityrace", true);
  bestScore_ = cityRacerPrefs.getUShort("best", 0);
  bestInitials_ = cityRacerPrefs.getUShort("init", PlayerProfile::defaultInitials());
  cityRacerPrefs.end();
  bestLoaded_ = true;
}

void CityRacerGame::saveBestScore() {
  bestInitials_ = PlayerProfile::loadInitials();
  cityRacerPrefs.begin("cityrace", false);
  cityRacerPrefs.putUShort("best", bestScore_);
  cityRacerPrefs.putUShort("init", bestInitials_);
  cityRacerPrefs.end();
}
