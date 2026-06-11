#pragma once
#include "../../App.h"
#include "../shared/logic/WiFiLogic.h"
#include <WebServer.h>
#include <DNSServer.h>

class WiFiSetupApp : public App {
public:
    WiFiSetupApp(uint32_t width, uint32_t height);
    bool hasCustomOverlay() const override { return true; }
protected:
    void onAppReset() override;
    void updateRunning(uint32_t deltaMs, const ButtonInput& b1, const ButtonInput& b2) override;
    void drawRunning(TFT_eSPI& tft) override;
private:
    WiFiLogic logic_;
    WebServer server_;
    DNSServer dns_;
    uint8_t menuIndex_ = 0;
};
