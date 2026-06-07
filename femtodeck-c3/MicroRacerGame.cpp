#include "MicroRacerGame.h"

#include <Arduino.h>
#include <Preferences.h>
#include <U8g2lib.h>

#include "PlayerProfile.h"

namespace {
Preferences racerPrefs;
}

MicroRacerGame::MicroRacerGame(uint32_t width, uint32_t height, uint32_t left)
    : App("Micro Racer", width, height), left_(left) {}

bool MicroRacerGame::hasCustomOverlay() const {
  return true;
}

void MicroRacerGame::onAppReset() {
  loadBestScore();
  playerLane_ = 1;
  level_ = 1;
  score_ = 0;
  spawnTimerMs_ = 0;
  speedPxPerSec_ = 18.0f;
  for (uint8_t i = 0; i < OBSTACLE_COUNT; i++) {
    obstacles_[i].active = false;
  }
}

void MicroRacerGame::updateRunning(uint32_t deltaMs, const ButtonInput& input) {
  const float deltaSec = static_cast<float>(deltaMs) * 0.001f;

  if (input.click) {
    playerLane_ = (playerLane_ + 1) % LANE_COUNT;
  }

  spawnTimerMs_ += deltaMs;
  if (spawnTimerMs_ >= 550) {
    spawnTimerMs_ = 0;
    spawnObstacle();
  }

  const float move = speedPxPerSec_ * deltaSec;
  for (uint8_t i = 0; i < OBSTACLE_COUNT; i++) {
    if (!obstacles_[i].active) {
      continue;
    }
    obstacles_[i].y += move;
    if (obstacles_[i].y > static_cast<float>(height)) {
      obstacles_[i].active = false;
      score_++;
      level_ = static_cast<uint8_t>(1 + (score_ / 50));
      speedPxPerSec_ = min(54.0f, 18.0f + static_cast<float>(level_ - 1) * 4.0f);
      if (score_ > bestScore_) {
        bestScore_ = score_;
        saveBestScore();
      }
    }

    if (obstacles_[i].active &&
        obstacles_[i].lane == playerLane_ &&
        obstacles_[i].y >= static_cast<float>(PLAYER_Y - 3) &&
        obstacles_[i].y <= static_cast<float>(PLAYER_Y + 3)) {
      endApp();
    }
  }
}

void MicroRacerGame::drawRunning(U8G2& u8g2) {
  u8g2.drawFrame(left_, 0, width, height);
  u8g2.drawLine(left_ + 23, 1, left_ + 23, height - 2);
  u8g2.drawLine(left_ + 46, 1, left_ + 46, height - 2);

  for (uint8_t i = 0; i < OBSTACLE_COUNT; i++) {
    if (obstacles_[i].active) {
      u8g2.drawBox(left_ + laneX(obstacles_[i].lane), static_cast<int>(obstacles_[i].y), 8, 4);
    }
  }

  u8g2.drawFrame(left_ + laneX(playerLane_), PLAYER_Y, 8, 4);
  u8g2.setFont(u8g2_font_4x6_tr);
  u8g2.setCursor(left_ + 2, 6);
  u8g2.print("L");
  u8g2.print(level_);
  u8g2.print(" ");
  u8g2.print(score_);
}

void MicroRacerGame::drawStart(U8G2& u8g2) {
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
    u8g2.drawStr(3, 10, "Top Cars");
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
    u8g2.drawLine(18, 11, 7, 36);
    u8g2.drawLine(54, 11, 66, 36);
    u8g2.drawLine(32, 9, 30, 36);
    u8g2.drawLine(40, 9, 42, 36);
    u8g2.drawFrame(30, 23, 12, 7);
    u8g2.drawBox(32, 20, 8, 4);
    u8g2.drawPixel(31, 31);
    u8g2.drawPixel(40, 31);
    u8g2.setFont(u8g2_font_5x8_tr);
    u8g2.drawStr(3, 9, appTitle());
  }
}

void MicroRacerGame::drawEnd(U8G2& u8g2) {
  u8g2.drawFrame(0, 0, width + 2, height);
  u8g2.setFont(u8g2_font_5x8_tr);
  u8g2.drawStr(3, 9, "Crashed");
  u8g2.setFont(u8g2_font_4x6_tr);
  u8g2.setCursor(3, 20);
  u8g2.print("Cars ");
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

int MicroRacerGame::laneX(uint8_t lane) const {
  return 7 + lane * 23;
}

void MicroRacerGame::spawnObstacle() {
  for (uint8_t i = 0; i < OBSTACLE_COUNT; i++) {
    if (!obstacles_[i].active) {
      obstacles_[i].active = true;
      obstacles_[i].lane = random(0, LANE_COUNT);
      obstacles_[i].y = 0.0f;
      return;
    }
  }
}

void MicroRacerGame::loadBestScore() {
  if (bestLoaded_) {
    return;
  }
  racerPrefs.begin("racer", true);
  bestScore_ = racerPrefs.getUShort("best", 0);
  bestInitials_ = racerPrefs.getUShort("init", PlayerProfile::defaultInitials());
  racerPrefs.end();
  bestLoaded_ = true;
}

void MicroRacerGame::saveBestScore() {
  bestInitials_ = PlayerProfile::loadInitials();
  racerPrefs.begin("racer", false);
  racerPrefs.putUShort("best", bestScore_);
  racerPrefs.putUShort("init", bestInitials_);
  racerPrefs.end();
}
