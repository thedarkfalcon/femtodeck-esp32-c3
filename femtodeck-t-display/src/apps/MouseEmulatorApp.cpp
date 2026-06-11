#include "MouseEmulatorApp.h"
#include <TFT_eSPI.h>
#include <NimBLEDevice.h>
#include <NimBLEHIDDevice.h>
#include <NimBLEServer.h>
#include <HIDTypes.h>

namespace {
constexpr const char* DEVICE_NAME = "Logitech Signature M650";
constexpr const char* DEVICE_MANUFACTURER = "Logitech";
constexpr int VERTICAL_WIGGLE_LIMIT = 20;
constexpr int VERTICAL_WIGGLE_MIN_STEPS = 8;
constexpr int VERTICAL_WIGGLE_MAX_STEPS = 28;

const uint8_t HID_REPORT_DESCRIPTOR[] = {
    USAGE_PAGE(1), 0x01, USAGE(1), 0x02, COLLECTION(1), 0x01, USAGE(1), 0x01, COLLECTION(1), 0x00,
    REPORT_ID(1), 0x01, USAGE_PAGE(1), 0x09, USAGE_MINIMUM(1), 0x01, USAGE_MAXIMUM(1), 0x05,
    LOGICAL_MINIMUM(1), 0x00, LOGICAL_MAXIMUM(1), 0x01, REPORT_SIZE(1), 0x01, REPORT_COUNT(1), 0x05,
    HIDINPUT(1), 0x02, REPORT_SIZE(1), 0x03, REPORT_COUNT(1), 0x01, HIDINPUT(1), 0x03,
    USAGE_PAGE(1), 0x01, USAGE(1), 0x30, USAGE(1), 0x31, USAGE(1), 0x38,
    LOGICAL_MINIMUM(1), 0x81, LOGICAL_MAXIMUM(1), 0x7f, REPORT_SIZE(1), 0x08, REPORT_COUNT(1), 0x03,
    HIDINPUT(1), 0x06, USAGE_PAGE(1), 0x0c, USAGE(2), 0x38, 0x02,
    LOGICAL_MINIMUM(1), 0x81, LOGICAL_MAXIMUM(1), 0x7f, REPORT_SIZE(1), 0x08, REPORT_COUNT(1), 0x01,
    HIDINPUT(1), 0x06, END_COLLECTION(0), END_COLLECTION(0)
};

class NimbleMouse : public NimBLEServerCallbacks {
  public:
    void begin() {
      if (started_) return;
      NimBLEDevice::init(DEVICE_NAME);
      server_ = NimBLEDevice::createServer();
      server_->setCallbacks(this, false);
      hid_ = new NimBLEHIDDevice(server_);
      inputMouse_ = hid_->getInputReport(1);
      hid_->setManufacturer(DEVICE_MANUFACTURER);
      hid_->setReportMap((uint8_t*)HID_REPORT_DESCRIPTOR, sizeof(HID_REPORT_DESCRIPTOR));
      server_->start();
      NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
      adv->setAppearance(HID_MOUSE);
      adv->addServiceUUID(hid_->getHidService()->getUUID());
      adv->start();
      started_ = true;
    }
    bool isConnected() const { return started_ && connected_ && server_ && server_->getConnectedCount() > 0; }
    void move(signed char x, signed char y) {
      if (!isConnected()) return;
      uint8_t report[] = {0, (uint8_t)x, (uint8_t)y, 0, 0};
      inputMouse_->notify(report, sizeof(report));
    }
  private:
    void onConnect(NimBLEServer* s, NimBLEConnInfo& c) override { connected_ = true; }
    void onDisconnect(NimBLEServer* s, NimBLEConnInfo& c, int r) override { connected_ = false; }
    bool started_ = false, connected_ = false;
    NimBLEServer* server_ = nullptr;
    NimBLEHIDDevice* hid_ = nullptr;
    NimBLECharacteristic* inputMouse_ = nullptr;
};
NimbleMouse bleMouse;
}

MouseEmulatorApp::MouseEmulatorApp(uint32_t width, uint32_t height)
    : App("Mouse Emulator", width, height) {}

void MouseEmulatorApp::onAppReset() {
  logic_.reset(millis());
  bleMouse.begin();
  lastClockUpdate_ = millis();
}

void MouseEmulatorApp::updateRunning(uint32_t deltaMs, const ButtonInput& b1, const ButtonInput& b2) {
  if (b1.click) logic_.toggleEnabled(millis());

  uint32_t now = millis();
  bool connected = bleMouse.isConnected();

  if (logic_.shouldStartMove(now, connected)) {
    logic_.startMove(now, 500, 5000);
  } else if (logic_.getPhase() == MouseEmulatorLogic::Forward) {
    if (!connected) {
      logic_.setPhase(MouseEmulatorLogic::Idle);
      logic_.setMoving(false);
      logic_.setLastJiggleTime(now);
    } else {
      int steps = min(stepsPerTick_, logic_.remainingSteps());
      for (int i = 0; i < steps; ++i) sendHumanizedStep();
      logic_.remainingSteps() -= steps;
      if (logic_.remainingSteps() <= 0) {
        logic_.setWaitUntil(now + random(400, 1200));
        logic_.setPhase(MouseEmulatorLogic::Waiting);
      }
    }
  } else if (logic_.getPhase() == MouseEmulatorLogic::Waiting) {
    if (now >= logic_.getWaitUntil()) {
      logic_.setPhase(MouseEmulatorLogic::Back);
      logic_.remainingSteps() = logic_.getLastMovementPixels();
      logic_.movementDirection() = -1;
    }
  } else if (logic_.getPhase() == MouseEmulatorLogic::Back) {
    if (!connected) {
        logic_.setPhase(MouseEmulatorLogic::Idle);
        logic_.setMoving(false);
        logic_.setLastJiggleTime(now);
    } else {
      int steps = min(stepsPerTick_, logic_.remainingSteps());
      for (int i = 0; i < steps; ++i) sendHumanizedStep();
      logic_.remainingSteps() -= steps;
      if (logic_.remainingSteps() <= 0) {
        logic_.setPhase(MouseEmulatorLogic::Idle);
        logic_.setMoving(false);
        logic_.setLastJiggleTime(now);
        logic_.calculateNextInterval();
      }
    }
  }

  if (!logic_.isMoving() && (now - lastClockUpdate_ >= 1000)) {
    lastClockUpdate_ = now;
  }
}

void MouseEmulatorApp::sendHumanizedStep() {
  int y = 0;
  logic_.verticalJitterCountdown()--;
  if (logic_.verticalJitterCountdown() <= 0) {
    if (logic_.verticalOffset() >= VERTICAL_WIGGLE_LIMIT) y = -1;
    else if (logic_.verticalOffset() <= -VERTICAL_WIGGLE_LIMIT) y = 1;
    else y = random(0, 2) == 0 ? -1 : 1;
    logic_.verticalOffset() += y;
    logic_.verticalJitterCountdown() = random(VERTICAL_WIGGLE_MIN_STEPS, VERTICAL_WIGGLE_MAX_STEPS);
  }
  bleMouse.move((signed char)logic_.movementDirection(), (signed char)y);
  delayMicroseconds(200);
}

void MouseEmulatorApp::drawRunning(TFT_eSPI& tft) {
  // Initial draw or major state change
  if (lastUpdateMs_ == phaseStartedAtMs_) {
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_ORANGE, TFT_BLACK);
    tft.setTextSize(2);
    tft.drawString("\"Logitech Mouse\"", 10, 12);
    tft.drawFastHLine(10, 34, 220, TFT_DARKGREY);
    tft.setTextSize(1);
    tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    tft.drawString("Host Connection: ", 10, 45);
    tft.drawString("Jiggler Core: ", 10, 60);
    tft.drawString("Next Sweep In: ", 10, 75);
    tft.drawString("Last Run Scale: ", 10, 90);
  }
  updateDynamicStats(tft);
}

void MouseEmulatorApp::updateDynamicStats(TFT_eSPI& tft) {
  tft.setTextSize(1);
  bool currentConn = bleMouse.isConnected();
  bool enabled = logic_.isEnabled();

  // Connection Status
  if (currentConn) {
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    if ((millis() / 5000) % 2 == 0) tft.drawString("PAIRED           ", 115, 45);
    else tft.drawString("READY            ", 115, 45);
  } else {
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.drawString("WAITING - PAIRING", 115, 45);
  }

  // Jiggler Status
  if (enabled != lastKnownEnabled_) {
    tft.fillRect(115, 60, 125, 12, TFT_BLACK);
    if (enabled) {
      tft.setTextColor(TFT_GREEN, TFT_BLACK);
      tft.drawString("ACTIVE", 115, 60);
    } else {
      tft.setTextColor(TFT_RED, TFT_BLACK);
      tft.drawString("PAUSED", 115, 60);
    }
    lastKnownEnabled_ = enabled;
  }

  // Countdown
  if (!currentConn || !enabled) {
    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.drawString("STANDBY                 ", 115, 75);
  } else if (logic_.isMoving()) {
    tft.setTextColor(TFT_ORANGE, TFT_BLACK);
    tft.drawString("0s (SWEEPING...)        ", 115, 75);
  } else {
    uint32_t elapsed = millis() - logic_.getLastJiggleTime();
    long remaining = 0;
    if (logic_.getTargetIntervalMs() > elapsed) remaining = (logic_.getTargetIntervalMs() - elapsed) / 1000;
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString(String(remaining) + " seconds               ", 115, 75);
  }

  // Last movement
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  if (logic_.getLastMovementPixels() > 0) tft.drawString(String(logic_.getLastMovementPixels()) + " px (H-Vector)  ", 115, 90);
  else tft.drawString("No runs logged yet       ", 115, 90);
}

void MouseEmulatorApp::drawStart(TFT_eSPI& tft) {
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_ORANGE, TFT_BLACK);
    tft.setTextSize(2);
    tft.setCursor(10, 40);
    tft.println("Mouse Emulator");
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(10, 80);
    tft.println("Press Button 1 to Start");
}

bool MouseEmulatorApp::hasCustomOverlay() const { return true; }
