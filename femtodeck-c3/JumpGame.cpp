#include "JumpGame.h"

#include <Preferences.h>
#include <U8g2lib.h>

#include "PlayerProfile.h"

namespace {
Preferences jumpPrefs;
}

JumpGame::JumpGame(uint32_t width, uint32_t height, uint32_t left)
    : App("Jump Run", width, height), left_(left) {}

bool JumpGame::hasCustomOverlay() const {
  return true;
}

void JumpGame::onAppReset() {
  loadBestScore();
  playerYPos_ = static_cast<float>(groundY() - PLAYER_H);
  playerVy_ = 0.0f;
  obstacleSpeed_ = 15.0f;
  spawnTimerMs_ = 0;
  nextSpawnMs_ = 1250;
  spawnPattern_ = 0;
  score_ = 0;
  for (uint8_t i = 0; i < OBSTACLE_COUNT; i++) {
    obstacles_[i].active = false;
  }
}

void JumpGame::updateRunning(uint32_t deltaMs, const ButtonInput& input) {
  const float deltaSec = static_cast<float>(deltaMs) * 0.001f;
  const float gravity = 72.0f;
  const float jumpVelocity = -42.0f;

  if (input.click && isOnGround()) {
    playerVy_ = jumpVelocity;
  }

  playerVy_ += gravity * deltaSec;
  playerYPos_ += playerVy_ * deltaSec;

  const float floorY = static_cast<float>(groundY() - PLAYER_H);
  if (playerYPos_ >= floorY) {
    playerYPos_ = floorY;
    playerVy_ = 0.0f;
  }

  spawnTimerMs_ += deltaMs;
  if (spawnTimerMs_ >= nextSpawnMs_) {
    spawnTimerMs_ = 0;
    spawnObstacle();
    nextSpawnMs_ = static_cast<uint16_t>(1180 + ((spawnPattern_ % 4) * 180));
  }

  for (uint8_t i = 0; i < OBSTACLE_COUNT; i++) {
    if (!obstacles_[i].active) {
      continue;
    }
    obstacles_[i].x -= obstacleSpeed_ * deltaSec;
    if ((obstacles_[i].x + obstacles_[i].w) < 0.0f) {
      obstacles_[i].active = false;
      score_++;
      if (score_ > bestScore_) {
        bestScore_ = score_;
        saveBestScore();
      }
      if ((score_ % 8) == 0 && obstacleSpeed_ < 24.0f) {
        obstacleSpeed_ += 1.0f;
      }
      continue;
    }
    if (intersectsObstacle(obstacles_[i])) {
      endApp();
      return;
    }
  }
}

void JumpGame::drawRunning(U8G2& u8g2) {
  u8g2.drawFrame(left_, 0, width, height);
  u8g2.drawLine(left_ + 1, groundY(), left_ + width - 2, groundY());

  for (uint8_t i = 0; i < OBSTACLE_COUNT; i++) {
    if (!obstacles_[i].active) {
      continue;
    }
    u8g2.drawBox(
        left_ + static_cast<int>(obstacles_[i].x),
        groundY() - obstacles_[i].h,
        obstacles_[i].w,
        obstacles_[i].h);
  }

  u8g2.drawBox(left_ + PLAYER_X, playerY(), PLAYER_W, PLAYER_H);
  u8g2.setFont(u8g2_font_4x6_tr);
  u8g2.setCursor(left_ + 2, 6);
  u8g2.print(score_);
}

int JumpGame::groundY() const {
  return static_cast<int>(height) - GROUND_MARGIN;
}

int JumpGame::playerY() const {
  return static_cast<int>(playerYPos_);
}

bool JumpGame::isOnGround() const {
  return playerYPos_ >= static_cast<float>(groundY() - PLAYER_H - 0.1f);
}

void JumpGame::spawnObstacle() {
  // Keep a minimum horizontal gap so obstacles are readable and fair.
  const int minGapPx = 28;
  float rightMostX = -1000.0f;
  bool hasActiveObstacle = false;

  for (uint8_t i = 0; i < OBSTACLE_COUNT; i++) {
    if (!obstacles_[i].active) {
      continue;
    }
    hasActiveObstacle = true;
    if (obstacles_[i].x > rightMostX) {
      rightMostX = obstacles_[i].x;
    }
  }

  if (hasActiveObstacle && rightMostX > static_cast<float>(width - minGapPx)) {
    return;
  }

  for (uint8_t i = 0; i < OBSTACLE_COUNT; i++) {
    if (!obstacles_[i].active) {
      obstacles_[i].active = true;
      obstacles_[i].x = static_cast<float>(width - 4);
      obstacles_[i].w = 3 + (spawnPattern_ % 2);
      obstacles_[i].h = 4 + ((spawnPattern_ * 2) % 4);
      spawnPattern_++;
      return;
    }
  }
}

bool JumpGame::intersectsObstacle(const Obstacle& obstacle) const {
  const int playerLeft = PLAYER_X;
  const int playerTop = playerY();
  const int playerRight = PLAYER_X + PLAYER_W - 1;
  const int playerBottom = playerTop + PLAYER_H - 1;

  const int obstacleLeft = static_cast<int>(obstacle.x);
  const int obstacleTop = groundY() - obstacle.h;
  const int obstacleRight = obstacleLeft + obstacle.w - 1;
  const int obstacleBottom = groundY() - 1;

  if (playerRight < obstacleLeft || playerLeft > obstacleRight) {
    return false;
  }
  if (playerBottom < obstacleTop || playerTop > obstacleBottom) {
    return false;
  }
  return true;
}

void JumpGame::drawStart(U8G2& u8g2) {
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
    u8g2.drawStr(3, 10, "Top Jumps");
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
    u8g2.drawLine(2, 34, 69, 34);
    u8g2.drawCircle(24, 16, 2);
    u8g2.drawLine(24, 18, 21, 25);
    u8g2.drawLine(22, 21, 16, 18);
    u8g2.drawLine(22, 21, 29, 18);
    u8g2.drawLine(21, 25, 15, 31);
    u8g2.drawLine(21, 25, 30, 29);
    u8g2.drawBox(49, 27, 4, 7);
    u8g2.setFont(u8g2_font_5x8_tr);
    u8g2.drawStr(3, 9, appTitle());
  }
}

void JumpGame::drawEnd(U8G2& u8g2) {
  u8g2.drawFrame(0, 0, width + 2, height);
  u8g2.setFont(u8g2_font_5x8_tr);
  u8g2.drawStr(3, 9, "Tripped");
  u8g2.setFont(u8g2_font_4x6_tr);
  u8g2.setCursor(3, 20);
  u8g2.print("Jumps ");
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

void JumpGame::loadBestScore() {
  if (bestLoaded_) {
    return;
  }
  jumpPrefs.begin("jump", true);
  bestScore_ = jumpPrefs.getUShort("best", 0);
  bestInitials_ = jumpPrefs.getUShort("init", PlayerProfile::defaultInitials());
  jumpPrefs.end();
  bestLoaded_ = true;
}

void JumpGame::saveBestScore() {
  bestInitials_ = PlayerProfile::loadInitials();
  jumpPrefs.begin("jump", false);
  jumpPrefs.putUShort("best", bestScore_);
  jumpPrefs.putUShort("init", bestInitials_);
  jumpPrefs.end();
}
