#pragma once

#include "App.h"

class DefenderMiniGame : public App {
  public:
    DefenderMiniGame(uint32_t width, uint32_t height, uint32_t left);
    bool hasCustomOverlay() const override;

  protected:
    void onAppReset() override;
    void updateRunning(uint32_t deltaMs, const ButtonInput& input) override;
    void drawRunning(U8G2& u8g2) override;
    void drawStart(U8G2& u8g2) override;
    void drawEnd(U8G2& u8g2) override;

  private:
    static constexpr uint8_t BAND_COUNT = 3;
    static constexpr uint8_t ENEMY_COUNT = 6;
    static constexpr uint8_t SHOT_COUNT = 6;

    struct Enemy {
      float x = 0.0f;
      uint8_t band = 0;
      bool active = false;
    };

    struct Shot {
      float x = 0.0f;
      uint8_t band = 0;
      bool active = false;
    };

    int bandY(uint8_t band) const;
    void loadBestScore();
    void saveBestScore();
    void loseHealth();
    void spawnEnemy();
    void spawnShot();

    uint32_t left_;
    uint8_t shipBand_ = 1;
    uint16_t spawnTimerMs_ = 0;
    uint16_t shotTimerMs_ = 0;
    uint8_t health_ = 5;
    uint16_t score_ = 0;
    uint16_t bestScore_ = 0;
    uint16_t bestInitials_ = 0;
    bool bestLoaded_ = false;
    Enemy enemies_[ENEMY_COUNT] = {};
    Shot shots_[SHOT_COUNT] = {};
};
