# Programmable Sequencer

[![Firmware CI](https://github.com/djairjr/Programmable_Sequencer/actions/workflows/firmware-ci.yml/badge.svg)](https://github.com/djairjr/Programmable_Sequencer/actions/workflows/firmware-ci.yml)

Programmable relay sequencer for escape rooms on the Waveshare ESP32-S3-ETH-8DI-8RO-C. It switches up to eight
relay channels (CH1-CH8) in an order the Gamemaster programs from a web page or from Home Assistant, and that
order is the clue the player watches. Not a game: there is no player input and no win condition.

## Download

Compiled firmware is in [`releases/`](releases/) (one folder per version) and is attached to each
[GitHub Release](https://github.com/djairjr/Programmable_Sequencer/releases/latest). Latest: [`releases/1.3.1`](releases/1.3.1).
How to write it to a unit: [docs/UPDATING.md](docs/UPDATING.md).

> **Versions 1.2.0 and 1.3.0 cannot log in**: the correct password returns to the sign-in page. Fixed in 1.3.1; see
> the [issue](https://github.com/djairjr/Programmable_Sequencer/issues/1) and the [CHANGELOG](CHANGELOG.md). Use 1.3.1 or later.

## The one rule

**The sequence is sovereign.** The state machine and the relays run in their own FreeRTOS task (core 1,
priority 10, 1 ms period), started before any network code. The web server, the configuration AP, MQTT and
Home Assistant live on the other side and can only post atomic requests and read a snapshot. Nothing on the
network side can delay a relay: no WiFi, no broker, no phone on the AP, no flash write of the settings.
The worst task lateness since boot is exposed as `seq_max_late_us`. See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md).

## Features

- Free-length sequence of channels 1-8 with repeats (`4,4,6,7,8,1,2,5,5,6`).
- Two modes: pulse (dark gap between steps) and swap (next light on as the previous goes off; a repeated
  channel gets a short forced off so the repeat stays visible).
- Timings: on time, gap, cycle pause, repeat-off; repeat once, N times or forever; autostart on power-up.
- Web page served by the board on Ethernet, WiFi and the configuration AP (login with session cookie).
- Configuration AP with captive portal when there is no network for 10 s: `Sequencer-XXXX`, `192.168.4.1`.
- Home Assistant over MQTT Discovery, bidirectional, **only active when a broker is configured and an IP
  exists**. The board works the same without it.
- Settings in JSON on LittleFS (`/config.json`, `/network.json`) over defaults in code, including the AP password (`ap.pass`).

## Hardware

Waveshare **ESP32-S3-ETH-8DI-8RO-C** (regular network port, no PoE). Details, pinout, flash map and what
Waveshare does not document: [docs/BOARD.md](docs/BOARD.md).

## Build and flash

```
arduino-cli compile --fqbn esp32:esp32:esp32s3:FlashSize=16M,PSRAM=opi,CDCOnBoot=cdc,PartitionScheme=custom \
  firmware/ProgrammableSequencer
arduino-cli upload -p /dev/ttyACM0 --fqbn esp32:esp32:esp32s3:FlashSize=16M,PSRAM=opi,CDCOnBoot=cdc,PartitionScheme=custom \
  firmware/ProgrammableSequencer
```

Toolchain and library versions: [DEPENDENCIES.md](DEPENDENCIES.md). Updating a deployed unit by USB:
[docs/UPDATING.md](docs/UPDATING.md). There is **no over-the-air update yet**; the partition table already has
`ota_0`/`ota_1`.

## First use

1. Power the board. With no network it raises the WiFi `Sequencer-XXXX` (XXXX = last 4 hex digits of the MAC)
   within about 10 s. Default password `sequencer1234!`; set `ap.pass` in `network.json` to change it.
2. Open `http://192.168.4.1`, log in (default page password `sequencer1234!`) and set the sequence, the
   timings and, on the network tab, Ethernet/WiFi/MQTT.
3. Holding the BOOT button for 10 s restores the default page password.

Serial console (115200): `start`, `stop`, `status`.

## Repository layout

| path | content |
| --- | --- |
| `firmware/ProgrammableSequencer/` | the Arduino sketch and its headers |
| `releases/` | compiled firmware per version (`full` and `app` images, `SHA256SUMS`) |
| `docs/ARCHITECTURE.md` | FSM, tasks, network, web access, Home Assistant |
| `docs/BOARD.md` | the board: pins, power, flash map, undocumented behaviour, bench to-do |
| `docs/UPDATING.md` | how to update a unit over USB, and how to roll back |
| `CHANGELOG.md`, `VERSION` | versions; tags are `vX.Y.Z` and releases carry the binaries |

## Status

- Compiles from a clean clone (CI builds every push). **The task separation and the AP changes of 1.x have not yet run on a
  board**; the bench checks are listed in [docs/BOARD.md](docs/BOARD.md#10-to-measure-on-the-bench).
- Open: over-the-air update; relay behaviour during an ESP32 reset (not documented by Waveshare).
- The default passwords (`sequencer1234!`) are defined in `Config.h` and are public: change the page password and set
  `ap.pass` on every deployed unit. The page password is changed from the page; the AP password through `ap.pass` in `network.json` (or `POST /api/network`).
- Licensed under the [MIT License](LICENSE). The third-party libraries have their own licenses: see
  [DEPENDENCIES.md](DEPENDENCIES.md#licenses).
