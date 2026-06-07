#include "CountdownApp.h"

#include <Arduino.h>
#include <U8g2lib.h>

namespace {
constexpr uint8_t ALERT_LED_PIN = 8;
constexpr bool ALERT_LED_ACTIVE_LOW = true;
}

static void setAlertLed(bool on) {
  digitalWrite(ALERT_LED_PIN, ALERT_LED_ACTIVE_LOW ? !on : on);
}
// Durations list (seconds): 10s,20s,30s,1m,2m,3m,4m,5m,6m,7m,8m,9m,10m,15m,20m,25m,30m,40m,50m,60m
const uint32_t CountdownApp::DURATIONS_MS[] = {
    10 * 1000, 20 * 1000, 30 * 1000, 60 * 1000, 2 * 60 * 1000, 3 * 60 * 1000, 4 * 60 * 1000,
    5 * 60 * 1000, 6 * 60 * 1000, 7 * 60 * 1000, 8 * 60 * 1000, 9 * 60 * 1000, 10 * 60 * 1000,
    15 * 60 * 1000, 20 * 60 * 1000, 25 * 60 * 1000, 30 * 60 * 1000, 40 * 60 * 1000, 50 * 60 * 1000,
    60 * 60 * 1000};
const uint8_t CountdownApp::DURATION_COUNT = sizeof(CountdownApp::DURATIONS_MS) / sizeof(CountdownApp::DURATIONS_MS[0]);

CountdownApp::CountdownApp(uint32_t width, uint32_t height, uint32_t left)
    : App("Countdown", width, height), durationMs_(DURATIONS_MS[selectedIndex_]), remainingMs_(durationMs_) {}

void CountdownApp::onAppReset() {
  // Start in selection mode
  mode_ = Mode::Select;
  durationMs_ = DURATIONS_MS[selectedIndex_];
  remainingMs_ = durationMs_;
  running_ = false;
  finished_ = false;
  flashMs_ = 0;
  flashOn_ = false;
  pinMode(ALERT_LED_PIN, OUTPUT);
  setAlertLed(false);
}

void CountdownApp::updateRunning(uint32_t deltaMs, const ButtonInput& input) {
  switch (mode_) {
    case Mode::Select:
      if (input.click) {
        // cycle selection
        selectedIndex_ = (selectedIndex_ + 1) % DURATION_COUNT;
        durationMs_ = DURATIONS_MS[selectedIndex_];
        remainingMs_ = durationMs_;
      }
      if (input.longPress) {
        // start countdown
        running_ = true;
        finished_ = false;
        mode_ = Mode::Running;
        remainingMs_ = durationMs_;
      }
      break;
    case Mode::Running:
      if (input.click) {
        // pause
        running_ = false;
        mode_ = Mode::Paused;
      }
      if (input.longPress) {
        // restart to selected duration
        remainingMs_ = durationMs_;
        running_ = true;
        mode_ = Mode::Running;
        finished_ = false;
        flashOn_ = false;
        flashMs_ = 0;
      }
      if (running_ && !finished_) {
        if (deltaMs >= remainingMs_) {
          remainingMs_ = 0;
          finished_ = true;
          running_ = false;
          mode_ = Mode::Finished;
          flashMs_ = 0;
          flashOn_ = true;
        } else {
          remainingMs_ -= deltaMs;
        }
      }
      break;
    case Mode::Paused:
      if (input.click) {
        // resume
        running_ = true;
        mode_ = Mode::Running;
      }
      if (input.longPress) {
        // long-press while paused: return to selection screen
        remainingMs_ = durationMs_;
        running_ = false;
        mode_ = Mode::Select;
        finished_ = false;
        flashOn_ = false;
        flashMs_ = 0;
      }
      break;
    case Mode::Finished:
      if (input.click) {
        // tapping after finished resets to selected duration but stays on finished screen until user acts
        remainingMs_ = durationMs_;
        finished_ = false;
        mode_ = Mode::Select;
      }
      if (input.longPress) {
        // long press returns to selection screen
        remainingMs_ = durationMs_;
        finished_ = false;
        mode_ = Mode::Select;
      }
      break;
  }

  if (mode_ == Mode::Finished) {
    flashMs_ += deltaMs;
    if (flashMs_ >= 300) {
      flashMs_ = 0;
      flashOn_ = !flashOn_;
    }
  }

  // Update hardware LED to match flash state when finished, otherwise ensure off
  if (mode_ == Mode::Finished) {
    setAlertLed(flashOn_);
  } else {
    setAlertLed(false);
  }
}

static void formatMmSs(char* buf, size_t bufSize, uint32_t ms) {
  uint32_t totalSec = ms / 1000;
  uint32_t sec = totalSec % 60;
  uint32_t min = totalSec / 60;
  snprintf(buf, bufSize, "%02u:%02u", (unsigned)min, (unsigned)sec);
}

void CountdownApp::drawStart(U8G2& u8g2) {
  // Selection screen
  u8g2.drawFrame(0, 0, width + 2, height);
  char buf[16];
  formatMmSs(buf, sizeof(buf), durationMs_);
  u8g2.setFont(u8g2_font_7x13_tr);
  int textW = u8g2.getStrWidth(buf);
  int x = (int)((width + 2 - textW) / 2);
  u8g2.drawStr(x, 22, buf);
  u8g2.setFont(u8g2_font_4x6_tr);
  // tap hint at top, hold hint at bottom
  u8g2.drawStr(3, 9, "Tap=cycle");
  u8g2.drawStr(3, 36, "Hold=start");
}

void CountdownApp::drawRunning(U8G2& u8g2) {
  if (mode_ == Mode::Finished && flashOn_) {
    u8g2.drawBox(0, 0, width + 2, height);
    u8g2.setDrawColor(0);
    u8g2.setFont(u8g2_font_5x8_tr);
    u8g2.drawStr(12, 24, "TIME!");
    u8g2.setDrawColor(1);
    return;
  }

  u8g2.drawFrame(0, 0, width, height);

  char buf[16];
  formatMmSs(buf, sizeof(buf), remainingMs_);

  u8g2.setFont(u8g2_font_7x13_tr);
  int textW = u8g2.getStrWidth(buf);
  int x = (int)((width + 2 - textW) / 2);
  u8g2.drawStr(x, 22, buf);

  u8g2.setFont(u8g2_font_4x6_tr);
  if (mode_ == Mode::Select) {
    u8g2.drawStr(3, 9, "Tap=cycle");
    u8g2.drawStr(3, 36, "Hold=start");
  } else if (mode_ == Mode::Running) {
    u8g2.drawStr(3, 9, "Tap=pause");
    u8g2.drawStr(3, 36, "Hold=restart");
  } else if (mode_ == Mode::Paused) {
    u8g2.drawStr(3, 9, "Tap=resume");
    u8g2.drawStr(3, 36, "Hold=select");
  } else if (mode_ == Mode::Finished) {
    u8g2.drawStr(3, 9, "Tap=reset");
    u8g2.drawStr(3, 36, "Hold=select");
  }
}

bool CountdownApp::hasCustomOverlay() const {
  return true;
}
