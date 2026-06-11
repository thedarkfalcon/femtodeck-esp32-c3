#include "CoinFlipperApp.h"
#include <TFT_eSPI.h>

CoinFlipperApp::CoinFlipperApp(uint32_t width, uint32_t height)
    : App("Coin Flipper", width, height) {}

void CoinFlipperApp::onAppReset() {
  logic_.reset();
}

void CoinFlipperApp::updateRunning(uint32_t deltaMs, const ButtonInput& b1, const ButtonInput& b2) {
  logic_.update(deltaMs, b1.click, b1.longPress);
}

void CoinFlipperApp::drawRunning(TFT_eSPI& tft) {
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(TFT_WHITE);

  if (logic_.getMode() == CoinFlipperLogic::Mode::Ready) {
    tft.drawString("Ready to flip!", 40, 50);
    tft.setTextSize(1);
    tft.drawString("Tap Button 1 to Flip", 60, 90);
  } else if (logic_.getMode() == CoinFlipperLogic::Mode::Flipping) {
    tft.setTextColor(TFT_YELLOW);
    tft.drawString("Flipping...", 60, 50);
    // Draw an animated circle?
    tft.drawCircle(120, 67, 30, TFT_YELLOW);
  } else {
    tft.setTextSize(4);
    if (logic_.isHeads()) {
        tft.setTextColor(TFT_GOLD);
        tft.drawString("HEADS", (240 - tft.textWidth("HEADS"))/2, 50);
    } else {
        tft.setTextColor(TFT_SILVER);
        tft.drawString("TAILS", (240 - tft.textWidth("TAILS"))/2, 50);
    }
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE);
    tft.drawString("Tap to flip again, Hold to menu", 40, 110);
  }
}

void CoinFlipperApp::drawStart(TFT_eSPI& tft) {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_YELLOW);
  tft.setTextSize(3);
  tft.drawString("Coin Flipper", 30, 40);
}

bool CoinFlipperApp::hasCustomOverlay() const { return true; }
bool CoinFlipperApp::startsRunningImmediately() const { return true; }
