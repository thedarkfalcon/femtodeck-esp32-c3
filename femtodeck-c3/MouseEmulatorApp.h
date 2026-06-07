#pragma once

#include "App.h"

class MouseEmulatorApp : public App {
  public:
    MouseEmulatorApp(uint32_t width, uint32_t height, uint32_t left = 1);

  protected:
    void onAppReset() override;
    void updateRunning(uint32_t deltaMs, const ButtonInput& input) override;
    void drawRunning(U8G2& u8g2) override;
    void drawStart(U8G2& u8g2) override;
    bool hasCustomOverlay() const override;

  private:
    void sendHumanizedStep();

    uint32_t targetIntervalMs_ = 0;
    uint32_t lastJiggleTime_ = 0;
    uint32_t lastClockUpdate_ = 0;
    bool jigglerEnabled_ = true;
    bool isMoving_ = false;

    enum MovePhase { Idle = 0, Forward = 1, Waiting = 2, Back = 3 };
    MovePhase phase_ = Idle;
    int movementTotal_ = 0;      // total pixels to move this cycle
    int remainingSteps_ = 0;     // steps left in current direction
    int movementDirection_ = 1;  // 1 or -1
    uint32_t waitUntilMs_ = 0;   // when to start the next phase (between forward/back)
    int stepsPerTick_ = 30;      // number of micro-steps to send per tick
    int verticalOffset_ = 0;      // bounded wiggle offset so the cursor does not drift away
    int verticalJitterCountdown_ = 0;
};
