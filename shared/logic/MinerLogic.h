#pragma once

// Femto Miner integrates a small Stratum/SHA mining engine for FemtoDeck.
// The Stratum flow and Bitcoin header hashing approach are adapted from the
// MIT-licensed NerdMiner_v2 project, source revision:
// a26865f7cdd9ac1a81c5b0a7c355e23cf4a1d568.
// See licenses/NerdMiner_v2-MIT.txt for the retained MIT notice.

#include <Arduino.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <mbedtls/sha256.h>

#include "WiFiLogic.h"
#include "nerdSHA256plus.h"

#if defined(CONFIG_IDF_TARGET_ESP32)
#include <hal/sha_ll.h>
#include <sha/sha_parallel_engine.h>
#include <soc/dport_access.h>
#include <soc/hwcrypto_reg.h>
#endif

namespace MinerLogic {

constexpr const char* PREF_NS = "miner";
constexpr const char* DEFAULT_POOL_HOST = "public-pool.io";
constexpr uint16_t DEFAULT_POOL_PORT = 21496;
constexpr const char* DEFAULT_POOL_PASS = "x";
constexpr const char* DEFAULT_WALLET = "bc1qfreyhgyjj03pk60jdpym2tmcx780jmsgcvj8gl";
constexpr const char* SETUP_AP_SSID = "FemtoMiner Setup";
constexpr const char* SETUP_AP_PASS = "femtominer";

enum class State : uint8_t {
  Idle,
  NoWifi,
  ConnectingWifi,
  WifiFailed,
  ConnectingPool,
  PoolFailed,
  Subscribing,
  Mining,
  Stopping,
  Error
};

struct Config {
  String wallet = DEFAULT_WALLET;
  String poolHost = DEFAULT_POOL_HOST;
  String poolPass = DEFAULT_POOL_PASS;
  uint16_t poolPort = DEFAULT_POOL_PORT;
};

struct Stats {
  State state = State::Idle;
  uint32_t hashesPerSecond = 0;
  uint64_t totalHashes = 0;
  uint32_t jobs = 0;
  uint32_t submitted = 0;
  uint32_t accepted = 0;
  uint32_t rejected = 0;
  double bestDifficulty = 0.0;
  double poolDifficulty = 0.00015;
  uint32_t uptimeSeconds = 0;
  char lastError[72] = "";
  char ssid[33] = "";
};

class MinerSettings {
public:
  void load() {
    Preferences prefs;
    prefs.begin(PREF_NS, true);
    config_.wallet = prefs.getString("wallet", DEFAULT_WALLET);
    config_.poolHost = prefs.getString("host", DEFAULT_POOL_HOST);
    config_.poolPass = prefs.getString("pass", DEFAULT_POOL_PASS);
    config_.poolPort = prefs.getUShort("port", DEFAULT_POOL_PORT);
    prefs.end();
    normalize();
  }

  void save(const Config& config) {
    config_ = config;
    normalize();
    Preferences prefs;
    prefs.begin(PREF_NS, false);
    prefs.putString("wallet", config_.wallet);
    prefs.putString("host", config_.poolHost);
    prefs.putString("pass", config_.poolPass);
    prefs.putUShort("port", config_.poolPort);
    prefs.end();
  }

  void reset() {
    Preferences prefs;
    prefs.begin(PREF_NS, false);
    prefs.clear();
    prefs.end();
    config_ = Config();
  }

  const Config& config() const { return config_; }

private:
  void normalize() {
    config_.wallet.trim();
    config_.poolHost.trim();
    config_.poolPass.trim();
    if (config_.wallet.length() == 0) config_.wallet = DEFAULT_WALLET;
    if (config_.poolHost.length() == 0) config_.poolHost = DEFAULT_POOL_HOST;
    if (config_.poolPass.length() == 0) config_.poolPass = DEFAULT_POOL_PASS;
    if (config_.poolPort == 0) config_.poolPort = DEFAULT_POOL_PORT;
  }

  Config config_;
};

inline const char* stateLabel(State state) {
  switch (state) {
    case State::Idle: return "Idle";
    case State::NoWifi: return "No WiFi";
    case State::ConnectingWifi: return "WiFi...";
    case State::WifiFailed: return "WiFi fail";
    case State::ConnectingPool: return "Pool...";
    case State::PoolFailed: return "Pool fail";
    case State::Subscribing: return "Sub...";
    case State::Mining: return "Mining";
    case State::Stopping: return "Stopping";
    case State::Error: return "Error";
  }
  return "?";
}

inline String sanitizedMac() {
  String mac = WiFi.macAddress();
  String clean;
  clean.reserve(12);
  for (uint8_t i = 0; i < mac.length(); i++) {
    const char c = mac[i];
    if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f')) {
      clean += static_cast<char>(toupper(c));
    }
  }
  if (clean.length() == 0) clean = "000000000000";
  return clean;
}

inline String workerName() {
  return String("Femto") + sanitizedMac();
}

inline String stratumUser(const Config& config) {
  return config.wallet + "." + workerName();
}

struct Job {
  String jobId;
  String prevHash;
  String coinb1;
  String coinb2;
  String version;
  String nbits;
  String ntime;
  String branches[16];
  uint8_t branchCount = 0;
  bool cleanJobs = false;
};

struct Subscribe {
  String extranonce1;
  String extranonce2;
  uint8_t extranonce2Size = 4;
};

struct Work {
  Job job;
  Subscribe sub;
  uint8_t header[80] = {};
  uint8_t paddedHeader[128] = {};
  uint32_t midstate[8] = {};
  uint32_t bake[16] = {};
#if defined(CONFIG_IDF_TARGET_ESP32)
  uint32_t hwWords[32] = {};
#endif
  uint32_t nextNonce = 0xDA54E700;
  bool ready = false;
};

class MinerEngine {
public:
  MinerEngine() = default;
  ~MinerEngine() { stop(); }

  bool start(const char* hostname) {
    if (task_ != nullptr) return true;
    settings_.load();
    config_ = settings_.config();
    hostname_ = hostname;
    stopRequested_ = false;
    resetStats();
    setState(State::ConnectingWifi);
    BaseType_t ok = xTaskCreate(taskEntry, "FemtoMiner", 10240, this, 1, &task_);
    if (ok != pdPASS) {
      task_ = nullptr;
      setError("Task create failed");
      return false;
    }
    return true;
  }

  void stop() {
    if (task_ == nullptr) {
      setState(State::Idle);
      return;
    }
    stopRequested_ = true;
    setState(State::Stopping);
    client_.stop();
    const uint32_t started = millis();
    while (task_ != nullptr && millis() - started < 1800) {
      delay(20);
    }
    if (task_ != nullptr) {
      vTaskDelete(task_);
      task_ = nullptr;
    }
    client_.stop();
    WiFi.disconnect(false, false);
    WiFi.mode(WIFI_OFF);
    setState(State::Idle);
  }

  bool running() const { return task_ != nullptr; }

  Stats stats() {
    Stats out;
    portENTER_CRITICAL(&mux_);
    out = stats_;
    portEXIT_CRITICAL(&mux_);
    return out;
  }

  Config config() {
    settings_.load();
    return settings_.config();
  }

  void saveConfig(const Config& config) {
    settings_.save(config);
  }

  void resetConfig() {
    settings_.reset();
  }

private:
  static void taskEntry(void* ctx) {
    static_cast<MinerEngine*>(ctx)->run();
  }

  void run() {
    runWorker();
    client_.stop();
    WiFi.disconnect(false, false);
    WiFi.mode(WIFI_OFF);
    task_ = nullptr;
    vTaskDelete(nullptr);
  }

  void runWorker() {
    if (!connectWifi()) return;
    if (!connectPool()) return;

    Subscribe sub;
    Work work;
    double poolDifficulty = 0.00015;
    uint32_t requestId = 1;
    uint32_t lastRateAt = millis();
    uint64_t lastRateHashes = 0;

    setState(State::Subscribing);
    sendSubscribe(requestId++);
    const uint32_t subStart = millis();
    bool subscribed = false;
    while (!stopRequested_ && client_.connected() && millis() - subStart < 15000) {
      String line;
      if (readLine(line, 250)) {
        parseLine(line, sub, work, poolDifficulty, subscribed);
        if (subscribed) break;
      }
      updateRate(lastRateAt, lastRateHashes);
    }
    if (!subscribed) {
      setError("Subscribe timeout");
      return;
    }

    sendAuth(requestId++);
    sendSuggestDifficulty(requestId++, poolDifficulty);
    setState(State::Mining);

    while (!stopRequested_) {
      if (!client_.connected()) {
        setError("Pool disconnected");
        return;
      }

      while (client_.available()) {
        String line;
        if (!readLine(line, 10)) break;
        parseLine(line, sub, work, poolDifficulty, subscribed);
      }

      if (work.ready) {
        mineBatch(work, poolDifficulty, requestId);
      } else {
        delay(30);
      }
      updateRate(lastRateAt, lastRateHashes);
      vTaskDelay(1);
    }
  }

  bool connectWifi() {
    WiFiLogic wifi;
    wifi.loadProfiles();
    bool sawProfile = false;
    for (uint8_t slot = 0; slot < WiFiLogic::MAX_PROFILES && !stopRequested_; slot++) {
      if (!wifi.isProfileUsed(slot)) continue;
      sawProfile = true;
      const String ssid = wifi.getSsid(slot);
      const String pass = wifi.getPass(slot);
      setState(State::ConnectingWifi);
      setSsid(ssid.c_str());
      WiFi.mode(WIFI_STA);
      WiFi.setSleep(false);
      WiFi.setHostname(hostname_);
      WiFi.disconnect(false, false);
      WiFi.begin(ssid.c_str(), pass.c_str());
      const uint32_t started = millis();
      while (!stopRequested_ && millis() - started < 15000) {
        if (WiFi.status() == WL_CONNECTED) return true;
        delay(150);
      }
    }
    setState(sawProfile ? State::WifiFailed : State::NoWifi);
    return false;
  }

  bool connectPool() {
    setState(State::ConnectingPool);
    if (!client_.connect(config_.poolHost.c_str(), config_.poolPort)) {
      setError("Pool connect failed");
      setState(State::PoolFailed);
      return false;
    }
    return true;
  }

  void sendSubscribe(uint32_t id) {
    client_.printf("{\"id\":%lu,\"method\":\"mining.subscribe\",\"params\":[\"FemtoMiner/2.0\"]}\n",
                   static_cast<unsigned long>(id));
  }

  void sendAuth(uint32_t id) {
    const String user = stratumUser(config_);
    client_.printf("{\"params\":[\"%s\",\"%s\"],\"id\":%lu,\"method\":\"mining.authorize\"}\n",
                   user.c_str(), config_.poolPass.c_str(), static_cast<unsigned long>(id));
  }

  void sendSuggestDifficulty(uint32_t id, double difficulty) {
    client_.printf("{\"id\":%lu,\"method\":\"mining.suggest_difficulty\",\"params\":[%.10g]}\n",
                   static_cast<unsigned long>(id), difficulty);
  }

  void sendSubmit(uint32_t id, const Work& work, uint32_t nonce) {
    char nonceHex[9];
    snprintf(nonceHex, sizeof(nonceHex), "%08lx", static_cast<unsigned long>(nonce));
    const String user = stratumUser(config_);
    client_.printf("{\"id\":%lu,\"method\":\"mining.submit\",\"params\":[\"%s\",\"%s\",\"%s\",\"%s\",\"%s\"]}\n",
                   static_cast<unsigned long>(id), user.c_str(), work.job.jobId.c_str(),
                   work.sub.extranonce2.c_str(), work.job.ntime.c_str(), nonceHex);
    incrementSubmitted();
  }

  bool readLine(String& out, uint32_t timeoutMs) {
    const uint32_t started = millis();
    while (!stopRequested_ && millis() - started < timeoutMs) {
      if (client_.available()) {
        out = client_.readStringUntil('\n');
        out.trim();
        return out.length() > 0;
      }
      delay(2);
    }
    return false;
  }

  void parseLine(const String& line, Subscribe& sub, Work& work, double& poolDifficulty, bool& subscribed) {
    StaticJsonDocument<4096> doc;
    DeserializationError err = deserializeJson(doc, line);
    if (err) return;

    if (doc["id"].is<uint32_t>() && doc["result"].is<bool>()) {
      const bool ok = doc["result"].as<bool>();
      if (doc["id"].as<uint32_t>() >= 4) {
        if (ok) incrementAccepted();
        else incrementRejected();
      }
    }

    const char* method = doc["method"] | "";
    if (strcmp(method, "mining.set_difficulty") == 0) {
      poolDifficulty = doc["params"][0] | poolDifficulty;
      setPoolDifficulty(poolDifficulty);
      return;
    }

    if (strcmp(method, "mining.notify") == 0) {
      JsonArray params = doc["params"].as<JsonArray>();
      if (params.size() < 9) return;
      Job job;
      job.jobId = params[0].as<const char*>();
      job.prevHash = params[1].as<const char*>();
      job.coinb1 = params[2].as<const char*>();
      job.coinb2 = params[3].as<const char*>();
      JsonArray branches = params[4].as<JsonArray>();
      job.branchCount = 0;
      for (JsonVariant branch : branches) {
        if (job.branchCount >= 16) break;
        job.branches[job.branchCount++] = branch.as<const char*>();
      }
      job.version = params[5].as<const char*>();
      job.nbits = params[6].as<const char*>();
      job.ntime = params[7].as<const char*>();
      job.cleanJobs = params[8] | false;
      work.job = job;
      work.sub = sub;
      work.nextNonce = 0xDA54E700 + stats_.jobs * 0x10000UL;
      work.ready = buildWork(work);
      incrementJobs();
      return;
    }

    if (doc["result"].is<JsonArray>()) {
      JsonArray result = doc["result"].as<JsonArray>();
      if (result.size() >= 3) {
        sub.extranonce1 = result[1].as<const char*>();
        sub.extranonce2Size = result[2] | 4;
        sub.extranonce2 = extranonce2ForSize(sub.extranonce2Size);
        subscribed = sub.extranonce1.length() > 0;
      }
    }
  }

  bool buildWork(Work& work) {
    if (work.sub.extranonce1.length() == 0) return false;
    const String coinbase = work.job.coinb1 + work.sub.extranonce1 + work.sub.extranonce2 + work.job.coinb2;
    uint8_t coinbaseBytes[384];
    const size_t coinbaseLen = hexToBytes(coinbase.c_str(), coinbaseBytes, sizeof(coinbaseBytes));
    if (coinbaseLen == 0) return false;

    uint8_t merkle[32];
    doubleSha(coinbaseBytes, coinbaseLen, merkle);
    for (uint8_t i = 0; i < work.job.branchCount; i++) {
      uint8_t branch[32];
      if (hexToBytes(work.job.branches[i].c_str(), branch, sizeof(branch)) != 32) return false;
      uint8_t concat[64];
      memcpy(concat, merkle, 32);
      memcpy(concat + 32, branch, 32);
      doubleSha(concat, sizeof(concat), merkle);
    }

    char merkleHex[65];
    bytesToHex(merkle, 32, merkleHex, sizeof(merkleHex));
    const String headerHex = work.job.version + work.job.prevHash + String(merkleHex) + work.job.ntime + work.job.nbits + "00000000";
    if (hexToBytes(headerHex.c_str(), work.header, sizeof(work.header)) != 80) return false;

    reverseRange(work.header, 0, 4);
    for (uint8_t offset = 4; offset < 36; offset += 4) {
      reverseRange(work.header, offset, 4);
    }
    reverseRange(work.header, 68, 4);
    reverseRange(work.header, 72, 4);
    memcpy(work.paddedHeader, work.header, sizeof(work.header));
    memset(work.paddedHeader + sizeof(work.header), 0, sizeof(work.paddedHeader) - sizeof(work.header));
    work.paddedHeader[80] = 0x80;
    work.paddedHeader[126] = 0x02;
    work.paddedHeader[127] = 0x80;
    nerd_mids(work.midstate, work.paddedHeader);
    nerd_sha256_bake(work.midstate, work.paddedHeader + 64, work.bake);
#if defined(CONFIG_IDF_TARGET_ESP32)
    for (uint8_t i = 0; i < 32; i++) {
      uint32_t word = 0;
      memcpy(&word, work.paddedHeader + i * 4, sizeof(word));
      work.hwWords[i] = __builtin_bswap32(word);
    }
#endif
    return true;
  }

#if defined(CONFIG_IDF_TARGET_ESP32)
  static inline void shaWaitIdleEsp32() {
    while (DPORT_REG_READ(SHA_256_BUSY_REG)) {}
  }

  static inline void shaFillBlockEsp32(const uint32_t* words) {
    uint32_t* reg = reinterpret_cast<uint32_t*>(SHA_TEXT_BASE);
    reg[0] = words[0];
    reg[1] = words[1];
    reg[2] = words[2];
    reg[3] = words[3];
    reg[4] = words[4];
    reg[5] = words[5];
    reg[6] = words[6];
    reg[7] = words[7];
    reg[8] = words[8];
    reg[9] = words[9];
    reg[10] = words[10];
    reg[11] = words[11];
    reg[12] = words[12];
    reg[13] = words[13];
    reg[14] = words[14];
    reg[15] = words[15];
  }

  static inline void shaFillUpperEsp32(const uint32_t* words, uint32_t nonce) {
    uint32_t* reg = reinterpret_cast<uint32_t*>(SHA_TEXT_BASE);
    reg[0] = words[0];
    reg[1] = words[1];
    reg[2] = words[2];
    reg[3] = __builtin_bswap32(nonce);
    reg[4] = 0x80000000;
    reg[5] = 0x00000000;
    reg[6] = 0x00000000;
    reg[7] = 0x00000000;
    reg[8] = 0x00000000;
    reg[9] = 0x00000000;
    reg[10] = 0x00000000;
    reg[11] = 0x00000000;
    reg[12] = 0x00000000;
    reg[13] = 0x00000000;
    reg[14] = 0x00000000;
    reg[15] = 0x00000280;
  }

  static inline void shaFillDoubleEsp32() {
    uint32_t* reg = reinterpret_cast<uint32_t*>(SHA_TEXT_BASE);
    reg[8] = 0x80000000;
    reg[9] = 0x00000000;
    reg[10] = 0x00000000;
    reg[11] = 0x00000000;
    reg[12] = 0x00000000;
    reg[13] = 0x00000000;
    reg[14] = 0x00000000;
    reg[15] = 0x00000100;
  }

  static inline void storeWord(uint8_t* out, uint8_t index, uint32_t value) {
    memcpy(out + index * 4, &value, sizeof(value));
  }

  static inline bool shaReadDigestCandidateEsp32(uint8_t* hash) {
    DPORT_INTERRUPT_DISABLE();
    const uint32_t fin = DPORT_SEQUENCE_REG_READ(SHA_TEXT_BASE + 7 * 4);
    if ((fin & 0xFFFF) != 0) {
      DPORT_INTERRUPT_RESTORE();
      return false;
    }
    storeWord(hash, 7, __builtin_bswap32(fin));
    storeWord(hash, 0, __builtin_bswap32(DPORT_SEQUENCE_REG_READ(SHA_TEXT_BASE + 0 * 4)));
    storeWord(hash, 1, __builtin_bswap32(DPORT_SEQUENCE_REG_READ(SHA_TEXT_BASE + 1 * 4)));
    storeWord(hash, 2, __builtin_bswap32(DPORT_SEQUENCE_REG_READ(SHA_TEXT_BASE + 2 * 4)));
    storeWord(hash, 3, __builtin_bswap32(DPORT_SEQUENCE_REG_READ(SHA_TEXT_BASE + 3 * 4)));
    storeWord(hash, 4, __builtin_bswap32(DPORT_SEQUENCE_REG_READ(SHA_TEXT_BASE + 4 * 4)));
    storeWord(hash, 5, __builtin_bswap32(DPORT_SEQUENCE_REG_READ(SHA_TEXT_BASE + 5 * 4)));
    storeWord(hash, 6, __builtin_bswap32(DPORT_SEQUENCE_REG_READ(SHA_TEXT_BASE + 6 * 4)));
    DPORT_INTERRUPT_RESTORE();
    return true;
  }

  bool mineBatchHardwareEsp32(Work& work, double poolDifficulty, uint32_t& requestId) {
    uint8_t hash[32];
    uint32_t foundNonce = 0;
    double foundDiff = 0.0;
    bool found = false;
    uint32_t processed = 0;
    double localBest = 0.0;
    const uint32_t startNonce = work.nextNonce;
    const uint32_t endNonce = startNonce + 16384UL;

    esp_sha_lock_engine(SHA2_256);
    for (uint32_t nonce = startNonce; nonce != endNonce && !stopRequested_; nonce++) {
      shaFillBlockEsp32(work.hwWords);
      sha_ll_start_block(SHA2_256);
      shaWaitIdleEsp32();
      shaFillUpperEsp32(work.hwWords + 16, nonce);
      sha_ll_continue_block(SHA2_256);
      shaWaitIdleEsp32();
      sha_ll_load(SHA2_256);
      shaWaitIdleEsp32();
      shaFillDoubleEsp32();
      sha_ll_start_block(SHA2_256);
      shaWaitIdleEsp32();
      sha_ll_load(SHA2_256);
      processed++;

      if (shaReadDigestCandidateEsp32(hash)) {
        const double diff = difficultyFromHash(hash);
        if (diff > localBest) localBest = diff;
        if (diff > poolDifficulty) {
          found = true;
          foundNonce = nonce;
          foundDiff = diff;
          break;
        }
      }
    }
    esp_sha_unlock_engine(SHA2_256);

    work.nextNonce = startNonce + processed;
    noteHashes(processed, localBest);
    if (found && client_.connected()) {
      noteBest(foundDiff);
      sendSubmit(requestId++, work, foundNonce);
    }
    return true;
  }
#endif

  void mineBatch(Work& work, double poolDifficulty, uint32_t& requestId) {
#if defined(CONFIG_IDF_TARGET_ESP32)
    if (mineBatchHardwareEsp32(work, poolDifficulty, requestId)) {
      return;
    }
#endif
    uint8_t hash[32];
    uint32_t foundNonce = 0;
    double foundDiff = 0.0;
    bool found = false;
    uint32_t processed = 0;
    double localBest = 0.0;

    for (uint16_t i = 0; i < 4096 && !stopRequested_; i++) {
      const uint32_t nonce = work.nextNonce++;
      memcpy(work.paddedHeader + 76, &nonce, sizeof(nonce));
      processed++;
      if (nerd_sha256d_baked(work.midstate, work.paddedHeader + 64, work.bake, hash)) {
        const double diff = difficultyFromHash(hash);
        if (diff > localBest) localBest = diff;
        if (diff > poolDifficulty) {
          found = true;
          foundNonce = nonce;
          foundDiff = diff;
          break;
        }
      }
    }

    noteHashes(processed, localBest);
    if (found && client_.connected()) {
      noteBest(foundDiff);
      sendSubmit(requestId++, work, foundNonce);
    }
  }

  static String extranonce2ForSize(uint8_t size) {
    if (size == 2) return "0001";
    if (size == 8) return "0000000000000001";
    return "00000001";
  }

  static uint8_t hexNibble(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return 0;
  }

  static size_t hexToBytes(const char* hex, uint8_t* out, size_t maxOut) {
    const size_t len = strlen(hex);
    if (len % 2 != 0) return 0;
    const size_t bytes = len / 2;
    if (bytes > maxOut) return 0;
    for (size_t i = 0; i < bytes; i++) {
      out[i] = static_cast<uint8_t>((hexNibble(hex[i * 2]) << 4) | hexNibble(hex[i * 2 + 1]));
    }
    return bytes;
  }

  static void bytesToHex(const uint8_t* data, size_t len, char* out, size_t outSize) {
    static constexpr char HEX_DIGITS[] = "0123456789abcdef";
    if (outSize < len * 2 + 1) return;
    for (size_t i = 0; i < len; i++) {
      out[i * 2] = HEX_DIGITS[data[i] >> 4];
      out[i * 2 + 1] = HEX_DIGITS[data[i] & 0x0F];
    }
    out[len * 2] = '\0';
  }

  static void reverseRange(uint8_t* data, uint8_t offset, uint8_t len) {
    for (uint8_t i = 0; i < len / 2; i++) {
      const uint8_t tmp = data[offset + i];
      data[offset + i] = data[offset + len - 1 - i];
      data[offset + len - 1 - i] = tmp;
    }
  }

  static void sha256(const uint8_t* data, size_t len, uint8_t out[32]) {
    mbedtls_sha256_context ctx;
    mbedtls_sha256_init(&ctx);
    mbedtls_sha256_starts(&ctx, 0);
    mbedtls_sha256_update(&ctx, data, len);
    mbedtls_sha256_finish(&ctx, out);
    mbedtls_sha256_free(&ctx);
  }

  static void doubleSha(const uint8_t* data, size_t len, uint8_t out[32]) {
    uint8_t first[32];
    sha256(data, len, first);
    sha256(first, sizeof(first), out);
  }

  static double le256ToDouble(const uint8_t* data) {
    const uint64_t part0 = readLe64(data);
    const uint64_t part1 = readLe64(data + 8);
    const uint64_t part2 = readLe64(data + 16);
    const uint64_t part3 = readLe64(data + 24);
    double value = part3 * 6277101735386680763835789423207666416102355444464034512896.0;
    value += part2 * 340282366920938463463374607431768211456.0;
    value += part1 * 18446744073709551616.0;
    value += part0;
    return value;
  }

  static uint64_t readLe64(const uint8_t* data) {
    uint64_t value = 0;
    for (uint8_t i = 0; i < 8; i++) {
      value |= static_cast<uint64_t>(data[i]) << (i * 8);
    }
    return value;
  }

  static double difficultyFromHash(const uint8_t* hash) {
    static constexpr double TRUE_DIFF_ONE = 26959535291011309493156476344723991336010898738574164086137773096960.0;
    double value = le256ToDouble(hash);
    if (value <= 0.0) value = 1.0;
    return TRUE_DIFF_ONE / value;
  }

  void resetStats() {
    portENTER_CRITICAL(&mux_);
    stats_ = Stats();
    stats_.poolDifficulty = 0.00015;
    startedAtMs_ = millis();
    portEXIT_CRITICAL(&mux_);
  }

  void setState(State state) {
    portENTER_CRITICAL(&mux_);
    stats_.state = state;
    if (state != State::Error) stats_.lastError[0] = '\0';
    stats_.uptimeSeconds = startedAtMs_ == 0 ? 0 : (millis() - startedAtMs_) / 1000;
    portEXIT_CRITICAL(&mux_);
  }

  void setError(const char* message) {
    portENTER_CRITICAL(&mux_);
    stats_.state = State::Error;
    strncpy(stats_.lastError, message, sizeof(stats_.lastError) - 1);
    stats_.lastError[sizeof(stats_.lastError) - 1] = '\0';
    portEXIT_CRITICAL(&mux_);
  }

  void setSsid(const char* ssid) {
    portENTER_CRITICAL(&mux_);
    strncpy(stats_.ssid, ssid, sizeof(stats_.ssid) - 1);
    stats_.ssid[sizeof(stats_.ssid) - 1] = '\0';
    portEXIT_CRITICAL(&mux_);
  }

  void setPoolDifficulty(double diff) {
    portENTER_CRITICAL(&mux_);
    stats_.poolDifficulty = diff;
    portEXIT_CRITICAL(&mux_);
  }

  void incrementJobs() {
    portENTER_CRITICAL(&mux_);
    stats_.jobs++;
    portEXIT_CRITICAL(&mux_);
  }

  void incrementSubmitted() {
    portENTER_CRITICAL(&mux_);
    stats_.submitted++;
    portEXIT_CRITICAL(&mux_);
  }

  void incrementAccepted() {
    portENTER_CRITICAL(&mux_);
    stats_.accepted++;
    portEXIT_CRITICAL(&mux_);
  }

  void incrementRejected() {
    portENTER_CRITICAL(&mux_);
    stats_.rejected++;
    portEXIT_CRITICAL(&mux_);
  }

  void noteHash(double diff) {
    portENTER_CRITICAL(&mux_);
    stats_.totalHashes++;
    if (diff > stats_.bestDifficulty) stats_.bestDifficulty = diff;
    stats_.uptimeSeconds = (millis() - startedAtMs_) / 1000;
    portEXIT_CRITICAL(&mux_);
  }

  void noteHashes(uint32_t count, double bestDiff) {
    if (count == 0) return;
    portENTER_CRITICAL(&mux_);
    stats_.totalHashes += count;
    if (bestDiff > stats_.bestDifficulty) stats_.bestDifficulty = bestDiff;
    stats_.uptimeSeconds = (millis() - startedAtMs_) / 1000;
    portEXIT_CRITICAL(&mux_);
  }

  void noteBest(double diff) {
    portENTER_CRITICAL(&mux_);
    if (diff > stats_.bestDifficulty) stats_.bestDifficulty = diff;
    portEXIT_CRITICAL(&mux_);
  }

  void updateRate(uint32_t& lastRateAt, uint64_t& lastRateHashes) {
    const uint32_t now = millis();
    if (now - lastRateAt < 1000) return;
    portENTER_CRITICAL(&mux_);
    const uint64_t total = stats_.totalHashes;
    const uint32_t elapsed = now - lastRateAt;
    stats_.hashesPerSecond = elapsed == 0 ? 0 : static_cast<uint32_t>((total - lastRateHashes) * 1000ULL / elapsed);
    stats_.uptimeSeconds = (now - startedAtMs_) / 1000;
    portEXIT_CRITICAL(&mux_);
    lastRateAt = now;
    lastRateHashes = total;
  }

  MinerSettings settings_;
  Config config_;
  const char* hostname_ = "FemtoDeck";
  WiFiClient client_;
  TaskHandle_t task_ = nullptr;
  volatile bool stopRequested_ = false;
  uint32_t startedAtMs_ = 0;
  Stats stats_;
  portMUX_TYPE mux_ = portMUX_INITIALIZER_UNLOCKED;
};

}  // namespace MinerLogic
