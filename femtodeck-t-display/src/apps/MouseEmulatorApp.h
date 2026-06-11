#pragma once

#include "../../App.h"
#include "../shared/logic/MouseEmulatorLogic.h"

class MouseEmulatorApp : public App {
  public:
    MouseEmulatorApp(uint32_t width, uint32_t height);

  protected:
    void onAppReset() override;
    void updateRunning(uint32_t deltaMs, const ButtonInput& b1, const ButtonInput& b2) override;
    void drawRunning(TFT_eSPI& tft) override;
    void drawStart(TFT_eSPI& tft) override;
    bool hasCustomOverlay() const override;

  private:
    void sendHumanizedStep();
    void updateDynamicStats(TFT_eSPI& tft);

    MouseEmulatorLogic logic_;
    uint32_t lastClockUpdate_ = 0;
    int stepsPerTick_ = 30;

    // UI state tracking to minimize redraws
    bool lastKnownConnection_ = false;
    bool lastKnownEnabled_ = false;
};
