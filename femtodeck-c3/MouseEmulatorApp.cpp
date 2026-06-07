#include "MouseEmulatorApp.h"

#include <U8g2lib.h>
#include <esp_system.h>
#include <NimBLEAdvertising.h>
#include <NimBLECharacteristic.h>
#include <NimBLEDevice.h>
#include <NimBLEHIDDevice.h>
#include <NimBLEServer.h>
#include <NimBLEService.h>
#include <HIDTypes.h>

namespace {

constexpr const char* DEVICE_NAME = "Logitech Signature M650";
constexpr const char* DEVICE_MANUFACTURER = "Logitech";
constexpr int VERTICAL_WIGGLE_LIMIT = 20;
constexpr int VERTICAL_WIGGLE_FIRST_MIN_STEPS = 8;
constexpr int VERTICAL_WIGGLE_FIRST_MAX_STEPS = 22;
constexpr int VERTICAL_WIGGLE_MIN_STEPS = 8;
constexpr int VERTICAL_WIGGLE_MAX_STEPS = 28;

const uint8_t HID_REPORT_DESCRIPTOR[] = {
    USAGE_PAGE(1), 0x01,
    USAGE(1), 0x02,
    COLLECTION(1), 0x01,
    USAGE(1), 0x01,
    COLLECTION(1), 0x00,
    REPORT_ID(1), 0x01,
    USAGE_PAGE(1), 0x09,
    USAGE_MINIMUM(1), 0x01,
    USAGE_MAXIMUM(1), 0x05,
    LOGICAL_MINIMUM(1), 0x00,
    LOGICAL_MAXIMUM(1), 0x01,
    REPORT_SIZE(1), 0x01,
    REPORT_COUNT(1), 0x05,
    HIDINPUT(1), 0x02,
    REPORT_SIZE(1), 0x03,
    REPORT_COUNT(1), 0x01,
    HIDINPUT(1), 0x03,
    USAGE_PAGE(1), 0x01,
    USAGE(1), 0x30,
    USAGE(1), 0x31,
    USAGE(1), 0x38,
    LOGICAL_MINIMUM(1), 0x81,
    LOGICAL_MAXIMUM(1), 0x7f,
    REPORT_SIZE(1), 0x08,
    REPORT_COUNT(1), 0x03,
    HIDINPUT(1), 0x06,
    USAGE_PAGE(1), 0x0c,
    USAGE(2), 0x38, 0x02,
    LOGICAL_MINIMUM(1), 0x81,
    LOGICAL_MAXIMUM(1), 0x7f,
    REPORT_SIZE(1), 0x08,
    REPORT_COUNT(1), 0x01,
    HIDINPUT(1), 0x06,
    END_COLLECTION(0),
    END_COLLECTION(0)
};

class NimbleMouse : public NimBLEServerCallbacks {
  public:
    void begin() {
      if (started_) {
        return;
      }

      NimBLEDevice::init(DEVICE_NAME);
      NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);
      NimBLEDevice::setSecurityAuth(true, false, true);

      server_ = NimBLEDevice::createServer();
      server_->setCallbacks(this, false);
      server_->advertiseOnDisconnect(true);

      hid_ = new NimBLEHIDDevice(server_);
      inputMouse_ = hid_->getInputReport(1);
      hid_->setManufacturer(DEVICE_MANUFACTURER);
      hid_->setPnp(0x02, 0xe502, 0xa111, 0x0210);
      hid_->setHidInfo(0x00, 0x02);
      hid_->setReportMap((uint8_t*)HID_REPORT_DESCRIPTOR, sizeof(HID_REPORT_DESCRIPTOR));
      hid_->setBatteryLevel(100);

      server_->start();

      NimBLEAdvertising* advertising = NimBLEDevice::getAdvertising();
      advertising->setAppearance(HID_MOUSE);
      advertising->addServiceUUID(hid_->getHidService()->getUUID());
      advertising->setName(DEVICE_NAME);
      advertising->enableScanResponse(true);
      advertising->start();

      started_ = true;
    }

    bool isConnected() const {
      return started_ && connected_ && server_ != nullptr && server_->getConnectedCount() > 0;
    }

    void move(signed char x, signed char y, signed char wheel = 0, signed char hWheel = 0) {
      if (!isConnected() || inputMouse_ == nullptr) {
        return;
      }

      uint8_t report[] = {0, (uint8_t)x, (uint8_t)y, (uint8_t)wheel, (uint8_t)hWheel};
      inputMouse_->notify(report, sizeof(report));
    }

  private:
    void onConnect(NimBLEServer* server, NimBLEConnInfo& connInfo) override {
      connected_ = true;
      server->updateConnParams(connInfo.getConnHandle(), 24, 48, 0, 180);
    }

    void onDisconnect(NimBLEServer* server, NimBLEConnInfo& connInfo, int reason) override {
      (void)server;
      (void)connInfo;
      (void)reason;
      connected_ = false;
      NimBLEDevice::startAdvertising();
    }

    bool started_ = false;
    bool connected_ = false;
    NimBLEServer* server_ = nullptr;
    NimBLEHIDDevice* hid_ = nullptr;
    NimBLECharacteristic* inputMouse_ = nullptr;
};

NimbleMouse bleMouse;

}  // namespace

MouseEmulatorApp::MouseEmulatorApp(uint32_t width, uint32_t height, uint32_t left)
    : App("Mouse Emulator", width, height), targetIntervalMs_(0), lastJiggleTime_(0), lastClockUpdate_(0), jigglerEnabled_(true), isMoving_(false) {
  (void)left;
}

void MouseEmulatorApp::onAppReset() {
  // initialize random seed and BLE
  randomSeed(esp_random());
  targetIntervalMs_ = (random(10, 51) * 1000);
  lastJiggleTime_ = millis();
  lastClockUpdate_ = millis();
  jigglerEnabled_ = true;
  isMoving_ = false;
  phase_ = Idle;
  movementTotal_ = 0;
  remainingSteps_ = 0;
  movementDirection_ = 1;
  waitUntilMs_ = 0;
  verticalOffset_ = 0;
  verticalJitterCountdown_ = 0;
  bleMouse.begin();
}

// Non-blocking movement handled in updateRunning via phase_ / remainingSteps_

void MouseEmulatorApp::updateRunning(uint32_t deltaMs, const ButtonInput& input) {
  (void)deltaMs;
  // click toggles jiggler
  if (input.click) {
    jigglerEnabled_ = !jigglerEnabled_;
    if (jigglerEnabled_) {
      lastJiggleTime_ = millis();
      targetIntervalMs_ = (random(10, 51) * 1000);
    }
  }

  // long press returns to menu
  if (input.longPress) {
    requestExitToMenu();
    return;
  }

  uint32_t now = millis();

  // Non-blocking movement state machine
  if (bleMouse.isConnected() && jigglerEnabled_) {
    if (phase_ == Idle) {
      if ((now - lastJiggleTime_) >= targetIntervalMs_) {
        movementTotal_ = random(500, 5000);
        remainingSteps_ = movementTotal_;
        movementDirection_ = 1;
        verticalOffset_ = 0;
        verticalJitterCountdown_ = random(VERTICAL_WIGGLE_FIRST_MIN_STEPS, VERTICAL_WIGGLE_FIRST_MAX_STEPS);
        phase_ = Forward;
        isMoving_ = true;
      }
    } else if (phase_ == Forward) {
      if (!bleMouse.isConnected()) {
        // abort
        phase_ = Idle;
        isMoving_ = false;
        lastJiggleTime_ = now;
      } else {
        int steps = min(stepsPerTick_, remainingSteps_);
        for (int i = 0; i < steps; ++i) {
          sendHumanizedStep();
        }
        remainingSteps_ -= steps;
        if (remainingSteps_ <= 0) {
          waitUntilMs_ = now + (uint32_t)random(400, 1200);
          phase_ = Waiting;
        }
      }
    } else if (phase_ == Waiting) {
      if (now >= waitUntilMs_) {
        phase_ = Back;
        remainingSteps_ = movementTotal_;
        movementDirection_ = -1;
      }
    } else if (phase_ == Back) {
      if (!bleMouse.isConnected()) {
        phase_ = Idle;
        isMoving_ = false;
        lastJiggleTime_ = now;
      } else {
        int steps = min(stepsPerTick_, remainingSteps_);
        for (int i = 0; i < steps; ++i) {
          sendHumanizedStep();
        }
        remainingSteps_ -= steps;
        if (remainingSteps_ <= 0) {
          phase_ = Idle;
          isMoving_ = false;
          lastJiggleTime_ = now;
          targetIntervalMs_ = (random(10, 51) * 1000);
        }
      }
    }
  }

  // update internal clock for display
  if (!isMoving_ && (now - lastClockUpdate_ >= 1000)) {
    lastClockUpdate_ = now;
  }
}

void MouseEmulatorApp::sendHumanizedStep() {
  int y = 0;

  verticalJitterCountdown_--;
  if (verticalJitterCountdown_ <= 0) {
    if (verticalOffset_ >= VERTICAL_WIGGLE_LIMIT) {
      y = -1;
    } else if (verticalOffset_ <= -VERTICAL_WIGGLE_LIMIT) {
      y = 1;
    } else {
      y = random(0, 2) == 0 ? -1 : 1;
    }

    verticalOffset_ += y;
    verticalJitterCountdown_ = random(VERTICAL_WIGGLE_MIN_STEPS, VERTICAL_WIGGLE_MAX_STEPS);
  }

  bleMouse.move((signed char)movementDirection_, (signed char)y, 0);
  delayMicroseconds(200);
}

void MouseEmulatorApp::drawRunning(U8G2& u8g2) {
  u8g2.drawFrame(0, 0, width + 2, height);

  // Top hint
  u8g2.setFont(u8g2_font_4x6_tr);
  u8g2.drawStr(3, 9, "Tap=Toggle");

  // State and connection status
  const char* conn = bleMouse.isConnected() ? "PAIRED" : "PAIRING";
  const char* state = jigglerEnabled_ ? "ACTIVE" : "PAUSED";
  u8g2.drawStr(3, 24, state);
  int connW = u8g2.getStrWidth(conn);
  int connX = (int)(width + 2 - connW - 3);
  if (connX < 3) connX = 3;
  u8g2.drawStr(connX, 24, conn);

  // progress bar
  u8g2.drawBox(1, 36, 0, 3); // clear area
  if (bleMouse.isConnected() && jigglerEnabled_ && !isMoving_ && targetIntervalMs_ != 0) {
    uint32_t elapsed = millis() - lastJiggleTime_;
    uint8_t w = (uint8_t)min((uint32_t)width, (elapsed * (uint32_t)width) / targetIntervalMs_);
    u8g2.drawBox(1, 36, w, 3);
  }

  // bottom-right hold hint, right-aligned to avoid clipping
  const char* holdText = "Hold=Menu";
  int holdW = u8g2.getStrWidth(holdText);
  int holdX = (int)(width + 2 - holdW - 3);
  if (holdX < 3) holdX = 3;
  u8g2.drawStr(holdX, 36, holdText);
}

void MouseEmulatorApp::drawStart(U8G2& u8g2) {
  u8g2.drawFrame(0, 0, width + 2, height);
  u8g2.setFont(u8g2_font_5x8_tr);
  u8g2.drawStr(3, 12, "Mouse Emulator");
  u8g2.setFont(u8g2_font_4x6_tr);
  u8g2.drawStr(3, 26, "Tap to toggle");
  u8g2.drawStr(3, 36, "Hold=menu");
}

bool MouseEmulatorApp::hasCustomOverlay() const {
  return true;
}
