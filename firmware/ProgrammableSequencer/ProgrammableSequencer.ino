// Programmable Sequencer — programmable relay sequencer for escape rooms
// Board: Waveshare ESP32-S3-ETH-8DI-8RO-C. Documentation: docs/ (ARCHITECTURE.md, BOARD.md).
// Nicolau dos Brinquedos — Djair Guilherme

#include "Config.h"
#include "SeqRunner.h"
#include "Net.h"
#include "WebUI.h"
#include "HA.h"

const char* resetReasonName() {
  switch (esp_reset_reason()) {
    case ESP_RST_POWERON:  return "POWERON (power-on)";
    case ESP_RST_SW:       return "SW (software restart)";
    case ESP_RST_PANIC:    return "PANIC (exception/crash)";
    case ESP_RST_INT_WDT:  return "INT_WDT (interrupt watchdog)";
    case ESP_RST_TASK_WDT: return "TASK_WDT (task watchdog)";
    case ESP_RST_WDT:      return "WDT (watchdog)";
    case ESP_RST_BROWNOUT: return "BROWNOUT (voltage drop)";
    case ESP_RST_USB:      return "USB (reset over USB)";
    case ESP_RST_EXT:      return "EXT (reset pin)";
    case ESP_RST_DEEPSLEEP:return "DEEPSLEEP";
    default:               return "other";
  }
}

ConfigStore store;
SeqRunner   fsm;     // its own task: owner of the FSM, the relays and I2C
Net         net;

void onSeqConfigChanged() { if (ha::connected()) ha::publishConfig(true); }
bool mqttConnected() { return ha::connected(); }

AppCtx app{store, fsm, net, onSeqConfigChanged, mqttConnected};
WebUI  web(app);

// ── Serial: start / stop / status ──
void serialCommands() {
  static String line;
  while (Serial.available()) {
    char c = Serial.read();
    if (c != '\n' && c != '\r') { if (line.length() < 64) line += c; continue; }
    line.trim();
    if (line == "start") { Serial.println("[CMD] start (serial)"); fsm.requestStart(); }
    else if (line == "stop") { Serial.println("[CMD] stop (serial)"); fsm.requestStop(); }
    else if (line == "status") {
      SeqRunner::Snap s = fsm.snap();
      Serial.printf("[STATUS] %s step %u/%u cycle %lu relays 0x%02X max late %lu us | eth %s wifi %s ap %s mqtt %s\n",
                    seqStateName(s.state), (unsigned)s.idx + 1, (unsigned)store.seq.sequence.size(),
                    (unsigned long)s.cycle, s.relays, (unsigned long)s.maxLateUs, net.ethIp().c_str(), net.wifiIp().c_str(),
                    net.apOn() ? net.apSsid().c_str() : "-", ha::connected() ? "ok" : "-");
    } else if (line.length()) Serial.println("[SERIAL] commands: start | stop | status");
    line = "";
  }
}

// ── BOOT held for 10 s: page password goes back to the default ──
void bootButton() {
  static uint32_t pressedAt = 0;
  static bool done = false;
  bool pressed = digitalRead(PIN_BOOT) == LOW;
  if (!pressed) { pressedAt = 0; done = false; return; }
  if (!pressedAt) pressedAt = millis();
  if (!done && millis() - pressedAt >= 10000) {
    done = true;
    store.net.admin_pass_hash = "";
    store.saveNet();
    Serial.println("[BOOT] page password restored to default");
    for (int i = 0; i < 3; i++) { rgbLedWrite(PIN_RGB, 0, 40, 0); delay(150); rgbLedWrite(PIN_RGB, 0, 0, 0); delay(150); }
  }
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.printf("\n[BOOT] Quadros Sequenciais %s\n", FW_VERSION);
  Serial.printf("[BOOT] reset reason: %s\n", resetReasonName());

  pinMode(PIN_BOOT, INPUT_PULLUP);
  rgbLedWrite(PIN_RGB, 0, 0, 0);

  // The sequencer starts first and alone: relays off and, with autostart, the sequence
  // is already running before any network exists. From here on no network code can reach it.
  store.begin();
  if (store.seq.autostart) Serial.println("[BOOT] autostart");
  fsm.begin(store.seq);

  net.begin(store.net);
  web.begin();
  ha::begin(app);       // only starts if a broker is configured
  ha_connects = [] { return ha::connects; };
}

// Logs FSM transitions (the sequencer task never writes to the serial port).
void logFsm() {
  static uint32_t lastRev = 0;
  static SeqState lastState = SeqState::PARADO;
  SeqRunner::Snap s = fsm.snap();
  if (s.rev == lastRev) return;
  lastRev = s.rev;
  if (s.state != lastState) {
    Serial.printf("[FSM] %s -> %s\n", seqStateName(lastState), seqStateName(s.state));
    lastState = s.state;
  }
}

void loop() {
  serialCommands();
  web.loop();
  ha::loop();
  net.loop();
  bootButton();
  logFsm();
  delay(1);
}
