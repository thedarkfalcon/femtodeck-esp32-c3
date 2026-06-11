#include "DiceRollerApp.h"
#include <TFT_eSPI.h>

DiceRollerApp::DiceRollerApp(uint32_t width, uint32_t height)
    : App("Dice Roller", width, height) {}

void DiceRollerApp::onAppReset() {
  logic_.reset();
}

void DiceRollerApp::updateRunning(uint32_t deltaMs, const ButtonInput& b1, const ButtonInput& b2) {
  logic_.update(deltaMs, b1.click, b1.longPress);
}

void DiceRollerApp::drawRunning(TFT_eSPI& tft) {
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(TFT_WHITE);

  if (logic_.getMode() == DiceRollerLogic::Mode::Select) {
    tft.drawString("Select Die:", 10, 10);
    tft.setTextSize(4);
    tft.setTextColor(TFT_CYAN);
    String die = "d" + String(logic_.getSelectedDice());
    tft.drawString(die, (240 - tft.textWidth(die)) / 2, 50);
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE);
    tft.drawString("Tap: Next, Hold: Roll", 10, 110);
  } else {
    String die = "d" + String(logic_.getSelectedDice());
    tft.drawString(die, 10, 10);

    if (logic_.isRolling()) {
      tft.setTextSize(4);
      tft.setTextColor(TFT_YELLOW);
      String val = String(random(1, logic_.getSelectedDice() + 1));
      tft.drawString(val, (240 - tft.textWidth(val)) / 2, 50);
    } else {
      tft.setTextSize(5);
      tft.setTextColor(TFT_GREEN);
      String val = String(logic_.getResult());
      tft.drawString(val, (240 - tft.textWidth(val)) / 2, 45);
      tft.setTextSize(1);
      tft.setTextColor(TFT_WHITE);
      tft.drawString("Tap/Hold: Back", 10, 110);
    }
  }
}

void DiceRollerApp::drawStart(TFT_eSPI& tft) {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_CYAN);
  tft.setTextSize(3);
  tft.drawString("Dice Roller", 40, 40);
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE);
  tft.drawString("Press Button 1 to Start", 60, 90);
}

bool DiceRollerApp::hasCustomOverlay() const { return true; }

bool DiceRollerApp::startsRunningImmediately() const { return true; }
