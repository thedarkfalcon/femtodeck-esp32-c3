#include "StopwatchApp.h"

#include <U8g2lib.h>

StopwatchApp::StopwatchApp(uint32_t width, uint32_t height, uint32_t left)
    : App("Stopwatch", width, height), running_(false), elapsedMs_(0) {}

void StopwatchApp::onAppReset() {
  running_ = false;
  elapsedMs_ = 0;
}

void StopwatchApp::updateRunning(uint32_t deltaMs, const ButtonInput& input) {
  if (input.click) {
    running_ = !running_;
  }
  // Use long press to reset the stopwatch to zero (do not exit to menu)
  if (input.longPress) {
    running_ = false;
    elapsedMs_ = 0;
    return;
  }
  if (running_) {
    elapsedMs_ += deltaMs;
  }
}

void StopwatchApp::drawRunning(U8G2& u8g2) {
  u8g2.drawFrame(0, 0, width, height);

  uint32_t ms = elapsedMs_ % 1000;
  uint32_t totalSec = elapsedMs_ / 1000;
  uint32_t sec = totalSec % 60;
  uint32_t min = totalSec / 60;
  uint32_t cs = ms / 10; // centiseconds

  char buf[16];
  snprintf(buf, sizeof(buf), "%02u:%02u.%02u", (unsigned)min, (unsigned)sec, (unsigned)cs);

  // Larger time display
  u8g2.setFont(u8g2_font_7x13_tr);
  int textW = u8g2.getStrWidth(buf);
  int x = (int)((width + 2 - textW) / 2);
  u8g2.drawStr(x, 22, buf);

  // Hints: tap hint at top, hold hint at bottom
  u8g2.setFont(u8g2_font_4x6_tr);
  const char* tapText = running_ ? "Tap=pause" : "Tap=start";
  u8g2.drawStr(3, 9, tapText);
  const char* holdText = "Hold=Reset";
  int holdW = u8g2.getStrWidth(holdText);
  int holdX = (int)(width + 2 - holdW - 3);
  if (holdX < 3) holdX = 3;
  u8g2.drawStr(holdX, 36, holdText);
}

void StopwatchApp::drawStart(U8G2& u8g2) {
  // Custom intro like Credits/Options: show title and prompt
  u8g2.drawFrame(0, 0, width + 2, height);
  u8g2.setFont(u8g2_font_5x8_tr);
  u8g2.drawStr(3, 10, "Stopwatch");
  u8g2.setFont(u8g2_font_4x6_tr);
  u8g2.drawStr(3, 24, "Tap to start");
  u8g2.drawStr(3, 36, "Hold=Reset");
}

bool StopwatchApp::hasCustomOverlay() const {
  return true;
}
