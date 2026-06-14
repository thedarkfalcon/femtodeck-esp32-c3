#include "MouseEmulatorApp.h"
#include <U8g2lib.h>
#include <esp_system.h>
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

MouseEmulatorApp::MouseEmulatorApp(uint32_t width, uint32_t height, uint32_t left)
    : App("Mouse Emulator", width, height) { (void)left; }

void MouseEmulatorApp::onAppReset() {
  logic_.reset(millis());
  bleMouse.begin();
  lastClockUpdate_ = millis();
}

void MouseEmulatorApp::updateRunning(uint32_t deltaMs, const ButtonInput& input) {
  if (input.click) logic_.toggleEnabled(millis());
  if (input.longPress) { requestExitToMenu(); return; }

  uint32_t now = millis();
  bool connected = bleMouse.isConnected();

  if (logic_.shouldStartMove(now, connected)) {
    logic_.startMove(now, 500, 5000);
  } else if (logic_.getPhase() == MouseEmulatorLogic::Forward) {
    if (!connected) { logic_.setPhase(MouseEmulatorLogic::Idle); logic_.setMoving(false); logic_.setLastJiggleTime(now); }
    else {
      int steps = min(stepsPerTick_, logic_.remainingSteps());
      for (int i = 0; i < steps; ++i) sendHumanizedStep();
      logic_.remainingSteps() -= steps;
      if (logic_.remainingSteps() <= 0) { logic_.setWaitUntil(now + random(400, 1200)); logic_.setPhase(MouseEmulatorLogic::Waiting); }
    }
  } else if (logic_.getPhase() == MouseEmulatorLogic::Waiting) {
    if (now >= logic_.getWaitUntil()) {
      logic_.setPhase(MouseEmulatorLogic::Back);
      logic_.remainingSteps() = logic_.getLastMovementPixels();
      logic_.movementDirection() = -1;
    }
  } else if (logic_.getPhase() == MouseEmulatorLogic::Back) {
    if (!connected) { logic_.setPhase(MouseEmulatorLogic::Idle); logic_.setMoving(false); logic_.setLastJiggleTime(now); }
    else {
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
  if (!logic_.isMoving() && (now - lastClockUpdate_ >= 1000)) lastClockUpdate_ = now;
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

void MouseEmulatorApp::drawRunning(U8G2& u8g2) {
  u8g2.drawFrame(0, 0, width + 2, height);
  u8g2.setFont(u8g2_font_4x6_tr);
  u8g2.drawStr(3, 9, "Tap=Toggle");
  const char* conn = bleMouse.isConnected() ? "PAIRED" : "PAIRING";
  const char* state = logic_.isEnabled() ? "ACTIVE" : "PAUSED";
  u8g2.drawStr(3, 24, state);
  int connW = u8g2.getStrWidth(conn);
  u8g2.drawStr(width + 2 - connW - 3, 24, conn);
  if (bleMouse.isConnected() && logic_.isEnabled() && !logic_.isMoving() && logic_.getTargetIntervalMs() != 0) {
    uint32_t elapsed = millis() - logic_.getLastJiggleTime();
    uint8_t w = (uint8_t)min((uint32_t)width, (elapsed * (uint32_t)width) / logic_.getTargetIntervalMs());
    u8g2.drawBox(1, 36, w, 3);
  }
  const char* holdText = "Hold=Menu";
  int holdW = u8g2.getStrWidth(holdText);
  u8g2.drawStr(width + 2 - holdW - 3, 36, holdText);
}

void MouseEmulatorApp::drawStart(U8G2& u8g2) {
  u8g2.drawFrame(0, 0, width + 2, height);
  u8g2.setFont(u8g2_font_5x8_tr);
  u8g2.drawStr(3, 12, "Mouse Emulator");
  u8g2.setFont(u8g2_font_4x6_tr);
  u8g2.drawStr(3, 26, "Tap to toggle");
  u8g2.drawStr(3, 36, "Hold=menu");
}

bool MouseEmulatorApp::hasCustomOverlay() const { return true; }
