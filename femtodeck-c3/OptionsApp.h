#pragma once

#include "App.h"

class OptionsApp : public App {
  public:
    OptionsApp(uint32_t width, uint32_t height);
    bool hasCustomOverlay() const override;

  protected:
    void onAppReset() override;
    void updateRunning(uint32_t deltaMs, const ButtonInput& input) override;
    void drawStart(U8G2& u8g2) override;
    void drawRunning(U8G2& u8g2) override;
    void drawEnd(U8G2& u8g2) override;

  private:
    char nextInitial(char value) const;

    char initials_[3] = {'J', 'F', '\0'};
    uint8_t selected_ = 0;
};
