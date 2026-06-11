#include "NoonShooterGame.h"

#include <Arduino.h>
#include <Preferences.h>
#include <TFT_eSPI.h>

#include "../../PlayerProfile.h"

namespace {
Preferences shooterPrefs;
}

NoonShooterGame::NoonShooterGame(uint32_t width, uint32_t height)
    : App("Noon Shooter", width, height) {}

bool NoonShooterGame::hasCustomOverlay() const {
  return true;
}

void NoonShooterGame::onAppReset() {
  loadScores();
  level_ = 1;
  tooEarly_ = false;
  playerLost_ = false;
  startRound();
}

void NoonShooterGame::startRound() {
  duelState_ = DuelState::Waiting;
  waitMs_ = static_cast<uint16_t>(random(1000, 3200));
  elapsedMs_ = 0;
  reactionMs_ = 0;
  animMs_ = 0;
}

void NoonShooterGame::updateRunning(uint32_t deltaMs, const ButtonInput& b1, const ButtonInput& b2) {
  elapsedMs_ += static_cast<uint16_t>(min<uint32_t>(deltaMs, 200));

  if (duelState_ == DuelState::PlayerShot || duelState_ == DuelState::EnemyShot) {
    animMs_ += deltaMs;
    if (animMs_ >= 1050) {
      if (duelState_ == DuelState::PlayerShot) {
        level_++;
        if (level_ > bestLevel_) {
          bestLevel_ = level_;
          saveBestLevel();
        }
        startRound();
      } else {
        endApp();
      }
    }
    return;
  }

  if (duelState_ == DuelState::Waiting) {
    if (b1.pressed) {
      tooEarly_ = true;
      playerLost_ = true;
      duelState_ = DuelState::EnemyShot;
      animMs_ = 0;
      return;
    }
    if (elapsedMs_ >= waitMs_) {
      duelState_ = DuelState::Draw;
      elapsedMs_ = waitMs_;
    }
    return;
  }

  if (b1.pressed) {
    reactionMs_ = elapsedMs_ - waitMs_;
    if (reactionMs_ < enemyDelayMs() && (bestReactionMs_ == 0 || reactionMs_ < bestReactionMs_)) {
      bestReactionMs_ = reactionMs_;
      saveBestReaction();
    }
    if (reactionMs_ < enemyDelayMs()) {
      duelState_ = DuelState::PlayerShot;
    } else {
      playerLost_ = true;
      duelState_ = DuelState::EnemyShot;
    }
    animMs_ = 0;
  } else if ((elapsedMs_ - waitMs_) >= enemyDelayMs()) {
    reactionMs_ = enemyDelayMs();
    playerLost_ = true;
    duelState_ = DuelState::EnemyShot;
    animMs_ = 0;
  }
}

void void NoonShooterGame::drawRunning(TFT_eSPI& tft) { tft.fillScreen(TFT_BLACK); { tft.fillScreen(TFT_BLACK);
  tft.drawRect(, 0, width, height);
  if (duelState_ == DuelState::PlayerShot || duelState_ == DuelState::EnemyShot) {
    const uint8_t frame = min<uint8_t>(4, animMs_ / 160);
    if (frame < 3) {
      drawRevolver(tft, duelState_ == DuelState::PlayerShot, frame);
      return;
    }
  }

  tft.drawLine(1, 31, width - 2, 31);
  if (duelState_ == DuelState::EnemyShot && animMs_ >= 480) {
    drawFallenGunslinger(tft, 12, 31, true);
  } else {
    drawGunslinger(tft, 12, 28, true);
  }
  if (duelState_ == DuelState::PlayerShot && animMs_ >= 480) {
    drawFallenGunslinger(tft, 56, 31, false);
  } else {
    drawGunslinger(tft, 56, 28, false);
  }

  if (duelState_ == DuelState::Waiting) {
    tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.drawString(23, 12, "WAIT");
  } else if (duelState_ == DuelState::Draw) {
    tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.drawString(22, 12, "DRAW!");
  } else {
    tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.drawString(24, 12, duelState_ == DuelState::PlayerShot ? "HIT" : "SHOT");
  }

  tft.setCursor(2, 38);
  tft.print("L");
  tft.print(level_);
  tft.print(" enemy ");
  tft.print(enemyDelayMs());
}

void NoonShooterGame::drawStart(TFT_eSPI& tft) { tft.fillScreen(TFT_BLACK);
  loadScores();
  tft.drawRect(0, 0, width + 2, height);
  if (showStartPromptPage()) {

    tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.drawString(20, 16, "Press");
    tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.drawString(13, 29, "to Start");
  } else if (showStartScorePage()) {
    char initials[4];
    PlayerProfile::unpackDottedInitials(bestInitials_, initials);

    tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.drawString(3, 10, "Top Duel");

    tft.setCursor(3, 23);
    if (bestLevel_ == 0) {
      tft.print("--");
    } else {
      tft.print(initials);
      tft.print(" L");
      tft.print(bestLevel_);
      tft.setCursor(3, 32);
      if (bestReactionMs_ == 0) {
        tft.print("--");
      } else {
        tft.print(bestReactionMs_);
        tft.print("ms");
      }
    }
  } else {
    tft.drawCircle(56, 8, 5);
    tft.drawLine(4, 31, 67, 31);
    drawGunslinger(tft, 18, 28, true);
    drawGunslinger(tft, 46, 28, false);

    tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.drawString(3, 10, appTitle());
  }
}

void NoonShooterGame::drawEnd(TFT_eSPI& tft) { tft.fillScreen(TFT_BLACK);
  tft.drawRect(0, 0, width + 2, height);

  tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.drawString(3, 9, tooEarly_ ? "Too Early" : "You Lost");

  tft.setCursor(3, 20);
  tft.print("React ");
  if (tooEarly_ || reactionMs_ == 0) {
    tft.print("--");
  } else {
    tft.print(reactionMs_);
    tft.print("ms");
  }
  tft.setCursor(3, 29);
  tft.print("Best L");
  tft.print(bestLevel_);
  tft.print(" ");
  if (bestLevel_ > 0) {
    char initials[4];
    PlayerProfile::unpackDottedInitials(bestInitials_, initials);
    tft.print(initials);
    tft.print(" ");
  }
  if (bestReactionMs_ == 0) {
    tft.print("--");
  } else {
    tft.print(bestReactionMs_);
    tft.print("ms");
  }
  tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.drawString(3, 38, "Tap retry Hold menu");
}

void NoonShooterGame::loadScores() {
  if (bestLoaded_) {
    return;
  }
  shooterPrefs.begin("shooter", true);
  bestReactionMs_ = shooterPrefs.getUShort("best", 0);
  bestLevel_ = shooterPrefs.getUChar("level", 0);
  bestInitials_ = shooterPrefs.getUShort("init", PlayerProfile::defaultInitials());
  shooterPrefs.end();
  bestLoaded_ = true;
}

void NoonShooterGame::saveBestReaction() {
  bestInitials_ = PlayerProfile::loadInitials();
  shooterPrefs.begin("shooter", false);
  shooterPrefs.putUShort("best", bestReactionMs_);
  shooterPrefs.putUShort("init", bestInitials_);
  shooterPrefs.end();
}

void NoonShooterGame::saveBestLevel() {
  bestInitials_ = PlayerProfile::loadInitials();
  shooterPrefs.begin("shooter", false);
  shooterPrefs.putUChar("level", bestLevel_);
  shooterPrefs.putUShort("init", bestInitials_);
  shooterPrefs.end();
}

uint16_t NoonShooterGame::enemyDelayMs() const {
  if (level_ <= 6) {
    return 900 - (level_ * 100);
  }
  return max<uint16_t>(230, 300 - ((level_ - 6) * 12));
}

void NoonShooterGame::drawRevolver(TFT_eSPI& tft, bool playerShot, uint8_t frame) {
  const int cx = 36;
  const int cy = 20;
  tft.drawCircle(cx, cy, 10);
  tft.drawCircle(cx, cy, 4);
  for (uint8_t i = 0; i < 6; i++) {
    const float angle = static_cast<float>(i) * 1.047f;
    tft.drawPixel(cx + static_cast<int>(cosf(angle) * 7), cy + static_cast<int>(sinf(angle) * 7));
  }
  tft.drawCircle(cx, cy, 2);
  if (frame >= 1) {
    const int flashX = playerShot ? 52 : 20;
    tft.drawLine(cx, cy, flashX, cy);
    tft.drawLine(flashX, cy - 3, flashX + (playerShot ? 5 : -5), cy);
    tft.drawLine(flashX, cy + 3, flashX + (playerShot ? 5 : -5), cy);
  }
  if (frame >= 2) {

    tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.drawString(28, 38, "BANG");
  }
}

void NoonShooterGame::drawGunslinger(TFT_eSPI& tft, int x, int y, bool facingRight) {
  tft.drawCircle(x, y - 12, 2);
  tft.drawLine(x, y - 9, x, y - 3);
  tft.drawLine(x, y - 3, x - 3, y);
  tft.drawLine(x, y - 3, x + 3, y);
  if (facingRight) {
    tft.drawLine(x, y - 8, x + 6, y - 8);
    tft.drawPixel(x + 7, y - 8);
  } else {
    tft.drawLine(x, y - 8, x - 6, y - 8);
    tft.drawPixel(x - 7, y - 8);
  }
}

void NoonShooterGame::drawFallenGunslinger(TFT_eSPI& tft, int x, int y, bool facingRight) {
  const int dir = facingRight ? 1 : -1;
  tft.drawCircle(x, y - 3, 2);
  tft.drawLine(x + dir * 2, y - 3, x + dir * 11, y - 2);
  tft.drawLine(x + dir * 5, y - 2, x + dir * 2, y);
  tft.drawLine(x + dir * 7, y - 2, x + dir * 11, y);
}
