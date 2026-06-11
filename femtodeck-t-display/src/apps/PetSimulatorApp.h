#pragma once

#include "../../App.h"

class PetSimulatorApp : public App {
  public:
    PetSimulatorApp(uint32_t width, uint32_t height, uint32_t left = 1);

  protected:
    void onAppReset() override;
    void updateRunning(uint32_t deltaMs, const ButtonInput& b1, const ButtonInput& b2) override;
    void drawRunning(TFT_eSPI& tft) override;
    bool startsRunningImmediately() const override;
    bool hasCustomOverlay() const override;

  private:
    enum class Mode { ChoosePet, Menu, Stats, Message };

    void loadPet();
    void savePet();
    void applyAction();
    void tickCare(uint32_t deltaMs);
    void updateIdle(uint32_t deltaMs);
    void drawPet(TFT_eSPI& tft, int x, int y);
    void drawBar(TFT_eSPI& tft, int x, int y, const char* label, uint8_t value);

    Mode mode_ = Mode::ChoosePet;
    uint8_t petType_ = 0;
    uint8_t menuIndex_ = 0;
    uint8_t hunger_ = 80;
    uint8_t fun_ = 80;
    uint8_t energy_ = 80;
    uint8_t clean_ = 80;
    uint8_t health_ = 100;
    uint8_t poop_ = 0;
    uint16_t careMs_ = 0;
    uint16_t messageMs_ = 0;
    uint16_t idleMs_ = 0;
    uint16_t wanderMs_ = 0;
    int8_t petX_ = 34;
    int8_t petY_ = 21;
    int8_t petDx_ = 1;
    int8_t petDy_ = 0;
    bool idleMode_ = false;
    const char* message_ = "";
    bool loaded_ = false;
};
