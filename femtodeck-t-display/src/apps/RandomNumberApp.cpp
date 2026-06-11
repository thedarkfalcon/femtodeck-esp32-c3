#include "RandomNumberApp.h"
#include <TFT_eSPI.h>

RandomNumberApp::RandomNumberApp(uint32_t width, uint32_t height)
    : App("Random Number", width, height) {}

void RandomNumberApp::onAppReset() {
  logic_.reset();
}

void RandomNumberApp::updateRunning(uint32_t deltaMs, const ButtonInput& b1, const ButtonInput& b2) {
  logic_.update(deltaMs, b1.click, b1.longPress);
}

void RandomNumberApp::drawRunning(TFT_eSPI& tft) {
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(2);

  if (logic_.isEditing()) {
    tft.setTextColor(TFT_WHITE);
    tft.drawString("Set Range:", 10, 10);

    tft.setTextSize(3);
    if (logic_.isEditingMin()) tft.setTextColor(TFT_YELLOW);
    else tft.setTextColor(TFT_WHITE);
    tft.drawString("Min: " + String(logic_.getMin()), 10, 40);

    if (!logic_.isEditingMin()) tft.setTextColor(TFT_YELLOW);
    else tft.setTextColor(TFT_WHITE);
    tft.drawString("Max: " + String(logic_.getMax()), 10, 80);

    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE);
    tft.drawString("Tap: +1, Hold: Next/Roll", 10, 120);
  } else {
    tft.setTextColor(TFT_WHITE);
    tft.drawString("Range: " + String(logic_.getMin()) + " - " + String(logic_.getMax()), 10, 10);

    tft.setTextSize(6);
    tft.setTextColor(TFT_GREEN);
    String res = String(logic_.getResult());
    tft.drawString(res, (240 - tft.textWidth(res))/2, 45);

    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE);
    tft.drawString("Tap: Roll again, Hold: Edit", 10, 120);
  }
}

void RandomNumberApp::drawStart(TFT_eSPI& tft) {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_MAGENTA);
  tft.setTextSize(3);
  tft.drawString("Randomizer", 30, 40);
}

bool RandomNumberApp::hasCustomOverlay() const { return true; }
bool RandomNumberApp::startsRunningImmediately() const { return true; }
