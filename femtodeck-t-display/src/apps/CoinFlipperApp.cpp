#include "CoinFlipperApp.h"
#include "../../TDisplayUi.h"
#include <TFT_eSPI.h>

namespace {
constexpr int COIN_CX = 120;
constexpr int COIN_CY = 67;
constexpr int COIN_R = 31;

void drawCoinBase(TFT_eSPI& tft, uint16_t rim, uint16_t fill) {
  tft.drawCircle(COIN_CX, COIN_CY, COIN_R, rim);
  tft.drawCircle(COIN_CX, COIN_CY, COIN_R - 2, rim);
  tft.fillCircle(COIN_CX, COIN_CY, COIN_R - 5, fill);
}

void drawHeadCoin(TFT_eSPI& tft) {
  drawCoinBase(tft, TFT_GOLD, TFT_DARKGREY);
  tft.fillCircle(COIN_CX, COIN_CY - 7, 8, TFT_GOLD);
  tft.drawFastHLine(COIN_CX - 16, COIN_CY + 14, 32, TFT_GOLD);
  tft.drawFastHLine(COIN_CX - 10, COIN_CY + 10, 20, TFT_GOLD);
  tft.drawLine(COIN_CX - 9, COIN_CY - 19, COIN_CX - 5, COIN_CY - 26, TFT_GOLD);
  tft.drawLine(COIN_CX, COIN_CY - 18, COIN_CX, COIN_CY - 28, TFT_GOLD);
  tft.drawLine(COIN_CX + 9, COIN_CY - 19, COIN_CX + 5, COIN_CY - 26, TFT_GOLD);
  tft.drawFastHLine(COIN_CX - 10, COIN_CY - 18, 20, TFT_GOLD);
}

void drawTailCoin(TFT_eSPI& tft) {
  drawCoinBase(tft, TFT_SILVER, TFT_DARKGREY);
  tft.setTextDatum(TL_DATUM);
  tft.setTextSize(3);
  tft.setTextColor(TFT_SILVER, TFT_DARKGREY);
  tft.drawString("$1", COIN_CX - 18, COIN_CY - 14);
}
}

CoinFlipperApp::CoinFlipperApp(uint32_t width, uint32_t height)
    : App("Coin Flipper", width, height) {}

void CoinFlipperApp::onAppReset() {
  logic_.reset();
  dirty_ = true;
  renderedFrame_ = UINT16_MAX;
}

void CoinFlipperApp::updateRunning(uint32_t deltaMs, const ButtonInput& b1, const ButtonInput& b2) {
  const auto oldMode = logic_.getMode();
  const bool oldHeads = logic_.isHeads();
  logic_.update(deltaMs, b1.click, b1.longPress);
  if (oldMode != logic_.getMode() || oldHeads != logic_.isHeads()) {
    dirty_ = true;
  }
}

void CoinFlipperApp::drawRunning(TFT_eSPI& tft) {
  if (logic_.getMode() == CoinFlipperLogic::Mode::Ready) {
    if (!dirty_ && renderedMode_ == logic_.getMode()) {
      return;
    }
    TDisplayUi::clear(tft);
    TDisplayUi::header(tft, "Coin Flipper", TFT_GOLD, "READY");
    drawTailCoin(tft);
    TDisplayUi::footer(tft, "B1 flip");
  } else if (logic_.getMode() == CoinFlipperLogic::Mode::Flipping) {
    const uint16_t frame = (logic_.getAnimMs() / 110) % 4;
    if (!dirty_ && renderedFrame_ == frame) {
      return;
    }
    if (dirty_ || renderedMode_ != logic_.getMode()) {
      TDisplayUi::clear(tft);
      TDisplayUi::header(tft, "Coin Flipper", TFT_YELLOW, "FLIP");
      TDisplayUi::footer(tft, "Flipping...");
    }
    tft.fillRect(COIN_CX - 42, COIN_CY - 36, 84, 72, TFT_BLACK);
    const int coinW = frame == 0 ? 58 : (frame == 1 || frame == 3 ? 18 : 36);
    tft.drawEllipse(COIN_CX, COIN_CY, coinW / 2, 30, TFT_YELLOW);
    tft.fillEllipse(COIN_CX, COIN_CY, max(2, coinW / 2 - 4), 25, frame == 2 ? TFT_DARKGREY : TFT_GOLD);
    renderedFrame_ = frame;
  } else {
    if (!dirty_ && renderedMode_ == logic_.getMode() && renderedHeads_ == logic_.isHeads()) {
      return;
    }
    TDisplayUi::clear(tft);
    TDisplayUi::header(tft, "Coin Flipper", logic_.isHeads() ? TFT_GOLD : TFT_SILVER, "RESULT");
    tft.setTextDatum(TL_DATUM);
    if (logic_.isHeads()) {
      drawHeadCoin(tft);
      tft.setTextSize(2);
      tft.setTextColor(TFT_GOLD, TFT_BLACK);
      tft.drawString("HEADS", (width - tft.textWidth("HEADS")) / 2, 101);
    } else {
      drawTailCoin(tft);
      tft.setTextSize(2);
      tft.setTextColor(TFT_SILVER, TFT_BLACK);
      tft.drawString("TAILS", (width - tft.textWidth("TAILS")) / 2, 101);
    }
    TDisplayUi::footer(tft, "B1 flip again / B2 hold menu");
  }
  renderedMode_ = logic_.getMode();
  renderedHeads_ = logic_.isHeads();
  dirty_ = false;
}

void CoinFlipperApp::drawStart(TFT_eSPI& tft) {
  TDisplayUi::clear(tft);
  TDisplayUi::header(tft, "Coin Flipper", TFT_GOLD);
  drawHeadCoin(tft);
  TDisplayUi::centered(tft, "Heads or tails", 101, 1, TFT_LIGHTGREY);
  TDisplayUi::footer(tft, "B1 start");
}

bool CoinFlipperApp::hasCustomOverlay() const { return true; }
bool CoinFlipperApp::startsRunningImmediately() const { return true; }
