#pragma once
// Sequencer runner: its own FreeRTOS task, high priority, on core 1.
// Sole owner of the FSM, the relays and I2C. Web, MQTT, WiFi and AP never block it:
// they talk to it only through atomic requests and a read-only snapshot (docs/ARCHITECTURE.md §1).

#include <atomic>
#include <algorithm>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include "Config.h"
#include "Relays.h"
#include "Sequencer.h"

class SeqRunner {
public:
  struct Snap {
    SeqState state = SeqState::STOPPED;
    uint32_t idx = 0;
    uint32_t cycle = 0;
    uint32_t rev = 0;
    uint8_t  manual = 0;
    uint8_t  relays = 0;
    bool     relaysOk = false;
    uint32_t maxLateUs = 0;   // worst task lateness since boot (proof that the network does not interfere)
  };

  // Call before any network initialization.
  void begin(const SeqConfig& c) {
    relays_.begin();                  // all relays off
    cfg_ = c;
    pendingMtx_ = xSemaphoreCreateMutex();
    if (c.autostart) reqStart_ = true;
    xTaskCreatePinnedToCore(taskEntry, "seq", 4096, this, 10, nullptr, 1);
  }

  // ── Requests (any task; they never block the FSM) ──
  void requestStart() { reqStart_ = true; }
  void requestStop()  { reqStop_ = true; }
  // New config: the FSM adopts it and goes to STOPPED. Holds the mutex only for one copy.
  void setConfig(const SeqConfig& c) {
    xSemaphoreTake(pendingMtx_, portMAX_DELAY);
    pendingCfg_ = c;
    pendingFlag_ = true;
    xSemaphoreGive(pendingMtx_);
  }
  // Manual test: only accepted with the FSM stopped. The FSM revalidates when applying.
  bool requestManual(uint8_t mask) {
    if (snap().state != SeqState::STOPPED || reqStart_) return false;
    manualReq_ = mask;
    return true;
  }

  // ── Reading (snapshot) ──
  Snap snap() const {
    Snap s;
    portENTER_CRITICAL(&mux_);
    s = snap_;
    portEXIT_CRITICAL(&mux_);
    return s;
  }
  SeqState state() const      { return snap().state; }
  size_t   stepIndex() const  { return snap().idx; }
  uint32_t cycle() const      { return snap().cycle; }
  uint32_t revision() const   { return snap().rev; }
  uint8_t  manualMask() const { return snap().manual; }
  uint8_t  relaysMask() const { return snap().relays; }
  bool     relaysOk() const   { return snap().relaysOk; }
  uint32_t maxLateUs() const  { return snap().maxLateUs; }

private:
  Relays    relays_;
  Sequencer fsm_;
  SeqConfig cfg_;            // only the task reads/writes it
  SeqConfig pendingCfg_;     // guarded by pendingMtx_
  SemaphoreHandle_t pendingMtx_ = nullptr;
  std::atomic<bool> pendingFlag_{false};
  std::atomic<bool> reqStart_{false};
  std::atomic<bool> reqStop_{false};
  std::atomic<int>  manualReq_{-1};
  mutable portMUX_TYPE mux_ = portMUX_INITIALIZER_UNLOCKED;
  Snap snap_;

  static void taskEntry(void* p) { static_cast<SeqRunner*>(p)->run(); }

  void run() {
    TickType_t last = xTaskGetTickCount();
    uint32_t prevUs = micros(), maxLate = 0, lastRelayRetry = 0;
    for (;;) {
      vTaskDelayUntil(&last, 1);
      uint32_t nowUs = micros();
      uint32_t dt = nowUs - prevUs;
      prevUs = nowUs;
      if (dt > 1000 && dt - 1000 > maxLate) maxLate = dt - 1000;
      uint32_t now = millis();

      // New config: swap without blocking (if the network side is copying, it waits for the next pass).
      if (pendingFlag_ && xSemaphoreTake(pendingMtx_, 0) == pdTRUE) {
        std::swap(cfg_, pendingCfg_);
        pendingFlag_ = false;
        xSemaphoreGive(pendingMtx_);
        fsm_.requestStop();
      }
      if (reqStop_.exchange(false))  fsm_.requestStop();
      if (reqStart_.exchange(false)) fsm_.requestStart();
      int m = manualReq_.exchange(-1);
      if (m >= 0) fsm_.requestManual((uint8_t)m);

      fsm_.tick(cfg_, now);

      // I2C failing: retry every 100 ms, without hogging the task at 1 kHz.
      if (relays_.ok() || now - lastRelayRetry >= 100) {
        lastRelayRetry = now;
        relays_.apply(fsm_.outputMask(cfg_));
      }

      Snap s;
      s.state = fsm_.state();
      s.idx = (uint32_t)fsm_.stepIndex();
      s.cycle = fsm_.cycle();
      s.rev = fsm_.revision();
      s.manual = fsm_.manualMask();
      s.relays = relays_.current();
      s.relaysOk = relays_.ok();
      s.maxLateUs = maxLate;
      portENTER_CRITICAL(&mux_);
      snap_ = s;
      portEXIT_CRITICAL(&mux_);
    }
  }
};
