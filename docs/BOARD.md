# Board: Waveshare ESP32-S3-ETH-8DI-8RO-C (regular version, no PoE)

Technical notes on the board this firmware targets. It contains only what has a source; what Waveshare does
**not** document is in section 9, and what is still to be measured on the bench is in section 10.

Sources: **[W]** Waveshare wiki https://www.waveshare.com/wiki/ESP32-S3-ETH-8DI-8RO-C (read 2026-10-26) ·
**[D]** the official Waveshare demo package (the same package serves the regular and PoE versions) ·
**[U]** values read from a deployed unit · **[F]** this firmware (`Pins.h`, `Relays.h`, `partitions.csv`).

## 1. Identification [U][W]

| item | value |
| --- | --- |
| MCU | ESP32-S3 QFN56, rev v0.2 |
| Flash | 16 MB (quad, 3.3 V) |
| PSRAM | 8 MB, embedded |
| Antenna | 2.4 GHz WiFi/BLE, external SMA |
| PoE version | separate product page (ESP32-S3-POE-ETH-8DI-8RO-C); this board has **no** PoE module |

## 2. Power [W]

- Screw terminal: **7-36 V DC** (wide input, isolated), or **5 V / 1 A over USB-C**.
- Onboard DC-DC converter MP1605GTF-Z. USB-C is also used for flashing and debugging.

## 3. Pinout [D][F]

| function | pin(s) | note |
| --- | --- | --- |
| Relays CH1-CH8 | TCA9554PWR EXIO1-EXIO8, I2C address 0x20 | not direct GPIOs |
| I2C (TCA9554 + RTC) | SDA = GPIO42, SCL = GPIO41 | bus shared with the PCF85063 RTC |
| Digital inputs DI1-DI8 | GPIO4-GPIO11 | isolated; the demo inverts the logic (`DIN_Inverse_Enable`); **unused** by this firmware (a future use is START/STOP/PAUSE/RESET buttons) |
| Ethernet W5500 (SPI) | INT = 12, MOSI = 13, MISO = 14, SCLK = 15, CS = 16, RST = 39, PHY address = 1 | `ETH.begin(ETH_PHY_W5500, ..., SPI)` |
| CAN / RS485 (UART1) | TX = GPIO17, RX = GPIO18 | isolated; unused |
| RGB LED WS2812 | GPIO38 | |
| Buzzer | GPIO46 | |
| BOOT | GPIO0 | |

## 4. Relays

- Eight channels, contact **1NO + 1NC**, up to 10 A / 250 V AC or 10 A / 30 V DC [W].
- The firmware writes the TCA9554 output register (reg 0x01) and sets the direction register (reg 0x03 = 0x00,
  all outputs). Bit 0 = CH1 ... bit 7 = CH8; high level = relay on [F].
- At boot the output register is zeroed **before** the pins become outputs, so no relay pulses [F].
- Loads are wired to the **NO** contact: off with the board unpowered or in reset (installation rule).
- The wiki's safety section says to confirm or reset the relay state after power-up [W]; the firmware does so by
  switching every relay off at boot.

## 5. Network

- W5500 Ethernet (10/100, SPI) and 2.4 GHz WiFi. The firmware uses both at the same time; Waveshare does not
  document simultaneous use as supported (section 9), so that is validated here.
- On Arduino-ESP32 core 3.3.12 **modem sleep is enabled by default** (`WIFI_PS_MIN_MODEM`) on every target but
  the S2. Without `WiFi.setSleep(false)` the TCP handshake on the AP/STA can be refused for seconds. The
  firmware disables it since 1.1.0.

## 6. Flashing [W]

- Native USB-C (USB-JTAG/serial, `303a:1001`, `/dev/ttyACM0` on Linux). Arduino IDE: "ESP32S3 Dev Module",
  **USB CDC On Boot = Enabled**.
- **Download mode:** hold BOOT, press and release RESET, release BOOT.
- FQBN used here: `esp32:esp32:esp32s3:FlashSize=16M,PSRAM=opi,CDCOnBoot=cdc,PartitionScheme=custom`.
- Use the esptool shipped with the Arduino core; a system-wide esptool may lack the S3 stub.
- Flash parameters of the build: `--flash-mode dio --flash-freq 80m --flash-size 16MB`.
- The demo package ships a factory test image (`ESP32-S3-POE-ETH-8DI-8RO-C.bin`) to be written at 0x0 with
  Espressif's Flash Download Tool 3.9.5 [D].

## 7. Flash map (`partitions.csv`) [F]

| partition | type | offset | size |
| --- | --- | --- | --- |
| nvs | data | 0x9000 | 0x5000 |
| otadata | data | 0xe000 | 0x2000 |
| app0 | ota_0 | 0x10000 | 0x400000 |
| app1 | ota_1 | 0x410000 | 0x400000 |
| spiffs (LittleFS) | data | 0x810000 | 0x7E0000 |
| coredump | data | 0xFF0000 | 0x10000 |

- `/config.json` (sequence) and `/network.json` (network, page password) live on LittleFS. **Writing the full
  16 MB image at 0x0 erases both.** To keep the settings, write **only the app at 0x10000**.
- The bootloader, the partition table and `boot_app0` of 1.0.0 and 1.1.x are byte-identical (checked with `cmp`).
- The table already has `ota_0`/`ota_1`, but the firmware does **not** implement OTA yet: USB is the only way.

## 8. What the firmware assumes from the board

- The FSM and the relays run in their own FreeRTOS task (core 1, priority 10); web, AP, MQTT and HA run on the
  Arduino `loop()` side. See [ARCHITECTURE.md](ARCHITECTURE.md).
- Watchdogs in the current build: interrupt watchdog 300 ms on both cores; task watchdog 5 s with panic; brownout
  detector on; core dump to flash enabled.

## 9. Not documented by Waveshare [W] (do not assume)

- The state of the TCA9554 outputs and relays during and after an ESP32 reset, and whether the RESET button
  reaches the expander.
- The power-up output state; whether the relays are latching (the wiki mentions latching relays only as a
  generic warning).
- The power budget; whether USB and the screw terminal may be used together.
- Any external watchdog or RTC alarm/timer feature; **OTA**.
- Simultaneous Ethernet + WiFi use; Ethernet initialization details.

## 10. To measure on the bench

Status 2026-10-26: firmware 1.3.1 runs on this board and the maintainer reported it fully functional. None of the
checks below has been taken yet (no numbers recorded), and per-channel confirmation of the relays was not recorded.

The system is made for entertainment. What a player perceives is the contrast between the relays being driven and
the end of the sequence (the cycle pause is always longer than the gap between steps, which the settings
validation enforces); a few milliseconds of jitter are not perceptible. The normal way to use the unit is to
configure it through its own access point, with no network, no broker and no Home Assistant, which is the lightest
load the firmware can have.

Checks that matter for that use:

1. Leave the unit running the sequence for several hours and read `seq_max_late_us` in `/api/state`. Anything within
   tens of milliseconds is far below what can be noticed.
2. Time for the AP to appear after power-up with no network (target: 10 s or less), and that a phone can join it and
   open the page.
3. What the loads do while the ESP32 resets (`ESP.restart()`, panic, the RESET button); Waveshare does not document it.
4. Physical proof of the relays CH1-CH8; it counts only once the owner confirms having seen or heard them
   (reported functional with 1.3.1; per channel not recorded).

Only if a unit is used with Ethernet, WiFi or Home Assistant:

5. `seq_max_late_us` with an unreachable MQTT broker, a saved WiFi network that is absent, and a settings write.
6. Brownout and stability with WiFi and Ethernet both active on the supplies used in the field (7-36 V and USB-C).
