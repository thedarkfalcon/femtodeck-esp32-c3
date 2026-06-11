#include "Breakout76Game.h"

#include <Arduino.h>
#include <Preferences.h>
#include <TFT_eSPI.h>

#include "../../PlayerProfile.h"

namespace {
Preferences breakout76Prefs;
constexpr uint8_t BRICK_W = 7;
constexpr uint8_t BRICK_H = 3;
constexpr uint8_t PADDLE_Y = 36;
}

Breakout76Game::Breakout76Game(uint32_t width, uint32_t height)
    : App("Breakout '76", width, height) {}

bool Breakout76Game::hasCustomOverlay() const {
  return true;
}

void Breakout76Game::onAppReset() {
  loadBestScore();
  paddleX_ = 25.0f;
  paddleVel_ = 0.0f;
  paddleDir_ = 1;
  paddleW_ = 14;
  wideTimerMs_ = 0;
  level_ = 1;
  score_ = 0;
  for (uint8_t i = 0; i < BALLS; i++) balls_[i].active = false;
  for (uint8_t i = 0; i < POWERUPS; i++) powers_[i].active = false;
  buildLevel();
}

void Breakout76Game::buildLevel() {
  bricksLeft_ = 0;
  for (uint8_t r = 0; r < ROWS; r++) {
    for (uint8_t c = 0; c < COLS; c++) {
      bool on = true;
      if ((level_ % 3) == 2) on = ((r + c) % 2) == 0 || r == 0;
      if ((level_ % 3) == 0) on = (r == 0 || r == ROWS - 1 || c == 0 || c == COLS - 1 || r == c / 2);
      bricks_[r][c] = on;
      if (on) bricksLeft_++;
    }
  }
  paddleX_ = 25.0f;
  for (uint8_t i = 0; i < BALLS; i++) balls_[i].active = false;
  launchBall(0, false);
}

void Breakout76Game::launchBall(uint8_t slot, bool alternate) {
  if (slot >= BALLS) return;
  balls_[slot].active = true;
  balls_[slot].x = 18.0f + static_cast<float>(random(0, 34));
  balls_[slot].y = 25.0f + static_cast<float>(random(0, 4));
  const float speed = 21.0f + static_cast<float>(level_) * 1.5f;
  balls_[slot].vx = (alternate ? -1.0f : 1.0f) * (13.0f + static_cast<float>(random(0, 9)));
  balls_[slot].vy = -speed;
}

uint8_t Breakout76Game::activeBallCount() const {
  uint8_t count = 0;
  for (uint8_t i = 0; i < BALLS; i++) if (balls_[i].active) count++;
  return count;
}

void Breakout76Game::spawnPower(float x, float y) {
  if (random(0, 100) > 13) return;
  for (uint8_t i = 0; i < POWERUPS; i++) {
    if (!powers_[i].active) {
      powers_[i].active = true;
      powers_[i].x = x;
      powers_[i].y = y;
      powers_[i].type = random(0, 2);
      return;
    }
  }
}

void Breakout76Game::applyPower(uint8_t type) {
  if (type == 0) {
    paddleW_ = 22;
    wideTimerMs_ = 9000;
    return;
  }
  for (uint8_t i = 0; i < BALLS; i++) {
    if (!balls_[i].active) {
      launchBall(i, (i % 2) == 0);
      return;
    }
  }
}

void Breakout76Game::updateRunning(uint32_t deltaMs, const ButtonInput& b1, const ButtonInput& b2) {
  const float dt = static_cast<float>(deltaMs) * 0.001f;
  if (b1.click) paddleDir_ = -paddleDir_;

  if (wideTimerMs_ > 0) {
    wideTimerMs_ = deltaMs >= wideTimerMs_ ? 0 : wideTimerMs_ - deltaMs;
    if (wideTimerMs_ == 0) paddleW_ = 14;
  }

  const float oldPaddle = paddleX_;
  paddleX_ += static_cast<float>(paddleDir_) * 34.0f * dt;
  if (paddleX_ < 1.0f) {
    paddleX_ = 1.0f;
  }
  if (paddleX_ > static_cast<float>(width - paddleW_ - 1)) {
    paddleX_ = static_cast<float>(width - paddleW_ - 1);
  }
  paddleVel_ = (paddleX_ - oldPaddle) / max(dt, 0.001f);

  for (uint8_t p = 0; p < POWERUPS; p++) {
    if (!powers_[p].active) continue;
    powers_[p].y += 13.0f * dt;
    if (powers_[p].y >= PADDLE_Y - 1 && powers_[p].x >= paddleX_ && powers_[p].x <= paddleX_ + paddleW_) {
      applyPower(powers_[p].type);
      powers_[p].active = false;
    } else if (powers_[p].y > height) {
      powers_[p].active = false;
    }
  }

  for (uint8_t i = 0; i < BALLS; i++) {
    if (!balls_[i].active) continue;
    Ball& b = balls_[i];
    b.x += b.vx * dt;
    b.y += b.vy * dt;

    if (b.x <= 1.0f || b.x >= width - 2) {
      b.x = b.x <= 1.0f ? 1.0f : static_cast<float>(width - 2);
      b.vx = -b.vx;
    }
    if (b.y <= 1.0f) {
      b.y = 1.0f;
      b.vy = -b.vy;
    }

    if (b.y >= PADDLE_Y - 2 && b.y <= PADDLE_Y + 1 && b.x >= paddleX_ && b.x <= paddleX_ + paddleW_ && b.vy > 0) {
      const float hit = ((b.x - paddleX_) / static_cast<float>(paddleW_)) - 0.5f;
      b.vx = hit * 42.0f + paddleVel_ * 0.28f;
      b.vy = -(24.0f + static_cast<float>(level_) * 1.4f);
      b.y = PADDLE_Y - 3;
    }

    const int bx = static_cast<int>(b.x);
    const int by = static_cast<int>(b.y);
    bool hitBrick = false;
    for (uint8_t r = 0; r < ROWS && !hitBrick; r++) {
      for (uint8_t c = 0; c < COLS; c++) {
        if (!bricks_[r][c]) continue;
        const int x = 3 + c * BRICK_W;
        const int y = 3 + r * BRICK_H;
        if (bx >= x && bx < x + BRICK_W - 1 && by >= y && by < y + BRICK_H - 1) {
          bricks_[r][c] = false;
          bricksLeft_--;
          score_++;
          if (score_ > bestScore_) {
            bestScore_ = score_;
            saveBestScore();
          }
          spawnPower(static_cast<float>(x + 2), static_cast<float>(y + 2));
          b.vy = -b.vy;
          if ((level_ % 3) == 0 && c == COLS - 1) b.vx = -abs(b.vx) - 5.0f;
          hitBrick = true;
          break;
        }
      }
    }

    if (b.y > height) b.active = false;
  }

  if (bricksLeft_ == 0) {
    level_++;
    buildLevel();
    return;
  }

  if (activeBallCount() == 0) endApp();
}

void void Breakout76Game::drawRunning(TFT_eSPI& tft) { tft.fillScreen(TFT_BLACK); { tft.fillScreen(TFT_BLACK);
  tft.drawRect(, 0, width, height);
  for (uint8_t r = 0; r < ROWS; r++) {
    for (uint8_t c = 0; c < COLS; c++) {
      if (bricks_[r][c]) tft.fillRect(3 + c * BRICK_W, 3 + r * BRICK_H, BRICK_W - 1, BRICK_H - 1);
    }
  }
  for (uint8_t p = 0; p < POWERUPS; p++) {
    if (powers_[p].active) {
      if (powers_[p].type == 0) tft.fillRect(static_cast<int>(powers_[p].x), static_cast<int>(powers_[p].y), 4, 2);
      else tft.drawCircle(static_cast<int>(powers_[p].x), static_cast<int>(powers_[p].y), 2);
    }
  }
  for (uint8_t i = 0; i < BALLS; i++) {
    if (balls_[i].active) tft.fillRect(clampInt(static_cast<int>(balls_[i].x), 1, width - 2), clampInt(static_cast<int>(balls_[i].y), 1, height - 2), 2, 2);
  }
  tft.fillRect(static_cast<int>(paddleX_), PADDLE_Y, paddleW_, 2);

  tft.setCursor(2, 6);
  tft.print("L");
  tft.print(level_);
  tft.print(" ");
  tft.print(score_);
}

void Breakout76Game::drawStart(TFT_eSPI& tft) { tft.fillScreen(TFT_BLACK);
  loadBestScore();
  tft.drawRect(0, 0, width + 2, height);
  if (showStartPromptPage()) {

    tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.drawString(20, 16, "Press");
    tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.drawString(13, 29, "to Start");
  } else if (showStartScorePage()) {
    char initials[4];
    PlayerProfile::unpackDottedInitials(bestInitials_, initials);

    tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.drawString(3, 10, "Top Bricks");

    tft.setCursor(3, 24);
    if (bestScore_ == 0) tft.print("--");
    else {
      tft.print(initials);
      tft.print(" ");
      tft.print(bestScore_);
    }
  } else {
    tft.fillRect(6, 28, 20, 2);
    tft.fillCircle(44, 17, 2);
    tft.drawLine(12, 24, 30, 15);
    tft.drawLine(30, 15, 44, 17);
    tft.drawPixel(49, 15);
    tft.drawPixel(53, 13);

    tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.drawString(3, 9, appTitle());
  }
}

void Breakout76Game::drawEnd(TFT_eSPI& tft) { tft.fillScreen(TFT_BLACK);
  tft.drawRect(0, 0, width + 2, height);

  tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.drawString(3, 9, "Ball Lost");

  tft.setCursor(3, 20);
  tft.print("Bricks ");
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

int Breakout76Game::clampInt(int value, int minValue, int maxValue) const {
  if (value < minValue) return minValue;
  if (value > maxValue) return maxValue;
  return value;
}

void Breakout76Game::loadBestScore() {
  if (bestLoaded_) return;
  breakout76Prefs.begin("breakout76", true);
  bestScore_ = breakout76Prefs.getUShort("best", 0);
  bestInitials_ = breakout76Prefs.getUShort("init", PlayerProfile::defaultInitials());
  breakout76Prefs.end();
  bestLoaded_ = true;
}

void Breakout76Game::saveBestScore() {
  bestInitials_ = PlayerProfile::loadInitials();
  breakout76Prefs.begin("breakout76", false);
  breakout76Prefs.putUShort("best", bestScore_);
  breakout76Prefs.putUShort("init", bestInitials_);
  breakout76Prefs.end();
}
