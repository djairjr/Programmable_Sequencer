# Changelog

All notable changes to this project are documented here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and the project uses
[Semantic Versioning](https://semver.org/).

## [1.3.0] - 2026-10-26

### Changed

- **The whole interface is in English**: web page, API error messages, validation messages and the Home Assistant
  entities. Default channel names are `Channel 1` ... `Channel 8`. Names already saved in `/config.json` are kept.
- State, mode and command identifiers are English: states `STOPPED`, `STEP`, `REPEAT_OFF`, `GAP`, `CYCLE_PAUSE`
  and modes `PULSE`, `SWAP` (they appear in `/api/state`, `/api/config`, `/config.json` and the Home Assistant
  state sensor and mode select). A saved `/config.json` written with the old mode names `PULSO`/`TROCA` is still
  read correctly. Home Assistant automations that matched the old state strings need updating.

## [1.2.0] - 2026-10-26

### Changed

- The product identity is now **Sequencer** everywhere: default hostname `sequencer` (mDNS
  `sequencer.local`), Home Assistant device name, boot banner, page title and the default channel names
  (`Canal 1` ... `Canal 8`). The firmware only sequences relay channels; what is wired to them is up to the
  installation. Values already saved in `/config.json` and `/network.json` are kept.
- Internal names follow: session cookie `seq_sess` (everyone logs in again after the update).

## [1.1.0] - 2026-10-26

### Changed

- **Defaults changed.** The AP SSID is now `Sequencer-XXXX` and the default page and AP
  password is `sequencer1234!`. A unit that never set its own page password (empty hash in `/network.json`)
  accepts the new default after an `app` update; a unit that set one keeps it.
- The AP password is now configurable: `ap.pass` in `/network.json` (8-63 characters; empty = default).
- The sequencer (FSM and relays) now runs in its own FreeRTOS task (core 1, priority 10, 1 ms period) and is
  started before any network code. Web, AP, MQTT and Home Assistant can no longer delay a relay.
- FSM transitions are anchored to the theoretical end of the previous state, so a late pass no longer
  accumulates drift over the steps.
- Saving a new sequence stops the FSM before the flash write, so the write stall happens with the relays off.
- The configuration AP comes up after 10 s without a network (was 15 s), in pure AP mode. The saved WiFi is
  retried every 60 s for up to 10 s, and never while a client is on the AP.
- MQTT/Home Assistant only try to connect when a broker is configured **and** an interface has an IP.
- Code comments, serial logs and documentation are in English. Page texts and Home Assistant labels stay in
  Portuguese.

### Fixed

- WiFi modem sleep is disabled; the core enables it by default and it can make TCP handshakes on the AP/STA be
  refused for seconds.

### Added

- `version` and `seq_max_late_us` in `/api/state`; max lateness in the serial `status` line.
- `docs/ARCHITECTURE.md`, `docs/BOARD.md`, `docs/UPDATING.md`, CI build.

## [1.0.0] - 2026-10-21

### Added

- First firmware: sovereign FSM (pulse and swap modes), JSON settings on LittleFS, web page with login,
  Ethernet + WiFi + configuration AP with captive portal, Home Assistant over MQTT Discovery, BOOT-button
  password recovery. FSM, web server, AP and MQTT shared one `loop()`.

[1.3.0]: https://github.com/djairjr/Programmable_Sequencer/releases/tag/v1.3.0
[1.2.0]: https://github.com/djairjr/Programmable_Sequencer/releases/tag/v1.2.0
[1.1.0]: https://github.com/djairjr/Programmable_Sequencer/releases/tag/v1.1.0
