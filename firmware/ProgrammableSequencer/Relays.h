#pragma once
// Relays CH1-CH8 through the TCA9554 I2C expander (EXIO1-EXIO8 = bits 0-7, high level = relay on).
// Based on WS_TCA9554PWR.cpp from the Waveshare demo.

#include <Wire.h>
#include "Pins.h"

class Relays {
public:
  bool begin() {
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
    // Outputs to zero BEFORE configuring the pins as outputs: no relay pulses at boot.
    bool ok = writeReg(REG_OUTPUT, 0x00) && writeReg(REG_CONFIG, 0x00);
    current_ = 0;
    ok_ = ok;
    if (!ok) Serial.println("[RELAY] TCA9554 did not answer at 0x20");
    return ok;
  }

  // Writes the mask only when it changes; if I2C fails, tries again on the next pass.
  void apply(uint8_t mask) {
    if (mask == current_ && ok_) return;
    ok_ = writeReg(REG_OUTPUT, mask);
    if (ok_) current_ = mask;
  }

  uint8_t current() const { return current_; }
  bool ok() const { return ok_; }

private:
  static constexpr uint8_t REG_OUTPUT = 0x01;
  static constexpr uint8_t REG_CONFIG = 0x03;
  uint8_t current_ = 0;
  bool ok_ = false;

  bool writeReg(uint8_t reg, uint8_t val) {
    Wire.beginTransmission(TCA9554_ADDR);
    Wire.write(reg);
    Wire.write(val);
    return Wire.endTransmission() == 0;
  }
};
