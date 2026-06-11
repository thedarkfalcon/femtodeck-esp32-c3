#include "SimonGame.h"

#include <Arduino.h>
#include <Preferences.h>
#include <TFT_eSPI.h>

#include "../../PlayerProfile.h"

namespace {
Preferences simonPrefs;
constexpr uint16_t SIMON_HOLD_MS = 360;
}

SimonGame::SimonGame(uint32_t width, uint32_t height)
    : App("Simon", width, height) {}

bool SimonGame::hasCustomOverlay() const {
  return true;
}

void SimonGame::onAppReset() {
  loadBest();
  length_ = 0;
  showIndex_ = 0;
  inputIndex_ = 0;
  timerMs_ = 0;
  mode_ = Mode::Show;
  flashLong_ = false;
  expectedLong_ = false;
  pressHandled_ = false;
  addStep();
}

void SimonGame::addStep() {
  if (length_ < 32) {
    sequence_[length_] = length_ == 0 ? false : random(0, 2) == 1;
    length_++;
  }
  showIndex_ = 0;
  inputIndex_ = 0;
  timerMs_ = 0;
  mode_ = Mode::Show;
  pressHandled_ = false;
}

void SimonGame::updateRunning(uint32_t deltaMs, const ButtonInput& b1, const ButtonInput& b2) {
  timerMs_ += deltaMs;
  if (mode_ == Mode::Show) {
    const uint16_t stepMs = 650;
    if (timerMs_ >= stepMs) {
      timerMs_ = 0;
      showIndex_++;
      if (showIndex_ >= length_) {
        mode_ = Mode::Input;
        inputIndex_ = 0;
        timerMs_ = 0;
        pressHandled_ = false;
      }
    }
    return;
  }

  if (mode_ == Mode::Good) {
    if (timerMs_ > 650) {
      addStep();
    }
    return;
  }

  if (timerMs_ < 260) {
    return;
  }

  if (b1.down && !pressHandled_ && b1.holdMs >= SIMON_HOLD_MS) {
    pressHandled_ = true;
    checkStep(true);
    return;
  }

  if (b1.released) {
    if (!pressHandled_) {
      checkStep(false);
    }
    pressHandled_ = false;
  }
}

void SimonGame::checkStep(bool longTone) {
  flashLong_ = longTone;
  expectedLong_ = sequence_[inputIndex_];
  if (sequence_[inputIndex_] != longTone) {
    endApp();
    return;
  }
  inputIndex_++;
  if (inputIndex_ >= length_) {
    if (length_ > bestLevel_) {
      bestLevel_ = length_;
      saveBest();
    }
    mode_ = Mode::Good;
    timerMs_ = 0;
    pressHandled_ = false;
  }
}

void void SimonGame::drawRunning(TFT_eSPI& tft) { tft.fillScreen(TFT_BLACK); { tft.fillScreen(TFT_BLACK);
  tft.drawRect(, 0, width, height);

  tft.setCursor(3, 7);
  tft.print("L");
  tft.print(length_);

  if (mode_ == Mode::Show) {
    const bool longTone = sequence_[showIndex_];
    if ((timerMs_ % 650) < (longTone ? 420 : 180)) {
      const char* cue = longTone ? "HOLD" : "TAP";

      tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.drawString(((width - tft.textWidth(cue)) / 2), 25, cue);
    }

    tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.drawString(23, 38, "Watch");
    return;
  }

  if (mode_ == Mode::Good) {

    tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.drawString(20, 25, "GOOD");
    return;
  }


  tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.drawString(18, 19, "Repeat");

  tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.drawString(5, 30, "Tap=short");
  tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.drawString(38, 30, "Hold");
  tft.setCursor(25, 38);
  tft.print(inputIndex_);
  tft.print("/");
  tft.print(length_);
}

void SimonGame::drawStart(TFT_eSPI& tft) { tft.fillScreen(TFT_BLACK);
  loadBest();
  tft.drawRect(0, 0, width + 2, height);
  if (showStartPromptPage()) {

    tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.drawString(20, 16, "Press");
    tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.drawString(13, 29, "to Start");
  } else if (showStartScorePage()) {
    char initials[4];
    PlayerProfile::unpackDottedInitials(bestInitials_, initials);

    tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.drawString(3, 10, "Best Memory");

    tft.setCursor(3, 24);
    if (bestLevel_ == 0) tft.print("--");
    else {
      tft.print(initials);
      tft.print(" L");
      tft.print(bestLevel_);
    }
  } else {
    tft.drawRect(42, 12, 12, 12);
    tft.drawRect(56, 12, 12, 12);
    tft.drawRect(42, 26, 12, 12);
    tft.drawRect(56, 26, 12, 12);
    tft.fillRect(56, 12, 12, 12);

    tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.drawString(3, 10, "Simon");

    tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.drawString(3, 22, "Copy flashes");
    tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.drawString(3, 31, "Tap=short");
    tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.drawString(3, 38, "Hold=long");
  }
}

void SimonGame::drawEnd(TFT_eSPI& tft) { tft.fillScreen(TFT_BLACK);
  tft.drawRect(0, 0, width + 2, height);

  tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.drawString(3, 9, "Wrong");

  tft.setCursor(3, 20);
  tft.print("Needed ");
  tft.print(expectedLong_ ? "HOLD" : "TAP");
  tft.setCursor(3, 29);
  tft.print("Best ");
  tft.print(bestLevel_);
  tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.drawString(3, 38, "Tap retry Hold menu");
}

void SimonGame::loadBest() {
  if (bestLoaded_) return;
  simonPrefs.begin("simon", true);
  bestLevel_ = simonPrefs.getUChar("best", 0);
  bestInitials_ = simonPrefs.getUShort("init", PlayerProfile::defaultInitials());
  simonPrefs.end();
  bestLoaded_ = true;
}

void SimonGame::saveBest() {
  bestInitials_ = PlayerProfile::loadInitials();
  simonPrefs.begin("simon", false);
  simonPrefs.putUChar("best", bestLevel_);
  simonPrefs.putUShort("init", bestInitials_);
  simonPrefs.end();
}
