#include "MetronomeApp.h"
#include "../../TDisplayUi.h"
#include <TFT_eSPI.h>

MetronomeApp::MetronomeApp(uint32_t width, uint32_t height)
    : App("Metronome", width, height) {}

void MetronomeApp::onAppReset() {
  logic_.reset();
  pulseUntilMs_ = 0;
  uiInitialized_ = false;
  lastRunning_ = false;
  lastPulse_ = false;
  lastBpm_ = 0;
}

void MetronomeApp::updateRunning(uint32_t deltaMs, const ButtonInput& b1, const ButtonInput& b2) {
  (void)deltaMs;
  if (b1.click) {
      if (logic_.isRunning()) logic_.toggle();
      else logic_.setBpm(logic_.getBpm() + 5);
      if (logic_.getBpm() > 300) logic_.setBpm(20);
      uiInitialized_ = false;
  }
  if (b2.click && !logic_.isRunning()) {
      logic_.setBpm(logic_.getBpm() <= 20 ? 300 : logic_.getBpm() - 5);
      uiInitialized_ = false;
  }
  if (b1.longPress) {
      logic_.toggle();
      uiInitialized_ = false;
  }

  if (logic_.update(millis())) {
      pulseUntilMs_ = millis() + 140;
  }
}

void MetronomeApp::drawStatic(TFT_eSPI& tft, bool running) {
  TDisplayUi::clear(tft);
  TDisplayUi::header(tft, "Metronome", running ? TFT_GREEN : TFT_YELLOW, running ? "RUN" : "SET");
  if (running) TDisplayUi::footer(tft, "B1 stop / B1 hold stop");
  else TDisplayUi::footer(tft, "B1 +5 / B2 -5 / B1 hold start");
  uiInitialized_ = true;
  lastRunning_ = running;
  lastPulse_ = false;
  lastBpm_ = 0;
}

void MetronomeApp::drawBpmArea(TFT_eSPI& tft, bool pulse) {
  const bool running = logic_.isRunning();
  const uint16_t bg = pulse ? TFT_GREEN : TFT_BLACK;
  const uint16_t valueColor = pulse ? TFT_BLACK : (running ? TFT_GREEN : TFT_YELLOW);
  const uint16_t labelColor = pulse ? TFT_BLACK : TFT_LIGHTGREY;

  tft.fillRect(0, 29, width, 88, bg);
  if (!pulse) {
    tft.drawRoundRect(8, 35, width - 16, 72, 8, running ? TFT_DARKGREEN : TFT_DARKGREY);
  }

  String bpm = String(logic_.getBpm());
  tft.setTextDatum(TL_DATUM);
  tft.setTextSize(6);
  if (tft.textWidth(bpm) > width - 70) {
    tft.setTextSize(5);
  }
  tft.setTextColor(valueColor, bg);
  tft.drawString(bpm, (width - tft.textWidth(bpm)) / 2, 45);

  tft.setTextSize(2);
  tft.setTextColor(labelColor, bg);
  tft.drawString("BPM", (width - tft.textWidth("BPM")) / 2, 95);

  if (pulse) {
    tft.fillRect(0, 29, 8, 88, TFT_WHITE);
    tft.fillRect(width - 8, 29, 8, 88, TFT_WHITE);
  }
}

void MetronomeApp::drawRunning(TFT_eSPI& tft) {
  const bool running = logic_.isRunning();
  const bool pulse = running && millis() < pulseUntilMs_;
  if (!uiInitialized_ || running != lastRunning_) {
    drawStatic(tft, running);
  }
  if (pulse != lastPulse_ || logic_.getBpm() != lastBpm_) {
    drawBpmArea(tft, pulse);
    lastPulse_ = pulse;
    lastBpm_ = logic_.getBpm();
  }
}

void MetronomeApp::drawStart(TFT_eSPI& tft) {
  TDisplayUi::clear(tft);
  TDisplayUi::header(tft, "Metronome", TFT_BLUE);
  TDisplayUi::centered(tft, "120", 50, 5, TFT_YELLOW);
  TDisplayUi::centered(tft, "BPM", 99, 1, TFT_LIGHTGREY);
  TDisplayUi::footer(tft, "B1 start");
}

bool MetronomeApp::hasCustomOverlay() const { return true; }
bool MetronomeApp::startsRunningImmediately() const { return true; }
