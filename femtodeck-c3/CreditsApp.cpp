#include "CreditsApp.h"

#include <U8g2lib.h>

#include "Version.h"

namespace {
constexpr uint8_t CREDIT_PAGE_COUNT = 24;

struct CreditPage {
  const char* game;
  const char* author1;
  const char* author2;
};

const CreditPage CREDIT_PAGES[CREDIT_PAGE_COUNT] = {
    {"Femto OS", "thedarkfalcon", nullptr},
    {"Breakout", "atomic14", "thedarkfalcon"},
    {"Micro Racer", "atomic14", "thedarkfalcon"},
    {"Defender Mini", "atomic14", "thedarkfalcon"},
    {"Jump Run", "atomic14", "thedarkfalcon"},
    {"Heli Cave", "atomic14", "thedarkfalcon"},
    {"Tower Stacker", "thedarkfalcon", nullptr},
    {"Tiny Golf", "thedarkfalcon", nullptr},
    {"Mini Lander", "thedarkfalcon", nullptr},
    {"Need Speed", "thedarkfalcon", nullptr},
    {"Noon Shooter", "thedarkfalcon", nullptr},
    {"Fishing Flick", "thedarkfalcon", nullptr},
    {"Maze Runner", "thedarkfalcon", nullptr},
    {"Maze Collector", "thedarkfalcon", nullptr},
    {"Pipe Mania", "thedarkfalcon", nullptr},
    {"Blackjack", "thedarkfalcon", nullptr},
    {"Counter", "thedarkfalcon", nullptr},
    {"Mouse Emulator", "thedarkfalcon", nullptr},
    {"Reading", "thedarkfalcon", nullptr},
    {"Stopwatch", "thedarkfalcon", nullptr},
    {"Countdown", "thedarkfalcon", nullptr},
    {"Options", "thedarkfalcon", nullptr},
    {"Credits", "thedarkfalcon", nullptr},
    {"Repo", "git.new/esp32games", nullptr},
};

void drawGithubHandle(U8G2& u8g2, int x, int y, const char* handle) {
  if (handle == nullptr) {
    return;
  }
  if (handle[0] == 't') {
    u8g2.drawStr(x, y, "github:");
    u8g2.drawStr(x, y + 7, handle);
  } else {
    u8g2.drawStr(x, y, "github:");
    u8g2.drawStr(x + 28, y, handle);
  }
}
}

CreditsApp::CreditsApp(uint32_t width, uint32_t height)
    : App("Credits", width, height) {}

bool CreditsApp::hasCustomOverlay() const {
  return true;
}

void CreditsApp::onAppReset() {
  page_ = 0;
}

void CreditsApp::updateRunning(uint32_t deltaMs, const ButtonInput& input) {
  (void)deltaMs;
  if (input.longPress) {
    requestExitToMenu();
    return;
  }
  if (input.click) {
    page_++;
    if (page_ >= CREDIT_PAGE_COUNT) {
      endApp();
    }
  }
}

void CreditsApp::drawRunning(U8G2& u8g2) {
  u8g2.drawFrame(0, 0, width + 2, height);
  const CreditPage& credit = CREDIT_PAGES[page_];

  u8g2.setFont(u8g2_font_5x8_tr);
  u8g2.drawStr(3, 9, credit.game);

  u8g2.setFont(u8g2_font_4x6_tr);
  if (credit.game[0] == 'R') {
    u8g2.drawStr(3, 19, "Repo URL");
    u8g2.drawStr(3, 29, "git.new/");
    u8g2.drawStr(3, 37, "esp32games");
    return;
  }
  if (credit.author2 != nullptr) {
    u8g2.drawStr(3, 18, "Created:atomic14");
    u8g2.drawStr(3, 29, "Modified:");
    u8g2.drawStr(3, 38, credit.author2);
  } else if (credit.author1[0] != 't') {
    u8g2.drawStr(3, 19, "Developer");
    drawGithubHandle(u8g2, 3, 28, credit.author1);
  } else {
    u8g2.drawStr(3, 19, "Developer");
    drawGithubHandle(u8g2, 3, 28, credit.author1);
  }
}

void CreditsApp::drawStart(U8G2& u8g2) {
  u8g2.drawFrame(0, 0, width + 2, height);
  u8g2.setFont(u8g2_font_5x8_tr);
  u8g2.drawStr(3, 10, "Credits");
  u8g2.drawStr(3, 24, "Tap to view");
  u8g2.setFont(u8g2_font_4x6_tr);
  u8g2.drawStr(3, 36, BuildInfo::BUILD_TEXT);
}

void CreditsApp::drawEnd(U8G2& u8g2) {
  u8g2.drawFrame(0, 0, width + 2, height);
  u8g2.setFont(u8g2_font_5x8_tr);
  u8g2.drawStr(3, 12, "Thanks");
  u8g2.drawStr(3, 26, "Tap replay");
  u8g2.setFont(u8g2_font_4x6_tr);
  u8g2.drawStr(3, 32, BuildInfo::BUILD_TEXT);
  u8g2.drawStr(3, 38, "Hold menu");
}
