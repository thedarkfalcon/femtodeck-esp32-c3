#include "StopwatchApp.h"
#include <TFT_eSPI.h>

StopwatchApp::StopwatchApp(uint32_t width, uint32_t height)
    : App("Stopwatch", width, height) {}

void StopwatchApp::onAppReset() {
  logic_.reset();
}

void StopwatchApp::updateRunning(uint32_t deltaMs, const ButtonInput& b1, const ButtonInput& b2) {
  if (b1.click) logic_.toggle(millis());
  if (b1.longPress) logic_.reset();
}

void StopwatchApp::drawRunning(TFT_eSPI& tft) {
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(TFT_WHITE);
  tft.drawString("Stopwatch", 10, 10);

  uint32_t elapsed = logic_.getElapsedMs(millis());
  uint32_t ms = elapsed % 1000;
  uint32_t s = (elapsed / 1000) % 60;
  uint32_t m = (elapsed / 60000) % 60;
  uint32_t h = (elapsed / 3600000);

  char buf[32];
  snprintf(buf, sizeof(buf), "%02u:%02u:%02u.%03u", (unsigned)h, (unsigned)m, (unsigned)s, (unsigned)ms);

  tft.setTextSize(3);
  tft.setTextColor(logic_.isRunning() ? TFT_GREEN : TFT_YELLOW);
  tft.drawString(buf, (240 - tft.textWidth(buf))/2, 50);

  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE);
  tft.drawString("Tap: Start/Stop, Hold: Reset", 10, 110);
}

void StopwatchApp::drawStart(TFT_eSPI& tft) {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_GREEN);
  tft.setTextSize(3);
  tft.drawString("Stopwatch", 40, 40);
}

bool StopwatchApp::hasCustomOverlay() const { return true; }
bool StopwatchApp::startsRunningImmediately() const { return true; }
