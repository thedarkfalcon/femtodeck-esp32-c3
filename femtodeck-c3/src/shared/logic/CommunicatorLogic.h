#pragma once

#include <Arduino.h>
#include <stdint.h>
#include <esp_now.h>

class CommunicatorLogic {
public:
    static constexpr uint8_t MAX_PATH = 4;
    static constexpr uint8_t INBOX_SIZE = 8;
    static constexpr uint8_t COMM_MAGIC_0 = 'F';
    static constexpr uint8_t COMM_MAGIC_1 = 'C';
    static constexpr uint8_t COMM_VERSION = 1;

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

    struct DictNode {
      const char* label;
      const DictNode* children;
      uint8_t childCount;
    };

    CommunicatorLogic() : inboxCount_(0), nonce_(1) {}

    bool packetIsValid(const MessagePacket& packet, const DictNode* categories, uint8_t categoryCount) const {
      return packet.magic0 == COMM_MAGIC_0 && packet.magic1 == COMM_MAGIC_1 && packet.version == COMM_VERSION &&
             packet.length > 0 && packet.length <= MAX_PATH &&
             nodeAtPath(packet.path, packet.length, categories, categoryCount) != nullptr;
    }

    const DictNode* nodeAtPath(const uint8_t* path, uint8_t length, const DictNode* categories, uint8_t categoryCount) const {
      if (length == 0 || path[0] >= categoryCount) return nullptr;
      const DictNode* node = &categories[path[0]];
      for (uint8_t i = 1; i < length; i++) {
        if (node->children == nullptr || path[i] >= node->childCount) return nullptr;
        node = &node->children[path[i]];
      }
      return node;
    }

    void pushInbox(const MessagePacket& packet, const uint8_t* from) {
      if (inboxCount_ < INBOX_SIZE) inboxCount_++;
      for (int8_t i = inboxCount_ - 1; i > 0; i--) inbox_[i] = inbox_[i - 1];
      inbox_[0].packet = packet;
      memcpy(inbox_[0].from, from, 6);
      inbox_[0].receivedMs = millis();
      inbox_[0].unread = true;
    }

    uint8_t getInboxCount() const { return inboxCount_; }
    InboxItem& getInboxItem(uint8_t index) { return inbox_[index]; }
    uint32_t nextNonce() { return nonce_++; }

private:
    InboxItem inbox_[INBOX_SIZE];
    uint8_t inboxCount_;
    uint32_t nonce_;
};
