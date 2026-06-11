#include "TowerStackerGame.h"

#include <Arduino.h>
#include <Preferences.h>
#include <TFT_eSPI.h>

#include "../../PlayerProfile.h"

namespace {
Preferences towerScorePrefs;
}

TowerStackerGame::TowerStackerGame(uint32_t width, uint32_t height)
    : App("Tower Stacker", width, height) {}

bool TowerStackerGame::hasCustomOverlay() const {
  return true;
}

void TowerStackerGame::onAppReset() {
  loadHighScore();
  score_ = 0;
  level_ = 1;
  startTowerLevel();
}

void TowerStackerGame::updateRunning(uint32_t deltaMs, const ButtonInput& b1, const ButtonInput& b2) {
  const float deltaSec = static_cast<float>(deltaMs) * 0.001f;
  movingX_ += static_cast<float>(movingDir_) * movingSpeed_ * deltaSec;

  const float minX = 1.0f;
  const float maxX = static_cast<float>(width - movingW_ - 1);
  if (movingX_ <= minX) {
    movingX_ = minX;
    movingDir_ = 1;
  } else if (movingX_ >= maxX) {
    movingX_ = maxX;
    movingDir_ = -1;
  }

  if (b1.click) {
    dropMovingBlock();
  }
}

void void TowerStackerGame::drawRunning(TFT_eSPI& tft) { tft.fillScreen(TFT_BLACK); { tft.fillScreen(TFT_BLACK);
  tft.drawRect(, 0, width, height);

  tft.setCursor(2, 6);
  tft.print("L");
  tft.print(level_);
  tft.print(" ");
  tft.print(score_);

  const uint8_t visibleLayers = (height - HUD_H - 2) / BLOCK_H;
  uint8_t firstVisibleLayer = 0;
  if (layerCount_ >= visibleLayers) {
    firstVisibleLayer = layerCount_ - visibleLayers + 1;
  }

  for (uint8_t i = firstVisibleLayer; i < layerCount_; i++) {
    tft.fillRect(layers_[i].x, layerY(i, firstVisibleLayer), layers_[i].w, BLOCK_H);
  }

  if (layerCount_ < MAX_LAYERS) {
    tft.drawRect(static_cast<int>(movingX_), layerY(layerCount_, firstVisibleLayer), movingW_, BLOCK_H);
  }
}

void TowerStackerGame::drawStart(TFT_eSPI& tft) { tft.fillScreen(TFT_BLACK);
  loadHighScore();
  tft.drawRect(0, 0, width + 2, height);
  if (showStartPromptPage()) {

    tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.drawString(20, 16, "Press");
    tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.drawString(13, 29, "to Start");
  } else if (showStartScorePage()) {
    char initials[4];
    PlayerProfile::unpackDottedInitials(highScoreInitials_, initials);

    tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.drawString(3, 10, "Top Score");

    tft.setCursor(3, 24);
    if (highScore_ == 0) {
      tft.print("--");
    } else {
      tft.print(initials);
      tft.print(" ");
      tft.print(highScore_);
    }
  } else {
    tft.fillRect(33, 12, 9, 22);
    tft.fillRect(29, 18, 17, 16);
    tft.fillRect(25, 24, 25, 10);
    tft.drawLine(18, 34, 56, 34);

    tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.drawString(3, 10, appTitle());
  }
}

void TowerStackerGame::drawEnd(TFT_eSPI& tft) { tft.fillScreen(TFT_BLACK);
  tft.drawRect(0, 0, width + 2, height);

  tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.drawString(3, 9, "Game Over");
  tft.setCursor(3, 19);
  tft.print("Score:");
  tft.print(score_);
  tft.setCursor(3, 29);
  tft.print("Best:");
  tft.print(highScore_);
  if (highScore_ > 0) {
    char initials[4];
    PlayerProfile::unpackDottedInitials(highScoreInitials_, initials);
    tft.print(" ");
    tft.print(initials);
  }

  tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.drawString(3, 38, "Tap retry Hold menu");
}

void TowerStackerGame::loadHighScore() {
  if (highScoreLoaded_) {
    return;
  }
  towerScorePrefs.begin("tower", true);
  highScore_ = towerScorePrefs.getUShort("best", 0);
  highScoreInitials_ = towerScorePrefs.getUShort("init", PlayerProfile::defaultInitials());
  towerScorePrefs.end();
  highScoreLoaded_ = true;
}

void TowerStackerGame::saveHighScore() {
  highScoreInitials_ = PlayerProfile::loadInitials();
  towerScorePrefs.begin("tower", false);
  towerScorePrefs.putUShort("best", highScore_);
  towerScorePrefs.putUShort("init", highScoreInitials_);
  towerScorePrefs.end();
}

void TowerStackerGame::startTowerLevel() {
  const int baseW = 36;
  layerCount_ = 1;
  towerHeight_ = 0;
  layers_[0].x = (static_cast<int>(width) - baseW) / 2;
  layers_[0].w = baseW;
  movingDir_ = 1;
  movingSpeed_ = 15.0f + static_cast<float>(level_ - 1) * 3.0f;
  prepareNextBlock(baseW);
}

void TowerStackerGame::dropMovingBlock() {
  const Layer& previous = layers_[layerCount_ - 1];
  const int droppedX = static_cast<int>(movingX_ + 0.5f);
  const int leftEdge = max(droppedX, previous.x);
  const int rightEdge = min(droppedX + movingW_, previous.x + previous.w);
  const int overlapW = rightEdge - leftEdge;

  if (overlapW <= 0) {
    endApp();
    return;
  }

  if (layerCount_ >= MAX_LAYERS) {
    level_++;
    startTowerLevel();
    return;
  }

  int nextX = leftEdge;
  int nextW = overlapW;
  const int miss = previous.w - overlapW;
  if (miss <= 2) {
    nextX = previous.x;
    nextW = previous.w;
  }
  layers_[layerCount_].x = nextX;
  layers_[layerCount_].w = nextW;
  layerCount_++;
  towerHeight_++;
  score_++;
  if (score_ > highScore_) {
    highScore_ = score_;
    saveHighScore();
  }

  if (towerHeight_ >= LEVEL_HEIGHT) {
    level_++;
    startTowerLevel();
  } else {
    prepareNextBlock(nextW);
  }
}

void TowerStackerGame::prepareNextBlock(int blockWidth) {
  movingW_ = blockWidth;
  movingX_ = 1.0f;
  movingDir_ = 1;
  movingSpeed_ = min(42.0f, 15.0f + static_cast<float>(level_ - 1) * 3.0f + static_cast<float>(towerHeight_) * 0.35f);
}

int TowerStackerGame::layerY(uint8_t layer, uint8_t firstVisibleLayer) const {
  const uint8_t visibleIndex = layer - firstVisibleLayer;
  return static_cast<int>(height) - 2 - ((visibleIndex + 1) * BLOCK_H);
}
