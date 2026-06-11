#include "FemtoFieldGame.h"

#include <Arduino.h>
#include <Preferences.h>
#include <TFT_eSPI.h>

#include "../../PlayerProfile.h"

namespace {
constexpr uint8_t FIELD_LED_PIN = 8;
constexpr bool FIELD_LED_ACTIVE_LOW = true;
constexpr uint8_t EVENT_COUNT = 6;
constexpr uint16_t EXTRA_LIFE_POINTS = 1000;
constexpr uint16_t RESULT_LOCK_MS = 650;
constexpr uint8_t STARTING_ATTEMPT_BANK = 5;
constexpr uint16_t THROW_ANIM_MS = 1400;
Preferences fieldPrefs;

const char* const EVENT_NAMES[EVENT_COUNT] = {
    "100m Dash",
    "Hurdles",
    "Long Jump",
    "Hammer",
    "Javelin",
    "High Jump",
};

int absInt(int value) {
  return value < 0 ? -value : value;
}

float absFloat(float value) {
  return value < 0.0f ? -value : value;
}

float clampFloat(float value, float low, float high) {
  if (value < low) return low;
  if (value > high) return high;
  return value;
}
}

FemtoFieldGame::FemtoFieldGame(uint32_t width, uint32_t height)
    : App("Femto Field", width, height) {}

bool FemtoFieldGame::hasCustomOverlay() const {
  return true;
}

void FemtoFieldGame::onAppReset() {
  pinMode(FIELD_LED_PIN, OUTPUT);
  setFieldLed(false);
  loadRecords();
  eventIndex_ = 0;
  round_ = 1;
  attemptsRemaining_ = STARTING_ATTEMPT_BANK;
  pressureMode_ = false;
  roundHadZero_ = false;
  totalScore_ = 0;
  nextExtraLifeAt_ = EXTRA_LIFE_POINTS;
  startEventIntro();
}

void FemtoFieldGame::startEventIntro() {
  state_ = FieldState::EventIntro;
  stateTimerMs_ = 0;
  bestAttemptScore_ = 0;
  bestAttemptValue_ = 0;
  qualified_ = false;
  extraLifeEarned_ = false;
  throwAnimMs_ = 0;
  throwSignedDistanceCm_ = 0;
  messageTimerMs_ = 0;
  setFieldLed(false);
}

void FemtoFieldGame::startAttempt() {
  bestAttemptScore_ = 0;
  bestAttemptValue_ = 0;
  extraLifeEarned_ = false;
  stateTimerMs_ = 0;

  switch (eventIndex_) {
    case 0:
      startCueSequence(CueMode::Dash, 10, max<uint16_t>(360, 510 - round_ * 14));
      break;
    case 1:
      hurdleIndex_ = 0;
      hurdlesCleared_ = 0;
      hurdleScrollMs_ = 0;
      hurdleRunnerY_ = 0.0f;
      hurdleVy_ = 0.0f;
      hurdleX_ = static_cast<float>(width + 8);
      hurdleSpeed_ = 23.0f + static_cast<float>(round_) * 1.4f;
      hurdleScored_ = false;
      hurdleHit_ = false;
      hurdleJumped_ = false;
      runQuality_ = 0;
      cueElapsedMs_ = 0;
      cueTargetMs_ = max<uint16_t>(360, 540 - round_ * 16);
      cueBaseTargetMs_ = cueTargetMs_;
      state_ = FieldState::HurdleCue;
      setFieldLed(false);
      break;
    case 2:
      startCueSequence(CueMode::LongRun, 7, max<uint16_t>(350, 500 - round_ * 12));
      break;
    case 3:
      directionDeg_ = 180.0f;
      directionQuality_ = 0;
      angleQuality_ = 0;
      powerQuality_ = 0;
      state_ = FieldState::DirectionSelect;
      setFieldLed(false);
      break;
    case 4:
      startCueSequence(CueMode::JavelinRun, 6, max<uint16_t>(350, 490 - round_ * 12));
      break;
    case 5:
    default:
      holding_ = false;
      highHoldMs_ = 0;
      highAngleQuality_ = 0;
      state_ = FieldState::HighJumpHold;
      setFieldLed(false);
      break;
  }
}

void FemtoFieldGame::startCueSequence(CueMode mode, uint8_t total, uint16_t targetMs) {
  cueMode_ = mode;
  cueTotal_ = total;
  cueDone_ = 0;
  cueQualitySum_ = 0;
  cueElapsedMs_ = 0;
  cueBaseTargetMs_ = targetMs;
  cueTargetMs_ = targetMs;
  lastCueErrorMs_ = 0;
  lastQuality_ = 0;
  messageTimerMs_ = 0;
  state_ = FieldState::CueSequence;
  stateTimerMs_ = 0;
  setFieldLed(false);
}

void FemtoFieldGame::updateRunning(uint32_t deltaMs, const ButtonInput& b1, const ButtonInput& b2) {
  const uint16_t cappedDelta = static_cast<uint16_t>(min<uint32_t>(deltaMs, 180));
  stateTimerMs_ += cappedDelta;
  if (messageTimerMs_ > cappedDelta) {
    messageTimerMs_ -= cappedDelta;
  } else {
    messageTimerMs_ = 0;
  }

  if (state_ == FieldState::EventIntro) {
    setFieldLed(false);
    if (stateTimerMs_ >= 450 && b1.click) {
      startAttempt();
    }
    return;
  }

  if (state_ == FieldState::Result) {
    setFieldLed(false);
    handleResultInput(input);
    return;
  }

  if (state_ == FieldState::ThrowAnim) {
    throwAnimMs_ += cappedDelta;
    setFieldLed(false);
    if (throwAnimMs_ >= THROW_ANIM_MS) {
      state_ = FieldState::Result;
      stateTimerMs_ = 0;
    }
    return;
  }

  if (state_ == FieldState::CueSequence) {
    cueElapsedMs_ += cappedDelta;
    updateCueLed();
    bool accepted = false;
    int16_t error = 0;
    if (b1.pressed) {
      error = static_cast<int16_t>(cueElapsedMs_) - static_cast<int16_t>(cueTargetMs_);
      accepted = true;
    } else if (cueElapsedMs_ > cueTargetMs_ + 430) {
      error = 430;
      accepted = true;
    }

    if (accepted) {
      const uint8_t quality = timingQuality(error);
      cueQualitySum_ += quality;
      cueDone_++;
      lastCueErrorMs_ = error;
      lastQuality_ = quality;
      if (error < 0) {
        messageTimerMs_ = 650;
      }
      if (cueDone_ >= cueTotal_) {
        setFieldLed(false);
        finishCueSequence();
      } else {
        const int16_t jitter = static_cast<int16_t>(random(0, 110)) - 35;
        cueElapsedMs_ = 0;
        cueTargetMs_ = static_cast<uint16_t>(max<int>(300, static_cast<int>(cueBaseTargetMs_) + jitter));
      }
    }
    return;
  }

  if (state_ == FieldState::HurdleCue) {
    const float dt = static_cast<float>(cappedDelta) * 0.001f;
    if ((b1.click || b1.pressed) && hurdleRunnerY_ <= 0.1f) {
      hurdleVy_ = 46.0f;
      hurdleJumped_ = true;
    }

    hurdleRunnerY_ += hurdleVy_ * dt;
    hurdleVy_ -= 88.0f * dt;
    if (hurdleRunnerY_ < 0.0f) {
      hurdleRunnerY_ = 0.0f;
      hurdleVy_ = 0.0f;
    }

    hurdleX_ -= hurdleSpeed_ * dt;
    const float runnerX = 18.0f;
    if (!hurdleScored_ && hurdleX_ <= runnerX + 2.0f) {
      const bool cleared = hurdleRunnerY_ >= 7.0f;
      if (cleared) {
        hurdlesCleared_++;
        runQuality_ += min<uint8_t>(100, 62 + static_cast<uint8_t>(hurdleRunnerY_ * 3.0f));
        hurdleHit_ = false;
      } else {
        runQuality_ += 10;
        hurdleHit_ = true;
      }
      hurdleIndex_++;
      hurdleScored_ = true;
    }

    if (hurdleX_ < -12.0f) {
      if (hurdleIndex_ >= 8) {
        const uint8_t avg = runQuality_ / 8;
        const uint16_t timeMs = 17200 - static_cast<uint16_t>(hurdlesCleared_) * 520 -
                                static_cast<uint16_t>(avg) * 12 - static_cast<uint16_t>(round_) * 80;
        const bool qualified = hurdlesCleared_ >= 6 && timeMs <= qualifyingTarget();
        const uint16_t score = qualified ? static_cast<uint16_t>(80 + hurdlesCleared_ * 12 + max<int>(0, qualifyingTarget() - timeMs) / 35 + avg / 2) : avg / 3;
        finishAttempt(qualified, score, timeMs, ResultUnit::Time);
      } else {
        hurdleX_ = static_cast<float>(width + random(8, 24));
        hurdleSpeed_ += 0.8f;
        hurdleScored_ = false;
        hurdleHit_ = false;
      }
    }
    return;
  }

  if (state_ == FieldState::DirectionSelect) {
    directionDeg_ += static_cast<float>(cappedDelta) * (0.16f + static_cast<float>(round_) * 0.012f);
    while (directionDeg_ >= 360.0f) directionDeg_ -= 360.0f;
    if (b1.pressed) {
      directionQuality_ = angleQuality(directionDeg_, 0.0f, 88.0f);
      selectorAngle_ = 20.0f;
      selectorDir_ = 1;
      state_ = FieldState::AngleSelect;
      stateTimerMs_ = 0;
    }
    return;
  }

  if (state_ == FieldState::AngleSelect) {
    selectorAngle_ += static_cast<float>(selectorDir_) * static_cast<float>(cappedDelta) * (0.060f + static_cast<float>(round_) * 0.004f);
    if (selectorAngle_ > 65.0f) {
      selectorAngle_ = 65.0f;
      selectorDir_ = -1;
    } else if (selectorAngle_ < 20.0f) {
      selectorAngle_ = 20.0f;
      selectorDir_ = 1;
    }
    if (!b1.pressed) {
      return;
    }

    if (eventIndex_ == 2) {
      angleQuality_ = angleQuality(selectorAngle_, 42.0f, 24.0f);
      const uint16_t distanceCm = 235 + static_cast<uint16_t>(runQuality_) * 5 + static_cast<uint16_t>(angleQuality_) * 2;
      const bool qualified = distanceCm >= qualifyingTarget();
      const uint16_t score = qualified ? static_cast<uint16_t>(70 + max<int>(0, distanceCm - qualifyingTarget()) / 7 + runQuality_ / 2 + angleQuality_ / 3) : (runQuality_ + angleQuality_) / 5;
      finishAttempt(qualified, score, distanceCm, ResultUnit::Distance);
    } else if (eventIndex_ == 3) {
      angleQuality_ = angleQuality(selectorAngle_, 45.0f, 26.0f);
      startCueSequence(CueMode::HammerPower, 5, max<uint16_t>(330, 430 - round_ * 10));
    } else if (eventIndex_ == 4) {
      selectorAngle_ = 15.0f;
      holding_ = false;
      state_ = FieldState::JavelinHoldAngle;
      stateTimerMs_ = 0;
    } else {
      angleQuality_ = angleQuality(selectorAngle_, 43.0f, 25.0f);
      const uint16_t distanceCm = 1700 + static_cast<uint16_t>(runQuality_) * 28 + static_cast<uint16_t>(angleQuality_) * 19;
      const bool qualified = distanceCm >= qualifyingTarget();
      const uint16_t score = qualified ? static_cast<uint16_t>(75 + max<int>(0, distanceCm - qualifyingTarget()) / 45 + runQuality_ / 2 + angleQuality_ / 3) : (runQuality_ + angleQuality_) / 5;
      finishAttempt(qualified, score, distanceCm, ResultUnit::Distance);
    }
    return;
  }

  if (state_ == FieldState::JavelinHoldAngle) {
    if (!holding_ && b1.pressed) {
      holding_ = true;
      selectorAngle_ = 15.0f;
    } else if (holding_ && b1.down) {
      selectorAngle_ += static_cast<float>(cappedDelta) * (0.072f + static_cast<float>(round_) * 0.004f);
      if (selectorAngle_ > 70.0f) {
        selectorAngle_ = 70.0f;
      }
    }

    if (holding_ && (b1.released || selectorAngle_ >= 70.0f)) {
      holding_ = false;
      angleQuality_ = angleQuality(selectorAngle_, 43.0f, 25.0f);
      const uint16_t distanceCm = 1700 + static_cast<uint16_t>(runQuality_) * 28 + static_cast<uint16_t>(angleQuality_) * 19;
      const bool qualified = distanceCm >= qualifyingTarget();
      const uint16_t score = qualified ? static_cast<uint16_t>(75 + max<int>(0, distanceCm - qualifyingTarget()) / 45 + runQuality_ / 2 + angleQuality_ / 3) : 0;
      finishAttempt(qualified, score, distanceCm, ResultUnit::Distance);
    }
    return;
  }

  if (state_ == FieldState::HighJumpHold) {
    if (!holding_ && b1.pressed) {
      holding_ = true;
      highHoldMs_ = 0;
    } else if (holding_ && b1.down && highHoldMs_ < 760) {
      highHoldMs_ += cappedDelta;
    }

    if (holding_ && (b1.released || highHoldMs_ >= 760)) {
      const float selectedAngle = clampFloat(18.0f + static_cast<float>(highHoldMs_) * 0.095f, 18.0f, 72.0f);
      highAngleQuality_ = angleQuality(selectedAngle, 42.0f, 25.0f);
      holding_ = false;
      startCueSequence(CueMode::HighBonus, 3, max<uint16_t>(360, 510 - round_ * 15));
    }
    return;
  }
}

void FemtoFieldGame::finishCueSequence() {
  const uint8_t avg = cueTotal_ == 0 ? 0 : static_cast<uint8_t>(cueQualitySum_ / cueTotal_);
  switch (cueMode_) {
    case CueMode::Dash: {
      const uint16_t timeMs = 17000 - static_cast<uint16_t>(avg) * 65;
      const bool qualified = timeMs <= qualifyingTarget();
      const uint16_t score = qualified ? static_cast<uint16_t>(85 + max<int>(0, qualifyingTarget() - timeMs) / 38 + avg / 2) : 0;
      finishAttempt(qualified, score, timeMs, ResultUnit::Time);
      break;
    }
    case CueMode::LongRun:
      runQuality_ = avg;
      selectorAngle_ = 20.0f;
      selectorDir_ = 1;
      state_ = FieldState::AngleSelect;
      stateTimerMs_ = 0;
      break;
    case CueMode::HammerPower: {
      powerQuality_ = avg;
      const float forwardFactor = cosf(directionDeg_ * 0.0174533f);
      const uint16_t rawDistanceCm = 1200 + static_cast<uint16_t>(angleQuality_) * 22 +
                                     static_cast<uint16_t>(powerQuality_) * 25;
      const int16_t signedDistanceCm = static_cast<int16_t>(clampFloat(static_cast<float>(rawDistanceCm) * forwardFactor, -9500.0f, 9500.0f));
      throwSignedDistanceCm_ = signedDistanceCm;
      const uint16_t forwardDistanceCm = signedDistanceCm > 0 ? static_cast<uint16_t>(signedDistanceCm) : 0;
      const bool qualified = forwardDistanceCm >= qualifyingTarget();
      const uint16_t score = qualified ? static_cast<uint16_t>(80 + max<int>(0, forwardDistanceCm - qualifyingTarget()) / 55 + angleQuality_ / 3 + powerQuality_ / 2) : 0;
      finishAttempt(qualified, score, static_cast<uint16_t>(absInt(signedDistanceCm)), ResultUnit::Distance);
      break;
    }
    case CueMode::JavelinRun:
      runQuality_ = avg;
      selectorAngle_ = 15.0f;
      holding_ = false;
      state_ = FieldState::JavelinHoldAngle;
      stateTimerMs_ = 0;
      break;
    case CueMode::HighBonus: {
      const uint16_t heightCm = 88 + static_cast<uint16_t>(highAngleQuality_) * 7 / 10 +
                                static_cast<uint16_t>(avg) * 35 / 100 + static_cast<uint16_t>(round_) * 2;
      const bool qualified = heightCm >= qualifyingTarget();
      const uint16_t score = qualified ? static_cast<uint16_t>(75 + max<int>(0, heightCm - qualifyingTarget()) * 4 + highAngleQuality_ / 2 + avg / 3) : 0;
      finishAttempt(qualified, score, heightCm, ResultUnit::Height);
      break;
    }
  }
}

void FemtoFieldGame::finishAttempt(bool qualified, uint16_t eventScore, uint16_t value, ResultUnit unit) {
  if (!qualified) {
    eventScore = 0;
    if (!pressureMode_ && attemptsRemaining_ > 0) {
      attemptsRemaining_--;
    }
  }
  qualified_ = qualified;
  resultScore_ = eventScore;
  resultValue_ = value;
  resultUnit_ = unit;
  bestAttemptScore_ = max(bestAttemptScore_, eventScore);
  bestAttemptValue_ = max(bestAttemptValue_, value);
  extraLifeEarned_ = false;
  state_ = (eventIndex_ == 3) ? FieldState::ThrowAnim : FieldState::Result;
  stateTimerMs_ = 0;
  throwAnimMs_ = 0;
  setFieldLed(false);

  if (qualified) {
    addScore(eventScore);
    if (eventScore > bestEventScores_[eventIndex_]) {
      bestEventScores_[eventIndex_] = eventScore;
      saveEventRecord(eventIndex_);
    }
  }
}

void FemtoFieldGame::handleResultInput(const ButtonInput& b1, const ButtonInput& b2) {
  if (stateTimerMs_ < RESULT_LOCK_MS || !b1.click) {
    return;
  }
  if (qualified_) {
    advanceEvent();
    return;
  }
  if (!pressureMode_ && attemptsRemaining_ > 0) {
    startAttempt();
    return;
  }
  pressureMode_ = true;
  roundHadZero_ = true;
  advanceEvent();
}

void FemtoFieldGame::advanceEvent() {
  eventIndex_++;
  if (eventIndex_ >= EVENT_COUNT) {
    if (roundHadZero_) {
      endApp();
      return;
    }
    eventIndex_ = 0;
    round_++;
    attemptsRemaining_ = STARTING_ATTEMPT_BANK;
    pressureMode_ = false;
    roundHadZero_ = false;
  }
  startEventIntro();
}

void FemtoFieldGame::loseLifeOrRetry() {
  pressureMode_ = true;
  roundHadZero_ = true;
  advanceEvent();
}

void FemtoFieldGame::addScore(uint16_t amount) {
  totalScore_ += amount;
  if (totalScore_ >= nextExtraLifeAt_) {
    nextExtraLifeAt_ += EXTRA_LIFE_POINTS;
  }
  if (totalScore_ > bestTotalScore_) {
    bestTotalScore_ = totalScore_;
    saveTotalRecord();
  }
}

void FemtoFieldGame::setFieldLed(bool on) {
  digitalWrite(FIELD_LED_PIN, FIELD_LED_ACTIVE_LOW ? !on : on);
}

void FemtoFieldGame::updateCueLed() {
  const bool inPush = cueElapsedMs_ >= cueTargetMs_ && cueElapsedMs_ <= cueTargetMs_ + 120;
  setFieldLed(inPush);
}

uint8_t FemtoFieldGame::timingQuality(int16_t errorMs) {
  const int penalty = errorMs < 0 ? absInt(errorMs) * 2 : absInt(errorMs);
  if (penalty >= 330) {
    return 0;
  }
  return static_cast<uint8_t>(100 - (penalty * 100 / 330));
}

uint8_t FemtoFieldGame::angleQuality(float selected, float optimal, float tolerance) const {
  float diff = absFloat(selected - optimal);
  if (diff > 180.0f) {
    diff = 360.0f - diff;
  }
  if (diff >= tolerance) {
    return 0;
  }
  return static_cast<uint8_t>(100.0f - (diff * 100.0f / tolerance));
}

uint16_t FemtoFieldGame::qualifyingTarget() const {
  const uint8_t r = round_ > 0 ? round_ - 1 : 0;
  switch (eventIndex_) {
    case 0:
      return max<uint16_t>(11200, 14500 - static_cast<uint16_t>(r) * 500);
    case 1:
      return max<uint16_t>(12600, 15800 - static_cast<uint16_t>(r) * 500);
    case 2:
      return 460 + static_cast<uint16_t>(r) * 40;
    case 3:
      return 3650 + static_cast<uint16_t>(r) * 260;
    case 4:
      return 3550 + static_cast<uint16_t>(r) * 280;
    case 5:
    default:
      return 128 + static_cast<uint16_t>(r) * 8;
  }
}

uint8_t FemtoFieldGame::maxAttemptsForEvent() const {
  return pressureMode_ ? 1 : attemptsRemaining_;
}

bool FemtoFieldGame::isFieldEvent() const {
  return eventIndex_ >= 2;
}

const char* FemtoFieldGame::eventName(uint8_t index) const {
  return EVENT_NAMES[index % EVENT_COUNT];
}

void FemtoFieldGame::loadRecords() {
  if (bestLoaded_) {
    return;
  }
  fieldPrefs.begin("femtofld", true);
  bestTotalScore_ = fieldPrefs.getUInt("total", 0);
  bestInitials_ = fieldPrefs.getUShort("init", PlayerProfile::defaultInitials());
  for (uint8_t i = 0; i < EVENT_COUNT; i++) {
    char key[5];
    snprintf(key, sizeof(key), "ev%u", i);
    bestEventScores_[i] = fieldPrefs.getUShort(key, 0);
  }
  fieldPrefs.end();
  bestLoaded_ = true;
}

void FemtoFieldGame::saveTotalRecord() {
  bestInitials_ = PlayerProfile::loadInitials();
  fieldPrefs.begin("femtofld", false);
  fieldPrefs.putUInt("total", bestTotalScore_);
  fieldPrefs.putUShort("init", bestInitials_);
  fieldPrefs.end();
}

void FemtoFieldGame::saveEventRecord(uint8_t eventIndex) {
  bestInitials_ = PlayerProfile::loadInitials();
  fieldPrefs.begin("femtofld", false);
  char key[5];
  snprintf(key, sizeof(key), "ev%u", eventIndex);
  fieldPrefs.putUShort(key, bestEventScores_[eventIndex]);
  fieldPrefs.putUShort("init", bestInitials_);
  fieldPrefs.end();
}

void void FemtoFieldGame::drawRunning(TFT_eSPI& tft) { tft.fillScreen(TFT_BLACK); { tft.fillScreen(TFT_BLACK);
  switch (state_) {
    case FieldState::EventIntro:
      drawEventIntro(tft);
      break;
    case FieldState::CueSequence:
      drawCueSequence(tft);
      break;
    case FieldState::HurdleCue:
    case FieldState::HurdleHold:
      drawHurdles(tft);
      break;
    case FieldState::DirectionSelect:
      drawDirectionSelect(tft);
      break;
    case FieldState::AngleSelect:
      drawAngleSelect(tft);
      break;
    case FieldState::JavelinHoldAngle:
      drawJavelinHoldAngle(tft);
      break;
    case FieldState::HighJumpHold:
      drawHighJumpHold(tft);
      break;
    case FieldState::ThrowAnim:
      drawThrowAnim(tft);
      break;
    case FieldState::Result:
      drawResult(tft);
      break;
  }
}

void FemtoFieldGame::drawStart(TFT_eSPI& tft) { tft.fillScreen(TFT_BLACK);
  loadRecords();
  tft.drawRect(0, 0, width + 2, height);
  if (showStartPromptPage()) {

    tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.drawString(20, 16, "Press");
    tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.drawString(13, 29, "to Start");
    return;
  }
  if (showStartScorePage()) {
    char initials[4];
    PlayerProfile::unpackDottedInitials(bestInitials_, initials);
    const uint8_t eventRecord = (millis() / 650) % EVENT_COUNT;

    tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.drawString(3, 9, "Top Total");

    tft.setCursor(3, 19);
    if (bestTotalScore_ == 0) {
      tft.print("--");
    } else {
      tft.print(initials);
      tft.print(" ");
      tft.print(bestTotalScore_);
    }
    tft.setCursor(3, 29);
    tft.print(eventName(eventRecord));
    tft.setCursor(3, 38);
    tft.print("Best ");
    tft.print(bestEventScores_[eventRecord]);
    return;
  }
  drawTrackSplash(tft);
}

void FemtoFieldGame::drawEnd(TFT_eSPI& tft) { tft.fillScreen(TFT_BLACK);
  setFieldLed(false);
  tft.drawRect(0, 0, width + 2, height);

  tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.drawString(3, 9, "Game Over");

  tft.setCursor(3, 20);
  tft.print("Score ");
  tft.print(totalScore_);
  tft.setCursor(3, 29);
  tft.print("Best ");
  tft.print(bestTotalScore_);
  tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.drawString(3, 38, "Tap retry Hold menu");
}

void FemtoFieldGame::drawScoreHeader(TFT_eSPI& tft) {

  tft.setCursor(2, 6);
  tft.print("R");
  tft.print(round_);
  tft.print(" A");
  tft.print(attemptsRemaining_);
  tft.print(" S");
  tft.print(totalScore_);
}

void FemtoFieldGame::drawEventIntro(TFT_eSPI& tft) {
  tft.drawRect(, 0, width, height);
  drawScoreHeader(tft);

  tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.drawString(3, 17, eventName(eventIndex_));

  tft.setCursor(3, 27);
  tft.print("Qual ");
  if (eventIndex_ <= 1) {
    tft.print(qualifyingTarget() / 1000);
    tft.print(".");
    tft.print((qualifyingTarget() / 100) % 10);
    tft.print("s");
  } else if (eventIndex_ == 5) {
    tft.print(qualifyingTarget() / 100);
    tft.print(".");
    tft.print((qualifyingTarget() / 10) % 10);
    tft.print("m");
  } else {
    tft.print(qualifyingTarget() / 100);
    tft.print(".");
    tft.print((qualifyingTarget() / 10) % 10);
    tft.print("m");
  }
  tft.setCursor(3, 38);
  if (pressureMode_) {
    tft.print("Pressure 1 try");
  } else {
    tft.print("Attempts left ");
    tft.print(attemptsRemaining_);
  }
}

void FemtoFieldGame::drawCueSequence(TFT_eSPI& tft) {
  tft.drawRect(, 0, width, height);
  drawScoreHeader(tft);
  const bool push = cueElapsedMs_ >= cueTargetMs_;

  tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.drawString(22, 18, push ? "PUSH" : "WAIT");
  if (messageTimerMs_ > 0) {

    tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.drawString(9, 27, "Early x2");
  } else {

    tft.setCursor(4, 27);
    tft.print("Q ");
    tft.print(lastQuality_);
  }
  const int barW = map(cueDone_, 0, cueTotal_, 0, 60);
  tft.drawRect(5, 32, 61, 4);
  if (barW > 0) {
    tft.fillRect(6, 33, barW, 2);
  }
  if (cueMode_ == CueMode::LongRun || cueMode_ == CueMode::JavelinRun || cueMode_ == CueMode::Dash) {
    drawStick(tft, 9 + (cueDone_ * 4), 31, false);
  }
}

void FemtoFieldGame::drawHurdles(TFT_eSPI& tft) {
  tft.drawRect(, 0, width, height);
  drawScoreHeader(tft);
  const bool airborne = hurdleRunnerY_ > 0.2f;
  const int hurdleX = static_cast<int>(hurdleX_);
  tft.drawLine(2, 33, width - 3, 33);
  drawStick(tft, 18, 33 - static_cast<int>(hurdleRunnerY_), airborne);
  if (hurdleX >= static_cast<int>() + 2 && hurdleX <= static_cast<int>(width - 14)) {
    drawHurdle(tft, hurdleX, 33, hurdleHit_);
  }
  if (!hurdleJumped_) {

    tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.drawString(17, 17, "TAP JUMP");
  }

  tft.setCursor(3, 39);
  tft.print("H");
  tft.print(hurdleIndex_ + 1);
  tft.print("/8 C");
  tft.print(hurdlesCleared_);
}

void FemtoFieldGame::drawDirectionSelect(TFT_eSPI& tft) {
  tft.drawRect(, 0, width, height);
  drawScoreHeader(tft);
  const int cx = 35;
  const int cy = 24;
  tft.drawCircle(cx, cy, 3);
  tft.drawCircle(cx, cy, 15);
  const float rad = directionDeg_ * 0.0174533f;
  const int bx = cx + static_cast<int>(cosf(rad) * 15);
  const int by = cy + static_cast<int>(sinf(rad) * 15);
  tft.drawLine(cx, cy, bx, by);
  tft.fillCircle(bx, by, 2);
  tft.drawLine(cx, cy, cx + 17, cy);

  tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.drawString(3, 14, "Aim ahead");
}

void FemtoFieldGame::drawAngleSelect(TFT_eSPI& tft) {
  tft.drawRect(, 0, width, height);
  drawScoreHeader(tft);
  drawAngleFan(tft, 12, 32, selectorAngle_);

  tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.drawString(31, 17, "ANGLE");

  tft.setCursor(34, 27);
  tft.print(static_cast<int>(selectorAngle_));
  tft.print(" deg");
}

void FemtoFieldGame::drawJavelinHoldAngle(TFT_eSPI& tft) {
  tft.drawRect(, 0, width, height);
  drawScoreHeader(tft);
  drawAngleFan(tft, 12, 32, selectorAngle_);
  tft.drawLine(8, 33, 32, 24);

  tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.drawString(32, 15, holding_ ? "Release" : "Hold");

  tft.setCursor(35, 27);
  tft.print(static_cast<int>(selectorAngle_));
  tft.print(" deg");
}

void FemtoFieldGame::drawHighJumpHold(TFT_eSPI& tft) {
  tft.drawRect(, 0, width, height);
  drawScoreHeader(tft);
  tft.drawLine(6, 33, 64, 33);
  tft.drawLine(48, 16, 48, 33);
  tft.drawLine(62, 16, 62, 33);
  tft.drawLine(48, 16, 62, 16);
  const int jumpY = holding_ ? 31 - min<int>(14, highHoldMs_ / 45) : 33;
  drawStick(tft, 27, jumpY, holding_);

  tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.drawString(5, 15, holding_ ? "Release" : "Hold");
}

void FemtoFieldGame::drawThrowAnim(TFT_eSPI& tft) {
  tft.drawRect(, 0, width, height);

  tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.drawString(3, 7, "Hammer");

  const int originX = 18;
  const int groundY = 32;
  tft.drawLine(3, groundY, width - 4, groundY);
  drawStick(tft, originX - 5, groundY, false);

  const float progress = min(1.0f, static_cast<float>(throwAnimMs_) / static_cast<float>(THROW_ANIM_MS));
  const float distanceM = static_cast<float>(throwSignedDistanceCm_) / 100.0f;
  const int travelPx = constrain(static_cast<int>(distanceM * 1.35f), -18, 48);
  const int ballX = originX + static_cast<int>(static_cast<float>(travelPx) * progress);
  const int arc = static_cast<int>(sinf(progress * 3.14159f) * 13.0f);
  const int ballY = groundY - 2 - arc;
  tft.fillCircle(ballX, ballY, 2);
  tft.drawLine(originX, groundY - 6, ballX, ballY);

  tft.setCursor(38, 18);
  if (throwSignedDistanceCm_ < 0) {
    tft.print("-");
  }
  tft.print(absInt(throwSignedDistanceCm_) / 100);
  tft.print(".");
  tft.print((absInt(throwSignedDistanceCm_) / 10) % 10);
  tft.print("m");
}

void FemtoFieldGame::drawResult(TFT_eSPI& tft) {
  tft.drawRect(, 0, width, height);

  tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.drawString(3, 9, qualified_ ? "Qualified" : "No Qual");

  tft.setCursor(3, 18);
  if (resultUnit_ == ResultUnit::Time) {
    tft.print("Time ");
    tft.print(resultValue_ / 1000);
    tft.print(".");
    tft.print((resultValue_ / 100) % 10);
    tft.print("s");
    if (eventIndex_ == 1) {
      tft.print(" C");
      tft.print(hurdlesCleared_);
    }
  } else {
    tft.print(resultUnit_ == ResultUnit::Height ? "Height " : "Dist ");
    if (eventIndex_ == 3 && throwSignedDistanceCm_ < 0) {
      tft.print("-");
    }
    const uint16_t displayValue = eventIndex_ == 3 ? static_cast<uint16_t>(absInt(throwSignedDistanceCm_)) : resultValue_;
    tft.print(displayValue / 100);
    tft.print(".");
    tft.print((displayValue / 10) % 10);
    tft.print("m");
  }
  tft.setCursor(3, 27);
  tft.print("Pts ");
  tft.print(resultScore_);
  tft.print(" Tot ");
  tft.print(totalScore_);
  tft.setCursor(3, 38);
  if (qualified_) {
    tft.print("Tap next");
  } else if (!pressureMode_ && attemptsRemaining_ > 0) {
    tft.print("Tap retry A");
    tft.print(attemptsRemaining_);
  } else {
    tft.print("Tap next 0pts");
  }
}

void FemtoFieldGame::drawStick(TFT_eSPI& tft, int x, int groundY, bool airborne) {
  const int y = groundY - 9;
  tft.drawCircle(x, y - 3, 2);
  tft.drawLine(x, y - 1, x, y + 5);
  tft.drawLine(x, y + 2, x + 4, y + (airborne ? 0 : 3));
  tft.drawLine(x, y + 2, x - 4, y + (airborne ? 1 : 4));
  tft.drawLine(x, y + 5, x + 4, groundY);
  tft.drawLine(x, y + 5, x - 3, groundY - (airborne ? 4 : 0));
}

void FemtoFieldGame::drawHurdle(TFT_eSPI& tft, int x, int groundY, bool fallen) {
  if (fallen) {
    tft.drawLine(x, groundY - 1, x + 11, groundY - 5);
    tft.drawLine(x + 1, groundY, x + 4, groundY - 3);
    tft.drawLine(x + 9, groundY, x + 7, groundY - 4);
    return;
  }
  tft.drawLine(x, groundY, x + 2, groundY - 9);
  tft.drawLine(x + 12, groundY, x + 10, groundY - 9);
  tft.drawLine(x + 2, groundY - 9, x + 10, groundY - 9);
}

void FemtoFieldGame::drawAngleFan(TFT_eSPI& tft, int ox, int oy, float angleDeg) {
  tft.drawLine(ox, oy, ox + 24, oy);
  tft.drawLine(ox, oy, ox + 18, oy - 18);
  const float rad = -angleDeg * 0.0174533f;
  tft.drawLine(ox, oy, ox + static_cast<int>(cosf(rad) * 24), oy + static_cast<int>(sinf(rad) * 24));
}

void FemtoFieldGame::drawTrackSplash(TFT_eSPI& tft) {

  tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.drawString(3, 9, "Femto Field");

  tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.drawString(3, 17, "Track and Field");
  tft.drawLine(3, 32, 67, 32);
  tft.drawLine(3, 36, 67, 36);
  drawStick(tft, 18, 32, false);
  drawHurdle(tft, 40, 32);
  tft.drawLine(53, 22, 64, 14);
  tft.drawPixel(66, 13);
}
