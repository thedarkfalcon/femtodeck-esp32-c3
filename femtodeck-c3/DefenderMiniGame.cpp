#include "DefenderMiniGame.h"

#include <Arduino.h>
#include <Preferences.h>
#include <U8g2lib.h>

#include "PlayerProfile.h"

namespace {
Preferences defenderPrefs;
}

DefenderMiniGame::DefenderMiniGame(uint32_t width, uint32_t height, uint32_t left)
    : App("Defender Mini", width, height), left_(left) {}

bool DefenderMiniGame::hasCustomOverlay() const {
  return true;
}

void DefenderMiniGame::onAppReset() {
  loadBestScore();
  shipBand_ = 1;
  score_ = 0;
  health_ = 5;
  spawnTimerMs_ = 0;
  shotTimerMs_ = 0;
  for (uint8_t i = 0; i < ENEMY_COUNT; i++) {
    enemies_[i].active = false;
  }
  for (uint8_t i = 0; i < SHOT_COUNT; i++) {
    shots_[i].active = false;
  }
}

void DefenderMiniGame::updateRunning(uint32_t deltaMs, const ButtonInput& input) {
  const float deltaSec = static_cast<float>(deltaMs) * 0.001f;

  if (input.click) {
    shipBand_ = (shipBand_ + 1) % BAND_COUNT;
  }

  spawnTimerMs_ += deltaMs;
  if (spawnTimerMs_ >= 700) {
    spawnTimerMs_ = 0;
    spawnEnemy();
  }

  shotTimerMs_ += deltaMs;
  if (shotTimerMs_ >= 350) {
    shotTimerMs_ = 0;
    spawnShot();
  }

  const float enemyMove = 18.0f * deltaSec;
  const float shotMove = 40.0f * deltaSec;

  for (uint8_t i = 0; i < ENEMY_COUNT; i++) {
    if (!enemies_[i].active) {
      continue;
    }
    enemies_[i].x -= enemyMove;
    if (enemies_[i].x <= 5.0f && enemies_[i].band == shipBand_) {
      enemies_[i].active = false;
      loseHealth();
    } else if (enemies_[i].x < 0.0f) {
      enemies_[i].active = false;
      loseHealth();
    }
  }

  for (uint8_t i = 0; i < SHOT_COUNT; i++) {
    if (!shots_[i].active) {
      continue;
    }
    shots_[i].x += shotMove;
    if (shots_[i].x > static_cast<float>(width)) {
      shots_[i].active = false;
    }
  }

  for (uint8_t e = 0; e < ENEMY_COUNT; e++) {
    if (!enemies_[e].active) {
      continue;
    }
    for (uint8_t s = 0; s < SHOT_COUNT; s++) {
      if (!shots_[s].active) {
        continue;
      }
      if (shots_[s].band == enemies_[e].band &&
          shots_[s].x >= enemies_[e].x &&
          shots_[s].x <= enemies_[e].x + 3.0f) {
        shots_[s].active = false;
        enemies_[e].active = false;
        score_++;
        if (score_ > bestScore_) {
          bestScore_ = score_;
          saveBestScore();
        }
        break;
      }
    }
  }
}

void DefenderMiniGame::drawRunning(U8G2& u8g2) {
  u8g2.drawFrame(left_, 0, width, height);
  u8g2.drawBox(left_ + 3, bandY(shipBand_), 4, 3);
  for (uint8_t i = 0; i < ENEMY_COUNT; i++) {
    if (enemies_[i].active) {
      u8g2.drawFrame(left_ + static_cast<int>(enemies_[i].x), bandY(enemies_[i].band), 4, 3);
    }
  }
  for (uint8_t i = 0; i < SHOT_COUNT; i++) {
    if (shots_[i].active) {
      u8g2.drawPixel(left_ + static_cast<int>(shots_[i].x), bandY(shots_[i].band) + 1);
    }
  }
  u8g2.setFont(u8g2_font_4x6_tr);
  u8g2.setCursor(left_ + 2, 6);
  u8g2.print(score_);
  u8g2.setCursor(left_ + 45, 6);
  u8g2.print("H");
  u8g2.print(health_);
}

void DefenderMiniGame::drawStart(U8G2& u8g2) {
  loadBestScore();
  u8g2.drawFrame(0, 0, width + 2, height);
  if (showStartPromptPage()) {
    u8g2.setFont(u8g2_font_5x8_tr);
    u8g2.drawStr(20, 16, "Press");
    u8g2.drawStr(13, 29, "to Start");
  } else if (showStartScorePage()) {
    char initials[4];
    PlayerProfile::unpackDottedInitials(bestInitials_, initials);
    u8g2.setFont(u8g2_font_5x8_tr);
    u8g2.drawStr(3, 10, "Top Kills");
    u8g2.setFont(u8g2_font_4x6_tr);
    u8g2.setCursor(3, 24);
    if (bestScore_ == 0) {
      u8g2.print("--");
    } else {
      u8g2.print(initials);
      u8g2.print(" ");
      u8g2.print(bestScore_);
    }
  } else {
    u8g2.drawTriangle(15, 22, 5, 17, 5, 27);
    u8g2.drawLine(15, 22, 27, 22);
    u8g2.drawPixel(34, 22);
    u8g2.drawPixel(42, 18);
    u8g2.drawFrame(55, 17, 5, 5);
    u8g2.setFont(u8g2_font_5x8_tr);
    u8g2.drawStr(3, 9, appTitle());
  }
}

void DefenderMiniGame::drawEnd(U8G2& u8g2) {
  u8g2.drawFrame(0, 0, width + 2, height);
  u8g2.setFont(u8g2_font_5x8_tr);
  u8g2.drawStr(3, 9, "Defeated");
  u8g2.setFont(u8g2_font_4x6_tr);
  u8g2.setCursor(3, 20);
  u8g2.print("Kills ");
  u8g2.print(score_);
  u8g2.setCursor(3, 29);
  u8g2.print("Best ");
  u8g2.print(bestScore_);
  if (bestScore_ > 0) {
    char initials[4];
    PlayerProfile::unpackDottedInitials(bestInitials_, initials);
    u8g2.print(" ");
    u8g2.print(initials);
  }
  u8g2.drawStr(3, 38, "Tap retry Hold menu");
}

int DefenderMiniGame::bandY(uint8_t band) const {
  return 8 + band * 10;
}

void DefenderMiniGame::spawnEnemy() {
  for (uint8_t i = 0; i < ENEMY_COUNT; i++) {
    if (!enemies_[i].active) {
      enemies_[i].active = true;
      enemies_[i].x = static_cast<float>(width - 5);
      enemies_[i].band = random(0, BAND_COUNT);
      return;
    }
  }
}

void DefenderMiniGame::spawnShot() {
  for (uint8_t i = 0; i < SHOT_COUNT; i++) {
    if (!shots_[i].active) {
      shots_[i].active = true;
      shots_[i].x = 8.0f;
      shots_[i].band = shipBand_;
      return;
    }
  }
}

void DefenderMiniGame::loseHealth() {
  if (health_ > 0) {
    health_--;
  }
  if (health_ == 0) {
    endApp();
  }
}

void DefenderMiniGame::loadBestScore() {
  if (bestLoaded_) {
    return;
  }
  defenderPrefs.begin("defender", true);
  bestScore_ = defenderPrefs.getUShort("best", 0);
  bestInitials_ = defenderPrefs.getUShort("init", PlayerProfile::defaultInitials());
  defenderPrefs.end();
  bestLoaded_ = true;
}

void DefenderMiniGame::saveBestScore() {
  bestInitials_ = PlayerProfile::loadInitials();
  defenderPrefs.begin("defender", false);
  defenderPrefs.putUShort("best", bestScore_);
  defenderPrefs.putUShort("init", bestInitials_);
  defenderPrefs.end();
}
