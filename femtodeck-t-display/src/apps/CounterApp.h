#pragma once

#include "../../App.h"
#include "../shared/logic/CounterLogic.h"

class CounterApp : public App {
  public:
    CounterApp(uint32_t width, uint32_t height);

  protected:
    void onAppReset() override;
    void updateRunning(uint32_t deltaMs, const ButtonInput& b1, const ButtonInput& b2) override;
    void drawRunning(TFT_eSPI& tft) override;
    void drawStart(TFT_eSPI& tft) override;
    bool hasCustomOverlay() const override;

  private:
    CounterLogic logic_;
};
