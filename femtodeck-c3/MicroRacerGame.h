#pragma once

#include "App.h"

class MicroRacerGame : public App {
  public:
    MicroRacerGame(uint32_t width, uint32_t height, uint32_t left);
    bool hasCustomOverlay() const override;

  protected:
    void onAppReset() override;
    void updateRunning(uint32_t deltaMs, const ButtonInput& input) override;
    void drawRunning(U8G2& u8g2) override;
    void drawStart(U8G2& u8g2) override;
    void drawEnd(U8G2& u8g2) override;

  private:
    static constexpr uint8_t LANE_COUNT = 3;
    static constexpr uint8_t OBSTACLE_COUNT = 8;
    static constexpr int PLAYER_Y = 33;

    struct Obstacle {
      float y = 0.0f;
      uint8_t lane = 0;
      bool active = false;
    };

    int laneX(uint8_t lane) const;
    void loadBestScore();
    void saveBestScore();
    void spawnObstacle();

    uint32_t left_;
    uint8_t playerLane_ = 1;
    uint8_t level_ = 1;
    uint16_t spawnTimerMs_ = 0;
    float speedPxPerSec_ = 18.0f;
    uint16_t score_ = 0;
    uint16_t bestScore_ = 0;
    uint16_t bestInitials_ = 0;
    bool bestLoaded_ = false;
    Obstacle obstacles_[OBSTACLE_COUNT] = {};
};
