#include "FemtoMinerApp.h"

#include <TFT_eSPI.h>
#include <WiFi.h>
#include <esp_wifi.h>

#include "../../TDisplayUi.h"

namespace {
constexpr uint16_t DNS_PORT = 53;
constexpr uint8_t AP_CHANNEL = 6;
constexpr uint16_t PORTAL_LAUNCH_NOTICE_MS = 250;
constexpr const char* HOSTNAME = "FemtoDeck-TDisplay";
const IPAddress AP_IP(192, 168, 4, 1);
const IPAddress AP_GATEWAY(192, 168, 4, 1);
const IPAddress AP_SUBNET(255, 255, 255, 0);

template <typename Drawer>
void drawBuffered(TFT_eSPI& tft, uint32_t width, uint32_t height, Drawer drawer) {
  static TFT_eSprite frame(&tft);
  static bool frameTried = false;
  static bool frameReady = false;
  if (!frameTried) {
    frameTried = true;
    frame.setColorDepth(8);
    frameReady = frame.createSprite(width, height) != nullptr;
  }

  if (frameReady) {
    drawer(frame);
    frame.pushSprite(0, 0);
  } else {
    drawer(tft);
  }
}

void drawFit(TFT_eSprite& canvas, const String& text, int x, int y, int maxWidth, uint8_t size, uint16_t color) {
  canvas.setTextSize(size);
  canvas.setTextColor(color, TFT_BLACK);
  String clipped = text;
  while (clipped.length() > 0 && canvas.textWidth(clipped) > maxWidth) clipped.remove(clipped.length() - 1);
  canvas.drawString(clipped, x, y);
}

template <typename Canvas>
void drawFitAny(Canvas& canvas, const String& text, int x, int y, int maxWidth, uint8_t size, uint16_t color) {
  canvas.setTextDatum(TL_DATUM);
  canvas.setTextSize(size);
  canvas.setTextColor(color, TFT_BLACK);
  String clipped = text;
  while (clipped.length() > 0 && canvas.textWidth(clipped) > maxWidth) clipped.remove(clipped.length() - 1);
  canvas.drawString(clipped, x, y);
}
}

FemtoMinerApp::FemtoMinerApp(uint32_t width, uint32_t height)
    : App("Femto Miner", width, height), server_(80), apIP_(AP_IP) {}

bool FemtoMinerApp::hasCustomOverlay() const {
  return true;
}

bool FemtoMinerApp::startsRunningImmediately() const {
  return true;
}

uint16_t FemtoMinerApp::runningRenderIntervalMs() const {
  return mode_ == Mode::Portal ? 500 : 750;
}

void FemtoMinerApp::setDebugAutoStart(bool enabled) {
  debugAutoStart_ = enabled;
}

void FemtoMinerApp::debugStartMining() {
  if (!miner_.running()) {
    miner_.start(HOSTNAME);
    markDirty();
  }
  debugPrintStats("debug-start");
}

void FemtoMinerApp::debugStopMining() {
  if (miner_.running()) {
    miner_.stop();
    markDirty();
  }
  debugPrintStats("debug-stop");
}

void FemtoMinerApp::debugPrintStats(const char* reason) {
  printStatsLine(miner_.stats(), reason);
}

void FemtoMinerApp::markDirty() {
  dirty_ = true;
}

void FemtoMinerApp::onAppReset() {
  mode_ = Mode::Dashboard;
  page_ = 0;
  message_ = "";
  portalLaunchPending_ = false;
  modeStartedAtMs_ = millis();
  settings_.load();
  stopPortal();
  lastSerialStatsMs_ = 0;
  if (debugAutoStart_) {
    miner_.start(HOSTNAME);
  }
  markDirty();
}

void FemtoMinerApp::onAppExit() {
  stopPortal();
  miner_.stop();
}

void FemtoMinerApp::launchPortal() {
  miner_.stop();
  mode_ = Mode::StartingPortal;
  portalLaunchPending_ = true;
  modeStartedAtMs_ = millis();
  markDirty();
}

void FemtoMinerApp::startPortal() {
  portalLaunchPending_ = false;
  stopPortal();
  WiFi.persistent(false);
  WiFi.mode(WIFI_AP);
  WiFi.setSleep(false);
  WiFi.setTxPower(WIFI_POWER_8_5dBm);
  if (!WiFi.softAPConfig(AP_IP, AP_GATEWAY, AP_SUBNET) ||
      !WiFi.softAP(MinerLogic::SETUP_AP_SSID, MinerLogic::SETUP_AP_PASS, AP_CHANNEL, false, 4)) {
    showMessage("AP failed");
    return;
  }
  esp_wifi_set_bandwidth(WIFI_IF_AP, WIFI_BW_HT20);
  esp_wifi_set_protocol(WIFI_IF_AP, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G);
  setupRoutes();
  dns_.start(DNS_PORT, "*", apIP_);
  server_.begin();
  portalRunning_ = true;
  mode_ = Mode::Portal;
  markDirty();
}

void FemtoMinerApp::stopPortal() {
  if (portalRunning_) {
    server_.stop();
    dns_.stop();
  }
  portalRunning_ = false;
  if (mode_ == Mode::Portal || mode_ == Mode::StartingPortal) {
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_OFF);
  }
}

void FemtoMinerApp::setupRoutes() {
  server_.on("/", HTTP_GET, [this]() { handleRoot(); });
  server_.on("/save", HTTP_POST, [this]() { handleSave(); });
  server_.on("/reset", HTTP_POST, [this]() { handleReset(); });
  server_.on("/generate_204", HTTP_GET, [this]() { handleRoot(); });
  server_.on("/hotspot-detect.html", HTTP_GET, [this]() { handleRoot(); });
  server_.on("/fwlink", HTTP_GET, [this]() { handleRoot(); });
  server_.onNotFound([this]() { handleNotFound(); });
}

void FemtoMinerApp::handleRoot() {
  server_.send(200, "text/html", buildPage(nullptr));
}

void FemtoMinerApp::handleSave() {
  MinerLogic::Config config;
  config.wallet = server_.arg("wallet");
  config.poolHost = server_.arg("host");
  config.poolPass = server_.arg("pass");
  config.poolPort = static_cast<uint16_t>(server_.arg("port").toInt());
  settings_.save(config);
  server_.send(200, "text/html", buildPage("Saved. Return to FemtoDeck."));
  markDirty();
}

void FemtoMinerApp::handleReset() {
  settings_.reset();
  server_.send(200, "text/html", buildPage("Miner settings reset."));
  markDirty();
}

void FemtoMinerApp::handleNotFound() {
  server_.sendHeader("Location", "/", true);
  server_.send(302, "text/plain", "");
}

String FemtoMinerApp::escapeHtml(const String& value) const {
  String out;
  out.reserve(value.length());
  for (uint16_t i = 0; i < value.length(); i++) {
    const char c = value[i];
    if (c == '&') out += F("&amp;");
    else if (c == '<') out += F("&lt;");
    else if (c == '>') out += F("&gt;");
    else if (c == '"') out += F("&quot;");
    else out += c;
  }
  return out;
}

String FemtoMinerApp::buildPage(const char* notice) const {
  MinerLogic::MinerSettings s;
  s.load();
  const MinerLogic::Config& config = s.config();
  String html;
  html.reserve(2400);
  html += F("<!doctype html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'>");
  html += F("<title>Femto Miner Setup</title><style>body{font-family:sans-serif;margin:24px;max-width:640px}input{box-sizing:border-box;width:100%;padding:10px;margin:6px 0 14px}button{padding:10px 14px;margin-right:8px}.note{color:#075}.muted{color:#666}</style></head><body>");
  html += F("<h1>Femto Miner Setup</h1>");
  html += F("<p class='muted'>Configure pool and payout wallet. WiFi networks are still configured from FemtoDeck WiFi Setup.</p>");
  if (notice != nullptr) {
    html += F("<p class='note'>");
    html += notice;
    html += F("</p>");
  }
  html += F("<form method='post' action='/save'><label>Wallet</label><input name='wallet' value='");
  html += escapeHtml(config.wallet);
  html += F("'><label>Pool Host</label><input name='host' value='");
  html += escapeHtml(config.poolHost);
  html += F("'><label>Pool Port</label><input name='port' type='number' value='");
  html += String(config.poolPort);
  html += F("'><label>Pool Password</label><input name='pass' value='");
  html += escapeHtml(config.poolPass);
  html += F("'><button type='submit'>Save</button></form>");
  html += F("<form method='post' action='/reset'><button type='submit'>Reset Miner Settings</button></form>");
  html += F("<p class='muted'>Default worker name is ");
  html += MinerLogic::workerName();
  html += F(".</p></body></html>");
  return html;
}

void FemtoMinerApp::showMessage(const char* message) {
  message_ = message;
  mode_ = Mode::Message;
  modeStartedAtMs_ = millis();
  markDirty();
}

void FemtoMinerApp::updateRunning(uint32_t deltaMs, const ButtonInput& b1, const ButtonInput& b2) {
  (void)deltaMs;
  const uint32_t now = millis();
  maybePrintSerialStats(now);

  if (mode_ == Mode::StartingPortal && portalLaunchPending_ && now - modeStartedAtMs_ >= PORTAL_LAUNCH_NOTICE_MS) {
    startPortal();
  }

  if (mode_ == Mode::Portal) {
    dns_.processNextRequest();
    server_.handleClient();
    if (b2.click) {
      stopPortal();
      mode_ = Mode::Dashboard;
      markDirty();
    }
    return;
  }

  if (mode_ == Mode::Message) {
    if (b1.click || b2.click || b1.longPress) {
      mode_ = Mode::Dashboard;
      markDirty();
    }
    return;
  }

  if (mode_ == Mode::ConfirmReset) {
    if (b1.longPress) {
      settings_.reset();
      showMessage("Miner reset");
    } else if (b1.click || b2.click) {
      mode_ = Mode::Dashboard;
      markDirty();
    }
    return;
  }

  if (b1.longPress) {
    if (page_ == 3) mode_ = Mode::ConfirmReset;
    else launchPortal();
    markDirty();
    return;
  }
  if (b2.click) {
    page_ = (page_ + 1) % 4;
    markDirty();
    return;
  }
  if (b1.click) {
    if (miner_.running()) miner_.stop();
    else miner_.start(HOSTNAME);
    markDirty();
  }
}

void FemtoMinerApp::maybePrintSerialStats(uint32_t nowMs) {
  if (!miner_.running()) return;
  if (lastSerialStatsMs_ != 0 && nowMs - lastSerialStatsMs_ < 1000) return;
  lastSerialStatsMs_ = nowMs;
  printStatsLine(miner_.stats(), "tick");
}

void FemtoMinerApp::printStatsLine(const MinerLogic::Stats& stats, const char* reason) {
  Serial.print("[miner] reason=");
  Serial.print(reason);
  Serial.print(" state=");
  Serial.print(MinerLogic::stateLabel(stats.state));
  Serial.print(" hps=");
  Serial.print(stats.hashesPerSecond);
  Serial.print(" khps=");
  Serial.print(stats.hashesPerSecond / 1000.0f, 1);
  Serial.print(" total_kh=");
  Serial.print(static_cast<unsigned long>(stats.totalHashes / 1000ULL));
  Serial.print(" jobs=");
  Serial.print(stats.jobs);
  Serial.print(" submitted=");
  Serial.print(stats.submitted);
  Serial.print(" accepted=");
  Serial.print(stats.accepted);
  Serial.print(" rejected=");
  Serial.print(stats.rejected);
  Serial.print(" best=");
  Serial.print(stats.bestDifficulty, 8);
  Serial.print(" pool=");
  Serial.print(stats.poolDifficulty, 8);
  Serial.print(" uptime=");
  Serial.print(stats.uptimeSeconds);
  if (stats.lastError[0]) {
    Serial.print(" error=");
    Serial.print(stats.lastError);
  }
  Serial.println();
}

const char* FemtoMinerApp::pageTitle() const {
  if (page_ == 1) return "Miner Details";
  if (page_ == 2) return "Miner Setup";
  if (page_ == 3) return "Reset Miner";
  return "Femto Miner";
}

uint16_t FemtoMinerApp::stateColor(MinerLogic::State state) const {
  switch (state) {
    case MinerLogic::State::Mining: return TFT_GREEN;
    case MinerLogic::State::Idle: return TFT_CYAN;
    case MinerLogic::State::NoWifi:
    case MinerLogic::State::WifiFailed:
    case MinerLogic::State::PoolFailed:
    case MinerLogic::State::Error: return TFT_RED;
    case MinerLogic::State::Stopping: return TFT_ORANGE;
    default: return TFT_YELLOW;
  }
}

template <typename Canvas>
void FemtoMinerApp::drawDashboard(Canvas& canvas, const MinerLogic::Stats& stats) {
  const uint16_t color = stateColor(stats.state);
  TDisplayUi::header(canvas, pageTitle(), color, MinerLogic::stateLabel(stats.state));
  const float kh = stats.hashesPerSecond / 1000.0f;
  char rate[18];
  if (stats.hashesPerSecond >= 1000) snprintf(rate, sizeof(rate), "%.1f KH/s", kh);
  else snprintf(rate, sizeof(rate), "%lu H/s", static_cast<unsigned long>(stats.hashesPerSecond));
  TDisplayUi::largeValue(canvas, rate, 43, color);
  canvas.setTextSize(1);
  canvas.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  canvas.drawString(String("Jobs ") + stats.jobs + "  Sub " + stats.submitted, 18, 88);
  canvas.drawString(String("OK ") + stats.accepted + "  Rej " + stats.rejected, 18, 103);
  TDisplayUi::footer(canvas, miner_.running() ? "B1 stop  B2 page  B1 hold setup" : "B1 start  B2 page  B1 hold setup");
}

template <typename Canvas>
void FemtoMinerApp::drawDetails(Canvas& canvas, const MinerLogic::Stats& stats) {
  TDisplayUi::header(canvas, pageTitle(), stateColor(stats.state));
  canvas.setTextSize(1);
  canvas.setTextColor(TFT_WHITE, TFT_BLACK);
  canvas.drawString(String("WiFi: ") + (stats.ssid[0] ? stats.ssid : "-"), 14, 36);
  canvas.drawString(String("Pool diff: ") + String(stats.poolDifficulty, 6), 14, 52);
  canvas.drawString(String("Best diff: ") + String(stats.bestDifficulty, 6), 14, 68);
  canvas.drawString(String("Hashes: ") + static_cast<unsigned long>(stats.totalHashes), 14, 84);
  canvas.drawString(String("Uptime: ") + stats.uptimeSeconds + "s", 14, 100);
  if (stats.lastError[0]) drawFitAny(canvas, stats.lastError, 14, 115, 210, 1, TFT_RED);
  TDisplayUi::footer(canvas, "B2 page");
}

template <typename Canvas>
void FemtoMinerApp::drawSettings(Canvas& canvas) {
  settings_.load();
  const MinerLogic::Config& config = settings_.config();
  TDisplayUi::header(canvas, pageTitle(), TFT_ORANGE);
  drawFitAny(canvas, String("Wallet: ") + config.wallet, 14, 36, 212, 1, TFT_WHITE);
  drawFitAny(canvas, String("Pool: ") + config.poolHost + ":" + config.poolPort, 14, 54, 212, 1, TFT_CYAN);
  drawFitAny(canvas, String("Worker: ") + MinerLogic::workerName(), 14, 72, 212, 1, TFT_GREEN);
  drawFitAny(canvas, String("User: ") + MinerLogic::stratumUser(config), 14, 90, 212, 1, TFT_LIGHTGREY);
  TDisplayUi::footer(canvas, "B1 hold setup AP  B2 page");
}

template <typename Canvas>
void FemtoMinerApp::drawReset(Canvas& canvas) {
  TDisplayUi::header(canvas, pageTitle(), TFT_RED);
  TDisplayUi::centered(canvas, "Reset wallet", 42, 2, TFT_WHITE);
  TDisplayUi::centered(canvas, "and pool settings?", 68, 1, TFT_LIGHTGREY);
  TDisplayUi::footer(canvas, "B1 hold confirm  B2 page");
}

template <typename Canvas>
void FemtoMinerApp::drawPortal(Canvas& canvas) {
  TDisplayUi::header(canvas, mode_ == Mode::StartingPortal ? "Miner Setup" : "Setup AP", TFT_ORANGE);
  if (mode_ == Mode::StartingPortal) {
    TDisplayUi::centered(canvas, "Launching", 46, 2, TFT_YELLOW);
    TDisplayUi::centered(canvas, "Miner Setup AP", 73, 2, TFT_ORANGE);
  } else {
    TDisplayUi::centered(canvas, MinerLogic::SETUP_AP_SSID, 42, 2, TFT_ORANGE);
    TDisplayUi::centered(canvas, "Pass: femtominer", 70, 1, TFT_LIGHTGREY);
    TDisplayUi::centered(canvas, "Open 192.168.4.1", 88, 1, TFT_CYAN);
  }
  TDisplayUi::footer(canvas, "B2 back");
}

template <typename Canvas>
void FemtoMinerApp::drawMessage(Canvas& canvas) {
  TDisplayUi::header(canvas, "Femto Miner", TFT_YELLOW);
  TDisplayUi::largeValue(canvas, message_, 54, TFT_YELLOW);
  TDisplayUi::footer(canvas, "B1/B2 continue");
}

template <typename Canvas>
void FemtoMinerApp::drawConfirmReset(Canvas& canvas) {
  TDisplayUi::header(canvas, "Are You Sure?", TFT_RED);
  TDisplayUi::centered(canvas, "Hold B1", 47, 3, TFT_RED);
  TDisplayUi::centered(canvas, "to reset", 81, 2, TFT_LIGHTGREY);
  TDisplayUi::footer(canvas, "B1 hold yes  B2 no");
}

template <typename Canvas>
void FemtoMinerApp::drawFrame(Canvas& canvas) {
  TDisplayUi::clear(canvas);
  if (mode_ == Mode::StartingPortal || mode_ == Mode::Portal) {
    drawPortal(canvas);
    return;
  }
  if (mode_ == Mode::Message) {
    drawMessage(canvas);
    return;
  }
  if (mode_ == Mode::ConfirmReset) {
    drawConfirmReset(canvas);
    return;
  }
  const MinerLogic::Stats stats = miner_.stats();
  if (page_ == 1) drawDetails(canvas, stats);
  else if (page_ == 2) drawSettings(canvas);
  else if (page_ == 3) drawReset(canvas);
  else drawDashboard(canvas, stats);
}

void FemtoMinerApp::drawRunning(TFT_eSPI& tft) {
  tft.setRotation(1);
  drawBuffered(tft, width, height, [this](auto& canvas) { drawFrame(canvas); });
  dirty_ = false;
}

void FemtoMinerApp::drawStart(TFT_eSPI& tft) {
  tft.setRotation(1);
  TDisplayUi::clear(tft);
  if (showStartPromptPage()) {
    TDisplayUi::header(tft, "Femto Miner", TFT_ORANGE);
    TDisplayUi::centered(tft, "Press", 50, 3, TFT_WHITE);
    TDisplayUi::centered(tft, "to Start", 80, 2, TFT_LIGHTGREY);
    TDisplayUi::footer(tft, "B1 start");
  } else if (showStartScorePage()) {
    TDisplayUi::header(tft, "Solo Miner", TFT_YELLOW);
    TDisplayUi::centered(tft, "public-pool.io", 49, 2, TFT_CYAN);
    TDisplayUi::centered(tft, "Wallet configurable", 78, 1, TFT_LIGHTGREY);
    TDisplayUi::footer(tft, "Settings via portal");
  } else {
    TDisplayUi::header(tft, "Femto Miner", TFT_ORANGE);
    tft.drawCircle(120, 66, 28, TFT_ORANGE);
    tft.drawCircle(120, 66, 18, TFT_YELLOW);
    tft.drawString("BTC", 104, 59, 2);
    TDisplayUi::footer(tft, "B1 start");
  }
}
