#pragma once

#include <Arduino.h>
#include <DNSServer.h>
#include <FS.h>
#include <IPAddress.h>

using fs::FS;

#include <WebServer.h>

#include "../../App.h"
#include "../shared/logic/MinerLogic.h"

class FemtoMinerApp : public App {
  public:
    FemtoMinerApp(uint32_t width, uint32_t height);

    bool hasCustomOverlay() const override;
    uint16_t runningRenderIntervalMs() const override;
    void setDebugAutoStart(bool enabled);
    void debugStartMining();
    void debugStopMining();
    void debugPrintStats(const char* reason = "manual");

  protected:
    void onAppReset() override;
    void onAppExit() override;
    void updateRunning(uint32_t deltaMs, const ButtonInput& b1, const ButtonInput& b2) override;
    void drawRunning(TFT_eSPI& tft) override;
    void drawStart(TFT_eSPI& tft) override;
    bool startsRunningImmediately() const override;

  private:
    enum class Mode { Dashboard, StartingPortal, Portal, Message, ConfirmReset };

    void markDirty();
    void launchPortal();
    void startPortal();
    void stopPortal();
    void setupRoutes();
    void handleRoot();
    void handleSave();
    void handleReset();
    void handleNotFound();
    String buildPage(const char* notice) const;
    String escapeHtml(const String& value) const;
    void showMessage(const char* message);
    const char* pageTitle() const;
    uint16_t stateColor(MinerLogic::State state) const;
    void maybePrintSerialStats(uint32_t nowMs);
    void printStatsLine(const MinerLogic::Stats& stats, const char* reason);

    template <typename Canvas>
    void drawFrame(Canvas& canvas);
    template <typename Canvas>
    void drawDashboard(Canvas& canvas, const MinerLogic::Stats& stats);
    template <typename Canvas>
    void drawDetails(Canvas& canvas, const MinerLogic::Stats& stats);
    template <typename Canvas>
    void drawSettings(Canvas& canvas);
    template <typename Canvas>
    void drawReset(Canvas& canvas);
    template <typename Canvas>
    void drawPortal(Canvas& canvas);
    template <typename Canvas>
    void drawMessage(Canvas& canvas);
    template <typename Canvas>
    void drawConfirmReset(Canvas& canvas);

    MinerLogic::MinerEngine miner_;
    MinerLogic::MinerSettings settings_;
    WebServer server_;
    DNSServer dns_;
    IPAddress apIP_;
    Mode mode_ = Mode::Dashboard;
    uint8_t page_ = 0;
    bool portalRunning_ = false;
    bool portalLaunchPending_ = false;
    bool dirty_ = true;
    bool debugAutoStart_ = false;
    uint32_t modeStartedAtMs_ = 0;
    uint32_t lastSerialStatsMs_ = 0;
    const char* message_ = "";
};
