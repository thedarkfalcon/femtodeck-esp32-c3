#pragma once

#include "App.h"

class StopwatchApp : public App {
  public:
    StopwatchApp(uint32_t width, uint32_t height, uint32_t left = 1);

  protected:
    void onAppReset() override;
    void updateRunning(uint32_t deltaMs, const ButtonInput& input) override;
    void drawRunning(U8G2& u8g2) override;
    void drawStart(U8G2& u8g2) override;
    bool hasCustomOverlay() const override;

  private:
    bool running_ = false;
    uint32_t elapsedMs_ = 0;
};
