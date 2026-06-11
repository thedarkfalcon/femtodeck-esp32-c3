#pragma once
#include "../../App.h"
#include "../shared/logic/CommunicatorLogic.h"

class CommunicatorApp : public App {
public:
    CommunicatorApp(uint32_t width, uint32_t height);
    bool hasCustomOverlay() const override { return true; }
protected:
    void onAppReset() override;
    void updateRunning(uint32_t deltaMs, const ButtonInput& b1, const ButtonInput& b2) override;
    void drawRunning(TFT_eSPI& tft) override;
private:
    CommunicatorLogic logic_;
};
