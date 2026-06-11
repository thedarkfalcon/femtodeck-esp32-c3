#include "CommunicatorApp.h"
#include <TFT_eSPI.h>

CommunicatorApp::CommunicatorApp(uint32_t width, uint32_t height)
    : App("Communicator", width, height) {}

void CommunicatorApp::onAppReset() {
    // Radio init
}

void CommunicatorApp::updateRunning(uint32_t deltaMs, const ButtonInput& b1, const ButtonInput& b2) {
    if (b2.click) requestExitToMenu();
}

void CommunicatorApp::drawRunning(TFT_eSPI& tft) {
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.setTextSize(2);
    tft.drawString("ESP-NOW Comms", 10, 10);

    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE);
    tft.drawString("Ready to receive...", 10, 50);
}
