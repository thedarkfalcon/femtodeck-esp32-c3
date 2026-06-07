#include "OptionsApp.h"

#include <U8g2lib.h>

#include "PlayerProfile.h"

OptionsApp::OptionsApp(uint32_t width, uint32_t height)
    : App("Options", width, height) {}

bool OptionsApp::hasCustomOverlay() const {
  return true;
}

void OptionsApp::onAppReset() {
  PlayerProfile::unpackInitials(PlayerProfile::loadInitials(), initials_);
  selected_ = 0;
}

void OptionsApp::updateRunning(uint32_t deltaMs, const ButtonInput& input) {
  (void)deltaMs;
  if (input.click) {
    initials_[selected_] = nextInitial(initials_[selected_]);
  }
  if (input.longPress) {
    if (selected_ == 0) {
      selected_ = 1;
    } else {
      PlayerProfile::saveInitials(initials_[0], initials_[1]);
      requestExitToMenu();
    }
  }
}

void OptionsApp::drawStart(U8G2& u8g2) {
  char dotted[4];
  PlayerProfile::unpackDottedInitials(PlayerProfile::loadInitials(), dotted);

  u8g2.drawFrame(0, 0, width + 2, height);
  u8g2.setFont(u8g2_font_5x8_tr);
  u8g2.drawStr(3, 9, "Options");
  u8g2.setFont(u8g2_font_4x6_tr);
  u8g2.drawStr(3, 21, "Player");
  u8g2.drawStr(36, 21, dotted);
  u8g2.drawStr(3, 38, "Tap edit");
}

void OptionsApp::drawRunning(U8G2& u8g2) {
  u8g2.drawFrame(0, 0, width + 2, height);
  u8g2.setFont(u8g2_font_5x8_tr);
  u8g2.drawStr(3, 9, "Initials");
  u8g2.setCursor(26, 24);
  u8g2.print(initials_[0]);
  u8g2.print(".");
  u8g2.print(initials_[1]);
  const int cursorX = selected_ == 0 ? 25 : 37;
  u8g2.drawLine(cursorX, 27, cursorX + 5, 27);
  u8g2.setFont(u8g2_font_4x6_tr);
  u8g2.drawStr(3, 38, selected_ == 0 ? "Tap char Hold next" : "Hold save/menu");
}

void OptionsApp::drawEnd(U8G2& u8g2) {
  u8g2.drawFrame(0, 0, width + 2, height);
  u8g2.setFont(u8g2_font_5x8_tr);
  u8g2.drawStr(3, 9, "Saved");
  u8g2.setCursor(25, 24);
  u8g2.print(initials_[0]);
  u8g2.print(".");
  u8g2.print(initials_[1]);
  u8g2.setFont(u8g2_font_4x6_tr);
  u8g2.drawStr(3, 38, "Hold menu");
}

char OptionsApp::nextInitial(char value) const {
  value = PlayerProfile::normalizeInitial(value);
  if (value >= 'A' && value < 'Z') {
    return static_cast<char>(value + 1);
  }
  if (value == 'Z') {
    return '0';
  }
  if (value >= '0' && value < '9') {
    return static_cast<char>(value + 1);
  }
  return 'A';
}
