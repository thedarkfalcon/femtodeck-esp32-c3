#pragma once

#include <Arduino.h>
#include <stdint.h>
#include <esp_now.h>

#include "../../App.h"

class CommunicatorApp : public App {
  public:
    struct DictNode {
      const char* label;
      const DictNode* children;
      uint8_t childCount;
    };

    CommunicatorApp(uint32_t width, uint32_t height, uint32_t left);

    bool hasCustomOverlay() const override;

  protected:
    void onAppReset() override;
    void updateRunning(uint32_t deltaMs, const ButtonInput& input) override;
    void drawRunning(U8G2& u8g2) override;
    bool startsRunningImmediately() const override;

  private:
    static constexpr uint8_t MAX_PATH = 4;
    static constexpr uint8_t INBOX_SIZE = 8;
    static constexpr uint8_t NO_MESSAGE = 255;

    struct MessagePacket {
      uint8_t magic0;
      uint8_t magic1;
      uint8_t version;
      uint8_t length;
      uint8_t path[MAX_PATH];
      uint32_t nonce;
    } __attribute__((packed));

    struct InboxItem {
      MessagePacket packet;
      uint8_t from[6];
      uint32_t receivedMs;
      bool unread;
    };

    enum class UiMode {
      Send,
      Inbox
    };

    enum class SendFeedback {
      None,
      Sending,
      Sent,
      Failed
    };

    bool beginRadio();
    void shutdownRadio();
    bool sendPacket(const MessagePacket& packet);
    void sendCurrentPath(uint8_t length);
    void handleTap();
    void handleHold();
    void handleDoubleTap();
    void handleIncoming(const uint8_t* from, const uint8_t* data, int len);
    void handleSendStatus(bool success);
    void pushInbox(const MessagePacket& packet, const uint8_t* from);
    bool packetIsValid(const MessagePacket& packet) const;
    const DictNode* nodeAtPath(const uint8_t* path, uint8_t length) const;
    const DictNode* currentSendList(uint8_t& count) const;
    const DictNode* selectedSendNode() const;
    uint8_t selectedNodeIndex() const;
    const char* packetLeafLabel(const MessagePacket& packet) const;
    const char* nodeLabelAt(const MessagePacket& packet, uint8_t index) const;
    void copyMac(uint8_t* dest, const uint8_t* src) const;
    bool selectedIsRepeat() const;
    uint8_t rootVisibleCount() const;
    void resetClickState();
    void drawHeader(U8G2& u8g2, const char* title) const;
    void drawSend(U8G2& u8g2);
    void drawInbox(U8G2& u8g2);
    void drawOpenMessage(U8G2& u8g2);
    void drawFeedback(U8G2& u8g2);
    void drawFit(U8G2& u8g2, int x, int y, const char* text, uint8_t maxChars) const;

    static void onReceiveStatic(const esp_now_recv_info_t* info, const uint8_t* data, int len);
    static void onSentStatic(const esp_now_send_info_t* info, esp_now_send_status_t status);

    uint32_t left_;
    UiMode uiMode_ = UiMode::Send;
    SendFeedback feedback_ = SendFeedback::None;
    InboxItem inbox_[INBOX_SIZE] = {};
    MessagePacket lastSent_ = {};
    uint8_t sendPath_[MAX_PATH] = {};
    uint8_t sendDepth_ = 0;
    uint8_t sendIndex_ = 0;
    uint8_t inboxIndex_ = 0;
    uint8_t openInboxIndex_ = NO_MESSAGE;
    uint8_t inboxCount_ = 0;
    bool hasLastSent_ = false;
    bool radioReady_ = false;
    bool clickPending_ = false;
    bool sendStatusPending_ = false;
    bool sendStatusSuccess_ = false;
    uint32_t clickPendingAtMs_ = 0;
    uint32_t feedbackMs_ = 0;
    uint32_t nonce_ = 1;
};
