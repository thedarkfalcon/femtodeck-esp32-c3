#include "MetronomeApp.h"
#include <TFT_eSPI.h>

MetronomeApp::MetronomeApp(uint32_t width, uint32_t height)
    : App("Metronome", width, height) {}

void MetronomeApp::onAppReset() {
  logic_.reset();
}

void MetronomeApp::updateRunning(uint32_t deltaMs, const ButtonInput& b1, const ButtonInput& b2) {
  if (b1.click) {
      if (logic_.isRunning()) logic_.toggle();
      else logic_.setBpm(logic_.getBpm() + 5);
      if (logic_.getBpm() > 300) logic_.setBpm(20);
  }
  if (b1.longPress) {
      logic_.toggle();
  }

  if (logic_.update(millis())) {
      // Visual feedback for tick
  }
}

void MetronomeApp::drawRunning(TFT_eSPI& tft) {
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(TFT_WHITE);
  tft.drawString("Metronome", 10, 10);

  tft.setTextSize(6);
  if (logic_.isRunning()) tft.setTextColor(TFT_GREEN);
  else tft.setTextColor(TFT_YELLOW);

  String bpm = String(logic_.getBpm());
  tft.drawString(bpm, (240 - tft.textWidth(bpm))/2, 45);

  tft.setTextSize(2);
  tft.drawString("BPM", 180, 70);

  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE);
  if (logic_.isRunning()) tft.drawString("Tap/Hold: Stop", 10, 110);
  else tft.drawString("Tap: +5 BPM, Hold: Start", 10, 110);
}

void MetronomeApp::drawStart(TFT_eSPI& tft) {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_BLUE);
  tft.setTextSize(3);
  tft.drawString("Metronome", 40, 40);
}

bool MetronomeApp::hasCustomOverlay() const { return true; }
bool MetronomeApp::startsRunningImmediately() const { return true; }
