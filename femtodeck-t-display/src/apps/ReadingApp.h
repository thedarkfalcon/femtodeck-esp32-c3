#pragma once

#include "../../App.h"

class ReadingApp : public App {
  public:
    ReadingApp(uint32_t width, uint32_t height);

    bool hasCustomOverlay() const override;

  protected:
    void onAppReset() override;
    void updateRunning(uint32_t deltaMs, const ButtonInput& b1, const ButtonInput& b2) override;
    void drawRunning(TFT_eSPI& tft) override;
    void drawStart(TFT_eSPI& tft) override;
    bool startsRunningImmediately() const override;

  private:
    enum class Mode {
      Select,
      Read,
      Complete
    };

    bool hasMoreAfterPage(const char* text, uint8_t page) const;
    void drawSelection(TFT_eSPI& tft) const;
    void drawReading(TFT_eSPI& tft) const;
    void drawComplete(TFT_eSPI& tft) const;
    void drawCenteredText(TFT_eSPI& tft, int y, const char* text) const;
    bool drawWrappedPage(TFT_eSPI& tft, const char* text, uint8_t page, int firstY) const;

    uint8_t selection_;
    uint8_t page_;
    Mode mode_;
};
