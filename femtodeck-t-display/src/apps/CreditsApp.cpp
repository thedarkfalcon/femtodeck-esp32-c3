#include "CreditsApp.h"

#include <TFT_eSPI.h>

#include "../../Version.h"

namespace {
constexpr uint8_t ABOUT_ITEM_COUNT = 2;
constexpr uint8_t LICENSE_PAGE_COUNT = 4;

struct CreditPage {
  const char* game;
  const char* author1;
  const char* author2;
};

const char* const ABOUT_ITEMS[ABOUT_ITEM_COUNT] = {
    "License",
    "Credits",
};

const CreditPage CREDIT_PAGES[] = {
    {"Femto OS", "thedarkfalcon", nullptr},
    {"Inspired by", "atomic14", nullptr},
    {"Breakout '76", "thedarkfalcon", nullptr},
    {"City Racer", "thedarkfalcon", nullptr},
    {"Alien Raiders", "thedarkfalcon", nullptr},
    {"Cave Chopper", "thedarkfalcon", nullptr},
    {"Tower Stacker", "thedarkfalcon", nullptr},
    {"Tiny Golf", "thedarkfalcon", nullptr},
    {"Mini Lander", "thedarkfalcon", nullptr},
    {"Need Speed", "thedarkfalcon", nullptr},
    {"Femto Field", "thedarkfalcon", nullptr},
    {"Noon Shooter", "thedarkfalcon", nullptr},
    {"Fishing Flick", "thedarkfalcon", nullptr},
    {"Maze Runner", "thedarkfalcon", nullptr},
    {"Maze Collector", "thedarkfalcon", nullptr},
    {"Pipe Mania", "thedarkfalcon", nullptr},
    {"Blackjack", "thedarkfalcon", nullptr},
    {"Knife Throw", "thedarkfalcon", nullptr},
    {"Reactor", "thedarkfalcon", nullptr},
    {"Simon", "thedarkfalcon", nullptr},
    {"Pet Simulator", "thedarkfalcon", nullptr},
    {"Counter", "thedarkfalcon", nullptr},
    {"Mouse Emulator", "thedarkfalcon", nullptr},
    {"Reading", "thedarkfalcon", nullptr},
    {"Stopwatch", "thedarkfalcon", nullptr},
    {"Countdown", "thedarkfalcon", nullptr},
    {"Dice Roller", "thedarkfalcon", nullptr},
    {"Coin Flipper", "thedarkfalcon", nullptr},
    {"Random Number", "thedarkfalcon", nullptr},
    {"Metronome", "thedarkfalcon", nullptr},
    {"Options", "thedarkfalcon", nullptr},
    {"About", "thedarkfalcon", nullptr},
};
constexpr uint8_t CREDIT_PAGE_COUNT = sizeof(CREDIT_PAGES) / sizeof(CREDIT_PAGES[0]);

void drawGithubHandle(TFT_eSPI& tft, int x, int y, const char* handle) {
  if (handle == nullptr) {
    return;
  }
  if (handle[0] == 't') {
    tft.drawString(x, y, "github:");
    tft.drawString(x, y + 7, handle);
  } else {
    tft.drawString(x, y, "github:");
    tft.drawString(x + 28, y, handle);
  }
}
}

CreditsApp::CreditsApp(uint32_t width, uint32_t height)
    : App("About", width, height) {}

bool CreditsApp::hasCustomOverlay() const {
  return true;
}

void CreditsApp::onAppReset() {
  mode_ = Mode::Select;
  selection_ = 0;
  page_ = 0;
}

void CreditsApp::updateRunning(uint32_t deltaMs, const ButtonInput& b1, const ButtonInput& b2) {
  (void)deltaMs;

  if (mode_ == Mode::Select) {
    if (input.click) {
      selection_ = (selection_ + 1) % ABOUT_ITEM_COUNT;
    } else if (input.longPress) {
      mode_ = selection_ == 0 ? Mode::License : Mode::Credits;
      page_ = 0;
    }
    return;
  }

  if (input.longPress) {
    requestExitToMenu();
    return;
  }

  if (input.click) {
    page_++;
    const uint8_t pageCount = mode_ == Mode::License ? LICENSE_PAGE_COUNT : CREDIT_PAGE_COUNT;
    if (page_ >= pageCount) {
      mode_ = Mode::Select;
      page_ = 0;
    }
  }
}

void CreditsApp::drawRunning(TFT_eSPI& tft) {
  switch (mode_) {
    case Mode::License:
      drawLicense(tft);
      break;
    case Mode::Credits:
      drawCredits(tft);
      break;
    case Mode::Select:
    default:
      drawSelect(tft);
      break;
  }
}

void CreditsApp::drawSelect(TFT_eSPI& tft) {
  tft.drawRect(0, 0, width + 2, height);

  tft.drawString(3, 8, "About");
  tft.setCursor(width - 14, 8);
  tft.print(selection_ + 1);
  tft.print("/");
  tft.print(ABOUT_ITEM_COUNT);


  tft.drawString(3, 22, ABOUT_ITEMS[selection_]);
  tft.drawString(3, 32, "Tap next");

  tft.drawString(3, 39, "Hold open");
}

void CreditsApp::drawLicense(TFT_eSPI& tft) {
  tft.drawRect(0, 0, width + 2, height);


  switch (page_) {
    case 0:
      tft.drawString(3, 9, "License");
      tft.drawString(3, 22, "WTFPL +");
      tft.drawString(3, 34, "No Warranty");
      break;
    case 1:
      tft.drawString(3, 9, "Copyright");

      tft.drawString(3, 20, "2026");
      tft.drawString(3, 29, "github:");
      tft.drawString(3, 38, "thedarkfalcon");
      break;
    case 2:
      tft.drawString(3, 9, "Permission");

      tft.drawString(3, 20, "copy modify");
      tft.drawString(3, 29, "publish sell");
      tft.drawString(3, 38, "as you want");
      break;
    default:
      tft.drawString(3, 9, "No Warranty");

      tft.drawString(3, 20, "provided as-is");
      tft.drawString(3, 29, "use at your");
      tft.drawString(3, 38, "own risk");
      break;
  }
}

void CreditsApp::drawCredits(TFT_eSPI& tft) {
  tft.drawRect(0, 0, width + 2, height);
  const CreditPage& credit = CREDIT_PAGES[page_];


  tft.drawString(3, 9, credit.game);


  if (credit.author2 != nullptr) {
    tft.drawString(3, 18, "Created:atomic14");
    tft.drawString(3, 29, "Modified:");
    tft.drawString(3, 38, credit.author2);
  } else if (credit.author1[0] != 't') {
    tft.drawString(3, 19, "Developer");
    drawGithubHandle(tft, 3, 28, credit.author1);
  } else {
    tft.drawString(3, 19, "Developer");
    drawGithubHandle(tft, 3, 28, credit.author1);
  }
}

void CreditsApp::drawStart(TFT_eSPI& tft) {
  tft.drawRect(0, 0, width + 2, height);

  tft.drawString(3, 10, "About");
  tft.drawString(3, 24, "Tap to view");

  tft.drawString(3, 36, BuildInfo::BUILD_TEXT);
}

void CreditsApp::drawEnd(TFT_eSPI& tft) {
  tft.drawRect(0, 0, width + 2, height);

  tft.drawString(3, 12, "About");
  tft.drawString(3, 26, "Tap replay");

  tft.drawString(3, 32, BuildInfo::BUILD_TEXT);
  tft.drawString(3, 38, "Hold menu");
}
