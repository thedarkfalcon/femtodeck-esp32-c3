#include "WiFiSetupApp.h"
#include <TFT_eSPI.h>
#include <WiFi.h>

WiFiSetupApp::WiFiSetupApp(uint32_t width, uint32_t height)
    : App("WiFi Setup", width, height), server_(80) {}

void WiFiSetupApp::onAppReset() {
    logic_.init();
    menuIndex_ = 0;
}

void WiFiSetupApp::updateRunning(uint32_t deltaMs, const ButtonInput& b1, const ButtonInput& b2) {
    if (b1.click) menuIndex_ = (menuIndex_ + 1) % 3;
    if (b2.click) requestExitToMenu();
}

void WiFiSetupApp::drawRunning(TFT_eSPI& tft) {
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.setTextSize(2);
    tft.drawString("WiFi Settings", 10, 10);

    tft.setTextSize(1);
    const char* items[] = {"Configure Portal", "Test Connections", "Delete All"};
    for (int i=0; i<3; i++) {
        tft.setCursor(20, 40 + i*20);
        if (i == menuIndex_) tft.setTextColor(TFT_BLACK, TFT_GREEN);
        else tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.print(items[i]);
    }
}
