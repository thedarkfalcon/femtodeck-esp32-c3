#include "CounterApp.h"
#include <TFT_eSPI.h>

CounterApp::CounterApp(uint32_t width, uint32_t height)
    : App("Counter", width, height) {}

void CounterApp::onAppReset() {
  logic_.reset();
}

void CounterApp::updateRunning(uint32_t deltaMs, const ButtonInput& b1, const ButtonInput& b2) {
  if (b1.click) logic_.increment();
  if (b1.longPress) logic_.reset();
}

void CounterApp::drawRunning(TFT_eSPI& tft) {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setTextSize(4);

  String val = String(logic_.getCount());
  int x = (240 - tft.textWidth(val)) / 2;
  tft.drawString(val, x, 50);

  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE);
  tft.drawString("Tap = +1", 10, 110);
  tft.drawString("Hold = Reset", 150, 110);
}

void CounterApp::drawStart(TFT_eSPI& tft) {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_GREEN);
  tft.setTextSize(3);
  tft.drawString("Counter", 60, 40);
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE);
  tft.drawString("Press Button 1 to Start", 60, 90);
}

bool CounterApp::hasCustomOverlay() const { return true; }
