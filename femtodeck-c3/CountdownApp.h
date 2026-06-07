#pragma once

#include "App.h"

class CountdownApp : public App {
  public:
    CountdownApp(uint32_t width, uint32_t height, uint32_t left = 1);

  protected:
    void onAppReset() override;
    void updateRunning(uint32_t deltaMs, const ButtonInput& input) override;
    void drawRunning(U8G2& u8g2) override;
    void drawStart(U8G2& u8g2) override;
    bool hasCustomOverlay() const override;

  private:
    enum class Mode { Select, Running, Paused, Finished };
    Mode mode_ = Mode::Select;
    static const uint32_t DURATIONS_MS[];
    static const uint8_t DURATION_COUNT;
    uint8_t selectedIndex_ = 0; // default to first entry (10 seconds)
    uint32_t durationMs_ = 60000; // currently selected duration
    uint32_t remainingMs_ = 0;
    bool running_ = false;
    bool finished_ = false;
    uint32_t flashMs_ = 0;
    bool flashOn_ = false;
};
