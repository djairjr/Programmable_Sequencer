#pragma once
// Sovereign FSM of the sequencer (see docs/ARCHITECTURE.md §1).
// Interfaces only call request*(); tick() decides. Relays = outputMask(), an effect of the state.

#include "Config.h"

enum class SeqState : uint8_t { STOPPED, STEP, REPEAT_OFF, GAP, CYCLE_PAUSE };

static const char* seqStateName(SeqState s) {
  switch (s) {
    case SeqState::STOPPED:     return "STOPPED";
    case SeqState::STEP:        return "STEP";
    case SeqState::REPEAT_OFF:  return "REPEAT_OFF";
    case SeqState::GAP:         return "GAP";
    default:                    return "CYCLE_PAUSE";
  }
}

class Sequencer {
public:
  // ── Requests (written by the interfaces) ──
  void requestStart() { reqStart_ = true; }
  void requestStop()  { reqStop_ = true; }
  // Manual test: only accepted in STOPPED. Returns false if refused.
  bool requestManual(uint8_t mask) {
    if (state_ != SeqState::STOPPED || reqStart_) return false;
    manualMask_ = mask;
    return true;
  }

  // ── One pass of the FSM ──
  void tick(const SeqConfig& c, uint32_t now) {
    // Godmode: STOP and START are valid in any state.
    if (reqStop_) { reqStop_ = false; reqStart_ = false; enter(SeqState::STOPPED, now); manualMask_ = 0; }
    if (reqStart_) {
      reqStart_ = false;
      manualMask_ = 0;
      if (c.sequence.empty()) { enter(SeqState::STOPPED, now); }
      else { idx_ = 0; cycle_ = 1; enter(SeqState::STEP, now); }
    }

    uint32_t el = now - t0_;
    uint32_t t;
    switch (state_) {
      case SeqState::STOPPED:
        break;

      case SeqState::STEP:
        if (el < c.on_ms) break;
        t = t0_ + c.on_ms;
        if (idx_ + 1 < c.sequence.size()) {
          if (c.mode == LightMode::PULSE) enter(SeqState::GAP, t, now);
          else if (c.sequence[idx_ + 1] == c.sequence[idx_]) enter(SeqState::REPEAT_OFF, t, now);
          else { idx_++; enter(SeqState::STEP, t, now); }
        } else if (hasMoreCycles(c)) {
          enter(SeqState::CYCLE_PAUSE, t, now);
        } else {
          enter(SeqState::STOPPED, now);
        }
        break;

      case SeqState::GAP:
        if (el >= c.gap_ms) { idx_++; enter(SeqState::STEP, t0_ + c.gap_ms, now); }
        break;

      case SeqState::REPEAT_OFF:
        if (el >= c.blink_off_ms) { idx_++; enter(SeqState::STEP, t0_ + c.blink_off_ms, now); }
        break;

      case SeqState::CYCLE_PAUSE:
        if (el >= c.cycle_pause_ms) { cycle_++; idx_ = 0; enter(SeqState::STEP, t0_ + c.cycle_pause_ms, now); }
        break;
    }
  }

  // ── Effect of the state ──
  uint8_t outputMask(const SeqConfig& c) const {
    switch (state_) {
      case SeqState::STOPPED: return manualMask_;
      case SeqState::STEP:
        if (idx_ < c.sequence.size()) return (uint8_t)(1u << (c.sequence[idx_] - 1));
        return 0;
      default: return 0;
    }
  }

  SeqState state() const { return state_; }
  size_t   stepIndex() const { return idx_; }
  uint32_t cycle() const { return cycle_; }
  uint8_t  manualMask() const { return manualMask_; }
  // Increments on every transition: interfaces publish only when it changes.
  uint32_t revision() const { return rev_; }

private:
  SeqState state_ = SeqState::STOPPED;
  size_t   idx_ = 0;
  uint32_t cycle_ = 0;
  uint32_t t0_ = 0;
  uint32_t rev_ = 0;
  uint8_t  manualMask_ = 0;
  bool     reqStart_ = false;
  bool     reqStop_ = false;

  bool hasMoreCycles(const SeqConfig& c) const {
    if (c.repeat == RepeatMode::LOOP) return true;
    if (c.repeat == RepeatMode::N_TIMES) return cycle_ < c.repeat_count;
    return false;
  }

  // `start` is the theoretical instant of the transition (end of the previous state): time does not accumulate
  // drift. If the pass was late by more than 100 ms, resynchronize at `now`. No Serial here:
  // whoever watches `revision()` logs the transition.
  void enter(SeqState s, uint32_t start, uint32_t now) {
    if (now - start > 100) start = now;
    if (s == SeqState::STOPPED) { idx_ = 0; cycle_ = 0; }
    state_ = s;
    t0_ = start;
    rev_++;
  }
  void enter(SeqState s, uint32_t now) { enter(s, now, now); }
};
