#pragma once
// Pinout of the Waveshare ESP32-S3-ETH-8DI-8RO-C (Waveshare wiki + official demo WS_ETH.h / I2C_Driver.h)

// I2C: TCA9554 (relays) and PCF85063 RTC (unused)
constexpr int PIN_I2C_SDA = 42;
constexpr int PIN_I2C_SCL = 41;
constexpr uint8_t TCA9554_ADDR = 0x20;

// Ethernet W5500 (SPI)
constexpr int PIN_ETH_SCK  = 15;
constexpr int PIN_ETH_MISO = 14;
constexpr int PIN_ETH_MOSI = 13;
constexpr int PIN_ETH_CS   = 16;
constexpr int PIN_ETH_INT  = 12;
constexpr int PIN_ETH_RST  = 39;
constexpr int ETH_PHY_ADDR_W5500 = 1;

// BOOT button (password recovery) and WS2812 RGB LED
constexpr int PIN_BOOT = 0;
constexpr int PIN_RGB  = 38;

constexpr uint8_t NUM_CHANNELS = 8;
