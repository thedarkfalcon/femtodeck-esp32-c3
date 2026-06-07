#pragma once

#include "App.h"

class JumpGame : public App {
  public:
    JumpGame(uint32_t width, uint32_t height, uint32_t left);
    bool hasCustomOverlay() const override;

  protected:
    void onAppReset() override;
    void updateRunning(uint32_t deltaMs, const ButtonInput& input) override;
    void drawRunning(U8G2& u8g2) override;
    void drawStart(U8G2& u8g2) override;
    void drawEnd(U8G2& u8g2) override;

  private:
    static constexpr int PLAYER_X = 12;
    static constexpr int PLAYER_W = 4;
    static constexpr int PLAYER_H = 6;
    static constexpr int GROUND_MARGIN = 3;
    static constexpr uint8_t OBSTACLE_COUNT = 5;

    struct Obstacle {
      float x = 0.0f;
      int w = 0;
      int h = 0;
      bool active = false;
    };

    int groundY() const;
    int playerY() const;
    bool isOnGround() const;
    void loadBestScore();
    void saveBestScore();
    void spawnObstacle();
    bool intersectsObstacle(const Obstacle& obstacle) const;

    uint32_t left_;
    float playerYPos_ = 0.0f;
    float playerVy_ = 0.0f;
    float obstacleSpeed_ = 20.0f;
    uint16_t spawnTimerMs_ = 0;
    uint16_t nextSpawnMs_ = 900;
    uint8_t spawnPattern_ = 0;
    uint16_t score_ = 0;
    uint16_t bestScore_ = 0;
    uint16_t bestInitials_ = 0;
    bool bestLoaded_ = false;
    Obstacle obstacles_[OBSTACLE_COUNT] = {};
};
