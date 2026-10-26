# Programmable Sequencer — Architecture

Programmable relay sequencer: switches the relay channels (CH1-CH8) in the order the Gamemaster (GM)
programmed. The order is the clue the player observes. The functional summary is in the
[README](../README.md); this document describes how the firmware implements it, starting with the FSM.


---

## 1. Sovereign FSM

A single state machine, a single `switch (state)` in `Sequencer::tick()`. The web page, Home
Assistant and serial only **write requests** to the FSM (`requestStart()`, `requestStop()`,
`requestManual()`). The decision of what happens is made by `tick()`. The relays are an **effect of
the state**, computed by `Sequencer::outputMask()` and written once per pass of the sequencer task (1 ms), only when
they change.

### 1.1 States

| state | relays (effect) | entered when |
| --- | --- | --- |
| `STOPPED` | all off (or the manual test mask) | boot, STOP, end of cycles, config saved |
| `STEP` | only the channel `sequence[idx]` | START, or step advance |
| `REPEAT_OFF` | all off | SWAP, and the next step repeats the current channel |
| `GAP` | all off | PULSE, between one step and the next |
| `CYCLE_PAUSE` | all off | end of sequence, with cycles left |

### 1.2 Transitions

```
                 START (godmode)
   ┌───────────┐ ─────────────────► ┌───────┐
   │  STOPPED  │                    │ STEP  │◄─────────────────────────┐
   └───────────┘ ◄── STOP (godmode, from any state)                   │
        ▲                              │ on_ms                         │
        │                              ▼                               │
        │          ┌──────── next step? ───────────────┐               │
        │          │ yes                               │ no            │
        │          ▼                                   ▼               │
        │   PULSE → GAP ──gap_ms──► idx++ ──► STEP              │
        │   SWAP, same channel → REPEAT_OFF ──blink_off_ms──► idx++ ──► STEP
        │   SWAP, different channel → idx++ ──► STEP (immediate switch)  │
        │                                                              │
        │                       cycles left? ── yes ──► CYCLE_PAUSE
        │                              │ no                    │ cycle_pause_ms
        └──────────────────────────────┘                        └─► cycle++, idx=0 ─┘
```

| from | event | to | action |
| --- | --- | --- | --- |
| any | STOP | `STOPPED` | relays off immediately; manual test cleared |
| any | START | `STEP` | `idx=0`, `cycle=1`, manual test cleared (empty sequence: stays in `STOPPED`) |
| any | config saved | `STOPPED` | new config applied; the sequence is not swapped in the middle of a cycle |
| `STEP` | `on_ms` and next step exists, PULSE | `GAP` | |
| `STEP` | `on_ms` and next step exists, SWAP, same channel | `REPEAT_OFF` | |
| `STEP` | `on_ms` and next step exists, SWAP, different channel | `STEP` | `idx++` |
| `STEP` | `on_ms`, last step, cycles left | `CYCLE_PAUSE` | |
| `STEP` | `on_ms`, last step, no cycles left | `STOPPED` | |
| `GAP` | `gap_ms` | `STEP` | `idx++` |
| `REPEAT_OFF` | `blink_off_ms` | `STEP` | `idx++` |
| `CYCLE_PAUSE` | `cycle_pause_ms` | `STEP` | `cycle++`, `idx=0` |
| `STOPPED` | manual test | `STOPPED` | the manual mask becomes the output (accepted only in `STOPPED`) |

"Cycles left" means: `repeat=LOOP`, or `repeat=N_TIMES` and `cycle < repeat_count`. In `ONCE`,
there never are.

### 1.3 Boot

`STOPPED` with all relays off. The TCA9554 receives the output register first (`0x00`) and only
afterwards the pin configuration as outputs, so no relay pulses at boot. With `autostart=true`
(default), `SeqRunner::begin()` requests the start before any network code runs.

### 1.4 Timing

Every transition is anchored to the *theoretical* instant the previous state ended (`t0 += duration`), not to
the moment the pass noticed it, so a late pass does not accumulate drift over the steps. If a pass is late by more
than 100 ms the timeline is resynchronized to the current time. The FSM never logs to the serial port: the
`loop()` side watches `revision()` and logs the transitions.

---

## 2. Configuration rules

Validated in `validateSeqConfig()`, the same function for the page, HA and JSON:

- sequence not empty, each step between 1 and 8, free length (`std::vector`);
- `on_ms > 0`, `blink_off_ms > 0`;
- PULSE: `gap_ms > 0` and `cycle_pause_ms > gap_ms`;
- SWAP: `cycle_pause_ms > blink_off_ms`;
- N_TIMES: `repeat_count >= 1`.

Rejected configuration:

- **Page:** HTTP 400 with the reason, which is shown on screen.
- **HA:** the entity republishes the previous value.
- **JSON at boot:** the firmware uses the defaults and logs the reason to the serial log.

`mode` is `PULSE` or `SWAP`; the 1.0.x names `PULSO`/`TROCA` are still accepted when reading a saved file.

Defaults live in the code; the JSON is only an override (`/config.json` and `/network.json` on LittleFS).

`/network.json` (all keys optional; a missing key keeps the default, an invalid file is rejected and logged):

```json
{
  "hostname": "sequencer",
  "admin_pass_hash": "",
  "ap":   { "pass": "" },
  "eth":  { "enabled": true, "dhcp": true, "ip": "", "gateway": "", "netmask": "", "dns": "" },
  "wifi": { "enabled": true, "ssid": "", "pass": "", "dhcp": true, "ip": "", "gateway": "", "netmask": "", "dns": "" },
  "mqtt": { "broker": "", "port": 1883, "user": "", "pass": "" }
}
```

- `ap.pass`: WPA2 password of the configuration AP, 8-63 characters; empty means the default `sequencer1234!`.
- `admin_pass_hash`: written by the page (`salt$sha256(salt+password)`); empty means the default page password.
- MQTT and Home Assistant stay off unless `mqtt.broker` is set.

---

## 3. Interfaces (FSM adapters)

| interface | reads from FSM | writes to FSM |
| --- | --- | --- |
| Web page (`WebUI.h`) | `/api/state` (state, step, cycle, mask), `/api/wifi/scan` (asynchronous scan, does not touch the FSM) | `/api/start`, `/api/stop`, `/api/manual`, `/api/config` |
| Home Assistant (`HA.h`) | sensors State, Step, Cycle, IPs | buttons, selects, numbers, switch, sequence text |
| Serial (USB) | `status` | `start`, `stop` |
| BOOT button | — | Holding it for 10 s restores the default page password (does not touch the FSM) |

**Two independent tasks.** The FSM and the relays run in `SeqRunner.h`, in their own FreeRTOS task
(core 1, priority 10, 1 ms step with `vTaskDelayUntil`). It has exclusive ownership of the FSM, the
relays and the I2C bus, and starts in `setup()` before the network, already with autostart. The
Arduino `loop()` only runs the infrastructure (web, HA, network, serial, BOOT). The HA callbacks and
the HTTP handlers call `requestStart/Stop/Manual` and `setConfig` (atomic requests) and read `snap()`
(snapshot); they never wait for the FSM nor block it. Without network, without a broker, or with the
AP active, the sequence runs the same. The task's worst-case delay appears in `seq_max_late_us`
(`/api/state`) and in the serial `status`.

---

## 4. Network (`Net.h`)

- **Ethernet W5500** (SPI 13/14/15, CS 16, INT 12, RST 39), DHCP or fixed IP, enabled by default.
- **WiFi STA** optional, DHCP or fixed IP.
- **Default route:** when Ethernet gets an IP, `Network.setDefaultInterface(ETH)`. If it goes down,
  the route falls back to WiFi. MQTT follows the default route and reconnects on its own through
  ArduinoHA.
- **AP + captive portal:** comes up when no interface has an IP for 10 s in a row, in pure AP mode
  (the STA stays idle: while searching for a missing network it makes the radio switch channels and
  drops the AP). Modem sleep is disabled (`WiFi.setSleep(false)`), because the core enables it by default and it can make TCP handshakes on the AP/STA be refused for seconds. While the AP is up, the saved WiFi network is retried every 60 s for up to 10 s, and
  only when there are no clients on the AP. MQTT/HA only try to connect when a broker is configured
  and there is an IP on some interface. SSID `Sequencer-XXXX` (last 4 hex digits of the MAC), WPA2
  password from `network.json` (`ap.pass`, 8-63 characters; empty = default `sequencer1234!`), `192.168.4.1`, DNS answering everything with the board's address.
  It goes down as soon as Ethernet or WiFi has an IP.
- **mDNS:** `http://<hostname>.local`.
- **Page and API:** a single WebServer on port 80, which responds on all interfaces.
- **Saving the network settings from the page** writes `/network.json` and restarts the board. The
  network configuration is read at boot.

---

## 5. Page access

- Login via `POST /login` (form) and session cookie `seq_sess`, `HttpOnly` and `SameSite=Strict`.
  The password is never sent in the URL.
- Public: only the login screen and `/logo.svg`. All `/api/*` routes require a session.
- Default password `sequencer1234!`. When changed through the page, it stores
  `salt$sha256(salt+password)` in `network.json.admin_pass_hash`. An empty hash means the default
  password.
- Sessions are kept in RAM (8 slots; the oldest is discarded). They are lost on reboot, and the GM
  logs in again.
- The AP WiFi password is independent of the page password: it comes from `ap.pass` in `network.json`.

---

## 6. Home Assistant (ArduinoHA 2.1.0), optional

Configuration and operation are done through the **board's own web server** (§3–§5), without
depending on Home Assistant. HA is an optional mirror: it is only enabled when an MQTT broker is
configured in `network.json`. Without a broker, the board works the same.

Device with an id derived from the MAC, shared availability and last will.

| entity | type | id |
| --- | --- | --- |
| Start / Stop | button | `start`, `stop` |
| State | sensor | `state` |
| Step / Cycle | sensor | `step` (`3/10`), `cycle` (`2/5` or `2/∞`) |
| Light mode | select | `mode` (PULSE;SWAP) |
| Repeat | select | `repeat` (ONCE;N_TIMES;LOOP) |
| On time / Time between steps / Time between cycles | number (s, 0.1) | `on_s`, `gap_s`, `cycle_s` |
| Cycles (N) | number | `repeat_count` |
| Autostart | switch | `autostart` |
| IP Ethernet / IP WiFi | sensor | `ip_eth`, `ip_wifi` |
| Sequence | text (own discovery) | `sequence` |

ArduinoHA 2.1.0 has no `text` entity, so the sequence is published with its own discovery
(`homeassistant/text/<id>_sequence/config`) and subscribed through `HAMqtt::onMessage`. Home
Assistant itself limits `text` entities to 255 characters (about 127 steps). Longer sequences are
only possible through the page; this is a HA limit, not a firmware one.

---

## 7. Files

| file | content |
| --- | --- |
| `firmware/ProgrammableSequencer/ProgrammableSequencer.ino` | `setup()` / `loop()`, serial, BOOT button |
| `Pins.h` | board pinout |
| `Relays.h` | TCA9554 driver (0x20), 8-bit mask, active high |
| `Config.h` | `SeqConfig`, `NetConfig`, defaults, validation, JSON on LittleFS, password hash |
| `SeqRunner.h` | the sequencer task: owns the FSM, the relays and I2C; atomic requests in, snapshot out |
| `Sequencer.h` | the FSM |
| `Net.h` | Ethernet, WiFi, AP, captive DNS, mDNS, default route |
| `WebUI.h` | HTTP server, sessions, API |
| `web_index.h` | embedded page (HTML/CSS/JS) |
| `logo_svg.h` | logo as gzipped SVG (generated from the Nicolau dos Brinquedos stamp SVG, which is not included in this repository) |
| `HA.h` | Home Assistant entities |
| `partitions.csv` | 16 MB: app 4 MB ×2 + LittleFS |

### Build

```
arduino-cli compile --fqbn esp32:esp32:esp32s3:FlashSize=16M,PSRAM=opi,CDCOnBoot=cdc,PartitionScheme=custom \
  firmware/ProgrammableSequencer
arduino-cli upload -p /dev/ttyACM0 --fqbn <same fqbn> firmware/ProgrammableSequencer
```

Libraries: ArduinoJson 7.x, home-assistant-integration (ArduinoHA) 2.1.0 with PubSubClient 2.8.
ESP32 Arduino core 3.3.12 (see `DEPENDENCIES.md`).
