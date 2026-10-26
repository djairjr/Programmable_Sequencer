#pragma once
// Home Assistant via MQTT Discovery (ArduinoHA 2.1.0), optional: only active with a broker in network.json.
// Bidirectional: every command is validated, applied, saved and published back (HA Integration Pattern).
// The callbacks run inside mqtt.loop(), on the loop() task, and only post requests to the sequencer / config.

#include <WiFi.h>
#include <ArduinoHA.h>
#include "WebUI.h"

namespace ha {

// Order matters: HAMqtt before the entities (they register themselves in the constructor).
WiFiClient client;   // NetworkClient: follows the default route (Ethernet or WiFi)
HADevice   device;
HAMqtt     mqtt(client, device);

HAButton  bStart("start"), bStop("stop");
HASensor  sState("state"), sStep("step"), sCycle("cycle"), sIpEth("ip_eth"), sIpWifi("ip_wifi");
HASelect  selMode("mode"), selRepeat("repeat");
HANumber  nOn("on_s", HANumber::PrecisionP1), nGap("gap_s", HANumber::PrecisionP1), nCycle("cycle_s", HANumber::PrecisionP1), nCount("repeat_count");
HASwitch  swAuto("autostart");

AppCtx*  ctx = nullptr;
bool     enabled = false;
uint32_t lastRev = 0xFFFFFFFF;
uint32_t lastIpCheck = 0;
uint32_t connects = 0;
String   lastEthIp = "-", lastWifiIp = "-";
String   seqCmdTopic, seqStateTopic;

const char* modeOptions   = "PULSE;SWAP";
const char* repeatOptions = "ONCE;N_TIMES;LOOP";

void publishFsm() {
  const SeqConfig& c = ctx->store.seq;
  SeqRunner::Snap s = ctx->seq.snap();
  sState.setValue(seqStateName(s.state));
  char buf[24];
  if (s.state == SeqState::STOPPED) { sStep.setValue("-"); sCycle.setValue("-"); return; }
  snprintf(buf, sizeof(buf), "%u/%u", (unsigned)s.idx + 1, (unsigned)c.sequence.size());
  sStep.setValue(buf);
  if (c.repeat == RepeatMode::LOOP) snprintf(buf, sizeof(buf), "%lu/\xE2\x88\x9E", (unsigned long)s.cycle);
  else snprintf(buf, sizeof(buf), "%lu/%lu", (unsigned long)s.cycle, (unsigned long)(c.repeat == RepeatMode::ONCE ? 1 : c.repeat_count));
  sCycle.setValue(buf);
}

void publishConfig(bool force) {
  const SeqConfig& c = ctx->store.seq;
  selMode.setState((int8_t)c.mode, force);
  selRepeat.setState((int8_t)c.repeat, force);
  nOn.setState(c.on_ms / 1000.0f, force);
  nGap.setState(c.gap_ms / 1000.0f, force);
  nCycle.setState(c.cycle_pause_ms / 1000.0f, force);
  nCount.setState((int32_t)c.repeat_count, force);
  swAuto.setState(c.autostart, force);
  if (mqtt.isConnected()) mqtt.publish(seqStateTopic.c_str(), sequenceToText(c.sequence).c_str(), true);
}

void publishIps(bool force) {
  String e = ctx->net.ethIp(), w = ctx->net.wifiIp();
  if (force || e != lastEthIp)  { sIpEth.setValue(e.length() ? e.c_str() : "-");  lastEthIp = e; }
  if (force || w != lastWifiIp) { sIpWifi.setValue(w.length() ? w.c_str() : "-"); lastWifiIp = w; }
}

// Applies a candidate config coming from HA. Refused: republishes the current state (previous value).
void applyCandidate(const SeqConfig& c) {
  const char* err = validateSeqConfig(c);
  if (err) {
    Serial.printf("[HA] rejected: %s\n", err);
    publishConfig(true);
    return;
  }
  ctx->store.seq = c;
  ctx->seq.setConfig(c);    // new config: the sequencer adopts it and goes to STOPPED, before the flash write
  ctx->store.saveSeq();
  publishConfig(true);
}

void onStart(HAButton*) { Serial.println("[CMD] start (HA)"); ctx->seq.requestStart(); }
void onStop(HAButton*)  { Serial.println("[CMD] stop (HA)"); ctx->seq.requestStop(); }

void onSelect(int8_t idx, HASelect* sender) {
  SeqConfig c = ctx->store.seq;
  if (sender == &selMode) { if (idx < 0 || idx > 1) { publishConfig(true); return; } c.mode = (LightMode)idx; }
  else                    { if (idx < 0 || idx > 2) { publishConfig(true); return; } c.repeat = (RepeatMode)idx; }
  applyCandidate(c);
}

void onNumber(HANumeric v, HANumber* sender) {
  SeqConfig c = ctx->store.seq;
  if (!v.isSet()) { publishConfig(true); return; }
  float f = v.toFloat();
  if (f < 0) { publishConfig(true); return; }
  uint32_t ms = (uint32_t)lroundf(f * 1000.0f);
  if (sender == &nOn) c.on_ms = ms;
  else if (sender == &nGap) c.gap_ms = ms;
  else if (sender == &nCycle) c.cycle_pause_ms = ms;
  else if (sender == &nCount) c.repeat_count = (uint32_t)lroundf(f);
  applyCandidate(c);
}

void onSwitch(bool state, HASwitch*) {
  SeqConfig c = ctx->store.seq;
  c.autostart = state;
  applyCandidate(c);
}

// Sequence: `text` entity with its own discovery (ArduinoHA 2.1.0 has no HAText).
void onMessage(const char* topic, const uint8_t* payload, uint16_t len) {
  if (seqCmdTopic != topic) return;
  String txt;
  txt.reserve(len);
  for (uint16_t i = 0; i < len; i++) txt += (char)payload[i];
  SeqConfig c = ctx->store.seq;
  if (!textToSequence(txt.c_str(), c.sequence)) {
    Serial.println("[HA] sequence rejected: invalid format");
    publishConfig(true);
    return;
  }
  applyCandidate(c);
}

void onConnected() {
  const char* id = device.getUniqueId();
  JsonDocument d;
  d["name"] = "Sequence";
  d["uniq_id"] = String(id) + "_sequence";
  d["cmd_t"] = seqCmdTopic;
  d["stat_t"] = seqStateTopic;
  d["max"] = 255;                       // HA's own limit for text entities
  d["pattern"] = "^[1-8]([ ,;]+[1-8])*$";
  d["ic"] = "mdi:format-list-numbered";
  if (device.getAvailabilityTopic()) d["avty_t"] = device.getAvailabilityTopic();
  d["dev"]["ids"] = id;
  String out;
  serializeJson(d, out);
  String disc = String("homeassistant/text/") + id + "_sequence/config";
  mqtt.publish(disc.c_str(), out.c_str(), true);
  mqtt.subscribe(seqCmdTopic.c_str());

  publishConfig(true);
  publishFsm();
  publishIps(true);
  Serial.println("[HA] connected to broker");
  connects++;
}

void begin(AppCtx& app) {
  ctx = &app;
  const NetConfig& n = app.store.net;
  enabled = n.mqtt_broker.length() > 0;
  if (!enabled) { Serial.println("[HA] no broker configured: Home Assistant disabled"); return; }

  uint8_t mac[6];
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  device.setUniqueId(mac, sizeof(mac));
  device.setName("Programmable Sequencer");
  device.setManufacturer("Nicolau dos Brinquedos");
  device.setModel("Waveshare ESP32-S3-ETH-8DI-8RO-C");
  device.setSoftwareVersion(FW_VERSION);
  device.enableSharedAvailability();
  device.enableLastWill();

  seqCmdTopic   = String("aha/") + device.getUniqueId() + "/sequence/cmd_t";
  seqStateTopic = String("aha/") + device.getUniqueId() + "/sequence/stat_t";

  bStart.setName("Start"); bStart.setIcon("mdi:play"); bStart.onCommand(onStart);
  bStop.setName("Stop");    bStop.setIcon("mdi:stop");  bStop.onCommand(onStop);

  sState.setName("State");      sState.setIcon("mdi:state-machine");
  sStep.setName("Step");        sStep.setIcon("mdi:debug-step-over");
  sCycle.setName("Cycle");       sCycle.setIcon("mdi:repeat");
  sIpEth.setName("IP Ethernet"); sIpEth.setIcon("mdi:ethernet");
  sIpWifi.setName("IP WiFi");    sIpWifi.setIcon("mdi:wifi");

  selMode.setName("Light mode"); selMode.setOptions(modeOptions);   selMode.setIcon("mdi:lightbulb-on");
  selRepeat.setName("Repeat");         selRepeat.setOptions(repeatOptions); selRepeat.setIcon("mdi:repeat-variant");
  selMode.onCommand(onSelect); selRepeat.onCommand(onSelect);

  for (HANumber* num : {&nOn, &nGap, &nCycle}) {
    num->setMin(0); num->setMax(3600); num->setStep(0.1f);
    num->setUnitOfMeasurement("s"); num->setMode(HANumber::ModeBox); num->onCommand(onNumber);
  }
  nOn.setName("On time");             nOn.setIcon("mdi:timer-sand");
  nGap.setName("Time between steps"); nGap.setIcon("mdi:timer-pause");
  nCycle.setName("Time between cycles");   nCycle.setIcon("mdi:timer-refresh");
  nCount.setName("Cycles (N)"); nCount.setMin(1); nCount.setMax(100000); nCount.setStep(1);
  nCount.setMode(HANumber::ModeBox); nCount.setIcon("mdi:counter"); nCount.onCommand(onNumber);

  swAuto.setName("Autostart"); swAuto.setIcon("mdi:power"); swAuto.onCommand(onSwitch);

  mqtt.setBufferSize(1024);
  mqtt.onMessage(onMessage);
  mqtt.onConnected(onConnected);
  const char* user = n.mqtt_user.length() ? n.mqtt_user.c_str() : nullptr;
  const char* pass = n.mqtt_pass.length() ? n.mqtt_pass.c_str() : nullptr;
  mqtt.begin(n.mqtt_broker.c_str(), n.mqtt_port, user, pass);
  Serial.printf("[HA] broker %s:%u\n", n.mqtt_broker.c_str(), n.mqtt_port);
}

void loop() {
  if (!enabled) return;
  // No IP on any interface means no broker to reach: do not even try to connect.
  if (!ctx->net.online()) return;
  mqtt.loop();
  if (!mqtt.isConnected()) return;
  if (ctx->seq.revision() != lastRev) { lastRev = ctx->seq.revision(); publishFsm(); }
  if (millis() - lastIpCheck > 5000) { lastIpCheck = millis(); publishIps(false); }
}

bool connected() { return enabled && mqtt.isConnected(); }

}  // namespace ha
