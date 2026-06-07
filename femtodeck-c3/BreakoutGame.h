#pragma once

#include "App.h"

class BreakoutGame : public App {
  public:
    BreakoutGame(uint32_t width, uint32_t height, uint32_t left);
    bool hasCustomOverlay() const override;

  protected:
    void onAppReset() override;
    void updateRunning(uint32_t deltaMs, const ButtonInput& input) override;
    void drawRunning(U8G2& u8g2) override;
    void drawStart(U8G2& u8g2) override;
    void drawEnd(U8G2& u8g2) override;

  private:
    static constexpr uint8_t PADDLE_WIDTH = 14;
    static constexpr int PADDLE_SPEED = 32;
    static constexpr uint8_t BRICK_ROWS = 4;
    static constexpr uint8_t BRICK_COLS = 8;
    static constexpr uint8_t BRICK_WIDTH = 8;
    static constexpr uint8_t BRICK_HEIGHT = 4;

    uint32_t left_;
    void loadBestScore();
    void saveBestScore();
    void loadLevel();
    void resetBall();

    float paddleX_ = 24.0f;
    int paddleDir_ = 1;
    float ballX_ = 35.0f;
    float ballY_ = 20.0f;
    float ballVX_ = 24.0f;
    float ballVY_ = -22.0f;
    int bricksLeft_ = 0;
    uint8_t level_ = 1;
    uint16_t score_ = 0;
    uint16_t bestScore_ = 0;
    uint16_t bestInitials_ = 0;
    bool bestLoaded_ = false;
    bool bricks_[BRICK_ROWS][BRICK_COLS] = {};
};
