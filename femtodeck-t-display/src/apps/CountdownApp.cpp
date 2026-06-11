#include "CountdownApp.h"
#include <TFT_eSPI.h>

CountdownApp::CountdownApp(uint32_t width, uint32_t height)
    : App("Countdown", width, height) {}

void CountdownApp::onAppReset() {
  logic_.reset();
}

void CountdownApp::updateRunning(uint32_t deltaMs, const ButtonInput& b1, const ButtonInput& b2) {
  if (b1.click) {
      if (logic_.isRunning()) logic_.toggle(millis());
      else logic_.adjustDuration(10000); // Add 10s if not running
  }
  if (b1.longPress) {
      if (logic_.isRunning()) logic_.reset();
      else logic_.toggle(millis());
  }
}

void CountdownApp::drawRunning(TFT_eSPI& tft) {
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(TFT_WHITE);
  tft.drawString("Countdown", 10, 10);

  uint32_t remaining = logic_.getRemainingMs(millis());
  uint32_t s = (remaining / 1000) % 60;
  uint32_t m = (remaining / 60000) % 60;
  uint32_t h = (remaining / 3600000);

  char buf[32];
  snprintf(buf, sizeof(buf), "%02u:%02u:%02u", (unsigned)h, (unsigned)m, (unsigned)s);

  tft.setTextSize(4);
  if (remaining == 0) tft.setTextColor(TFT_RED);
  else if (logic_.isRunning()) tft.setTextColor(TFT_GREEN);
  else tft.setTextColor(TFT_YELLOW);

  tft.drawString(buf, (240 - tft.textWidth(buf))/2, 50);

  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE);
  if (logic_.isRunning()) tft.drawString("Tap: Stop, Hold: Reset", 10, 110);
  else tft.drawString("Tap: +10s, Hold: Start", 10, 110);
}

void CountdownApp::drawStart(TFT_eSPI& tft) {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_RED);
  tft.setTextSize(3);
  tft.drawString("Countdown", 40, 40);
}

bool CountdownApp::hasCustomOverlay() const { return true; }
bool CountdownApp::startsRunningImmediately() const { return true; }
