#include "BreakoutGame.h"

#include <Preferences.h>
#include <U8g2lib.h>

#include "PlayerProfile.h"

namespace {
Preferences breakoutPrefs;

int clampInt(int value, int minValue, int maxValue) {
  if (value < minValue) {
    return minValue;
  }
  if (value > maxValue) {
    return maxValue;
  }
  return value;
}

float clampFloat(float value, float minValue, float maxValue) {
  if (value < minValue) {
    return minValue;
  }
  if (value > maxValue) {
    return maxValue;
  }
  return value;
}
}  // namespace

BreakoutGame::BreakoutGame(uint32_t width, uint32_t height, uint32_t left)
    : App("Breakout", width, height), left_(left) {}

bool BreakoutGame::hasCustomOverlay() const {
  return true;
}

void BreakoutGame::onAppReset() {
  loadBestScore();
  paddleX_ = 24.0f;
  paddleDir_ = 1;
  level_ = 1;
  score_ = 0;
  loadLevel();
}

void BreakoutGame::loadLevel() {
  paddleX_ = 24.0f;
  paddleDir_ = 1;
  resetBall();
  const float speed = 1.0f + static_cast<float>(level_ - 1) * 0.12f;
  ballVX_ *= speed;
  ballVY_ *= speed;
  bricksLeft_ = 0;
  for (uint8_t row = 0; row < BRICK_ROWS; row++) {
    for (uint8_t col = 0; col < BRICK_COLS; col++) {
      bricks_[row][col] = true;
      bricksLeft_++;
    }
  }
}

void BreakoutGame::resetBall() {
  ballX_ = 35.0f;
  ballY_ = 20.0f;
  ballVX_ = 24.0f;
  ballVY_ = -22.0f;
}

void BreakoutGame::updateRunning(uint32_t deltaMs, const ButtonInput& input) {
  const float deltaSec = static_cast<float>(deltaMs) * 0.001f;
  const float prevBallX = ballX_;
  const float prevBallY = ballY_;

  if (input.click) {
    paddleDir_ *= -1;
  }

  paddleX_ += static_cast<float>(paddleDir_ * PADDLE_SPEED) * deltaSec;
  paddleX_ = clampFloat(paddleX_, 1.0f, static_cast<float>(width - PADDLE_WIDTH - 1));

  ballX_ += ballVX_ * deltaSec;
  ballY_ += ballVY_ * deltaSec;

  if (ballX_ <= 1.0f) {
    ballX_ = 1.0f;
    ballVX_ = -ballVX_;
  } else if (ballX_ >= static_cast<float>(width - 2)) {
    ballX_ = static_cast<float>(width - 2);
    ballVX_ = -ballVX_;
  }

  if (ballY_ <= 1.0f) {
    ballY_ = 1.0f;
    ballVY_ = -ballVY_;
  }

  const int paddleY = static_cast<int>(height) - 4;
  if (ballY_ >= static_cast<float>(paddleY - 1) &&
      ballY_ <= static_cast<float>(paddleY + 1) &&
      ballX_ >= paddleX_ &&
      ballX_ <= (paddleX_ + static_cast<float>(PADDLE_WIDTH))) {
    ballY_ = static_cast<float>(paddleY - 2);
    ballVY_ = -ballVY_;
  }

  const int ballXi = static_cast<int>(ballX_);
  const int ballYi = static_cast<int>(ballY_);

  bool brickHit = false;
  for (uint8_t row = 0; row < BRICK_ROWS && !brickHit; row++) {
    for (uint8_t col = 0; col < BRICK_COLS; col++) {
      if (!bricks_[row][col]) {
        continue;
      }
      const int bx = 2 + col * BRICK_WIDTH;
      const int by = 2 + row * BRICK_HEIGHT;
      if (ballXi >= bx && ballXi < (bx + BRICK_WIDTH - 1) && ballYi >= by && ballYi < (by + BRICK_HEIGHT - 1)) {
        bricks_[row][col] = false;
        bricksLeft_--;
        score_++;
        if (score_ > bestScore_) {
          bestScore_ = score_;
          saveBestScore();
        }
        brickHit = true;

        const float brickLeft = static_cast<float>(bx);
        const float brickRight = static_cast<float>(bx + BRICK_WIDTH - 1);
        const float brickTop = static_cast<float>(by);
        const float brickBottom = static_cast<float>(by + BRICK_HEIGHT - 1);

        const bool hitFromLeft = prevBallX < brickLeft && ballX_ >= brickLeft;
        const bool hitFromRight = prevBallX > brickRight && ballX_ <= brickRight;
        const bool hitFromTop = prevBallY < brickTop && ballY_ >= brickTop;
        const bool hitFromBottom = prevBallY > brickBottom && ballY_ <= brickBottom;

        if ((hitFromLeft || hitFromRight) && !(hitFromTop || hitFromBottom)) {
          ballVX_ = -ballVX_;
        } else {
          ballVY_ = -ballVY_;
        }
        break;
      }
    }
  }

  if (bricksLeft_ == 0) {
    level_++;
    loadLevel();
    return;
  }

  if (ballY_ >= static_cast<float>(height - 1)) {
    if (score_ > bestScore_) {
      bestScore_ = score_;
      saveBestScore();
    }
    endApp();
  }
}

void BreakoutGame::drawRunning(U8G2& u8g2) {
  u8g2.drawFrame(left_, 0, width, height);
  for (uint8_t row = 0; row < BRICK_ROWS; row++) {
    for (uint8_t col = 0; col < BRICK_COLS; col++) {
      if (bricks_[row][col]) {
        u8g2.drawBox(left_ + 2 + col * BRICK_WIDTH, 2 + row * BRICK_HEIGHT, BRICK_WIDTH - 1, BRICK_HEIGHT - 1);
      }
    }
  }
  const int paddleX = clampInt(static_cast<int>(paddleX_), 1, static_cast<int>(width) - PADDLE_WIDTH - 1);
  const int ballX = clampInt(static_cast<int>(ballX_), 1, static_cast<int>(width) - 2);
  const int ballY = clampInt(static_cast<int>(ballY_), 1, static_cast<int>(height) - 2);
  u8g2.drawBox(left_ + paddleX, height - 4, PADDLE_WIDTH, 2);
  u8g2.drawBox(left_ + ballX, ballY, 2, 2);
  u8g2.setFont(u8g2_font_4x6_tr);
  u8g2.setCursor(left_ + 2, 6);
  u8g2.print("L");
  u8g2.print(level_);
  u8g2.print(" ");
  u8g2.print(score_);
}

void BreakoutGame::drawStart(U8G2& u8g2) {
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
    u8g2.drawStr(3, 10, "Top Bricks");
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
    u8g2.drawBox(6, 28, 20, 2);
    u8g2.drawDisc(44, 17, 2);
    u8g2.drawLine(12, 24, 30, 15);
    u8g2.drawLine(30, 15, 44, 17);
    u8g2.drawPixel(49, 15);
    u8g2.drawPixel(53, 13);
    u8g2.setFont(u8g2_font_5x8_tr);
    u8g2.drawStr(3, 9, appTitle());
  }
}

void BreakoutGame::drawEnd(U8G2& u8g2) {
  u8g2.drawFrame(0, 0, width + 2, height);
  u8g2.setFont(u8g2_font_5x8_tr);
  u8g2.drawStr(3, 9, "Ball Lost");
  u8g2.setFont(u8g2_font_4x6_tr);
  u8g2.setCursor(3, 20);
  u8g2.print("Bricks ");
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

void BreakoutGame::loadBestScore() {
  if (bestLoaded_) {
    return;
  }
  breakoutPrefs.begin("breakout", true);
  bestScore_ = breakoutPrefs.getUShort("best", 0);
  bestInitials_ = breakoutPrefs.getUShort("init", PlayerProfile::defaultInitials());
  breakoutPrefs.end();
  bestLoaded_ = true;
}

void BreakoutGame::saveBestScore() {
  bestInitials_ = PlayerProfile::loadInitials();
  breakoutPrefs.begin("breakout", false);
  breakoutPrefs.putUShort("best", bestScore_);
  breakoutPrefs.putUShort("init", bestInitials_);
  breakoutPrefs.end();
}
