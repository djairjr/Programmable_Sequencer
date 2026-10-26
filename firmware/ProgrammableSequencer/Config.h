#pragma once
// Configuration: defaults in code, JSON on LittleFS as override (Config JSON Pattern).
//   /config.json  -> SeqConfig (sequence, mode, timings, repeat, autostart, names)
//   /network.json -> NetConfig (hostname, page password, Ethernet, WiFi, MQTT)

#include <Arduino.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <vector>
#include <mbedtls/sha256.h>
#include <esp_random.h>
#include "Pins.h"

static const char* FW_VERSION         = "1.1.0";
static const char* DEFAULT_ADMIN_PASS = "sequencer1234!";
static const char* DEFAULT_AP_PASS     = "sequencer1234!";   // used when network.json has no ap.pass

// ───────────────────────── Sequence ─────────────────────────

enum class LightMode : uint8_t { PULSO, TROCA };
enum class RepeatMode : uint8_t { ONCE, N_TIMES, LOOP };

static const char* lightModeName(LightMode m) { return m == LightMode::PULSO ? "PULSO" : "TROCA"; }
static const char* repeatModeName(RepeatMode r) {
  switch (r) { case RepeatMode::ONCE: return "ONCE"; case RepeatMode::N_TIMES: return "N_TIMES"; default: return "LOOP"; }
}
static bool parseLightMode(const char* s, LightMode& out) {
  if (!s) return false;
  if (!strcmp(s, "PULSO")) { out = LightMode::PULSO; return true; }
  if (!strcmp(s, "TROCA")) { out = LightMode::TROCA; return true; }
  return false;
}
static bool parseRepeatMode(const char* s, RepeatMode& out) {
  if (!s) return false;
  if (!strcmp(s, "ONCE"))    { out = RepeatMode::ONCE; return true; }
  if (!strcmp(s, "N_TIMES")) { out = RepeatMode::N_TIMES; return true; }
  if (!strcmp(s, "LOOP"))    { out = RepeatMode::LOOP; return true; }
  return false;
}

struct SeqConfig {
  std::vector<uint8_t> sequence{4, 4, 6, 7, 8, 1, 2, 5, 5, 6};
  LightMode  mode = LightMode::PULSO;
  uint32_t   on_ms = 2000;
  uint32_t   gap_ms = 500;
  uint32_t   cycle_pause_ms = 5000;
  uint32_t   blink_off_ms = 300;
  RepeatMode repeat = RepeatMode::LOOP;
  uint32_t   repeat_count = 3;
  bool       autostart = true;
  String     channel_names[NUM_CHANNELS] = {"Quadro 1", "Quadro 2", "Quadro 3", "Quadro 4",
                                            "Quadro 5", "Quadro 6", "Quadro 7", "Quadro 8"};
};

// Returns nullptr if valid, or the reason it was refused (shown on the page, in Portuguese).
static const char* validateSeqConfig(const SeqConfig& c) {
  if (c.sequence.empty()) return "A sequencia esta vazia.";
  for (uint8_t ch : c.sequence)
    if (ch < 1 || ch > NUM_CHANNELS) return "Cada passo deve ser um canal de 1 a 8.";
  if (c.on_ms == 0) return "O tempo aceso deve ser maior que zero.";
  if (c.blink_off_ms == 0) return "A apagada de repeticao deve ser maior que zero.";
  if (c.mode == LightMode::PULSO) {
    if (c.gap_ms == 0) return "No modo PULSO o tempo entre acionamentos deve ser maior que zero.";
    if (c.cycle_pause_ms <= c.gap_ms) return "O tempo entre ciclos deve ser maior que o tempo entre acionamentos.";
  } else {
    if (c.cycle_pause_ms <= c.blink_off_ms) return "O tempo entre ciclos deve ser maior que a apagada de repeticao.";
  }
  if (c.repeat == RepeatMode::N_TIMES && c.repeat_count < 1) return "O numero de ciclos deve ser pelo menos 1.";
  return nullptr;
}

static String sequenceToText(const std::vector<uint8_t>& seq) {
  String s;
  for (size_t i = 0; i < seq.size(); i++) { if (i) s += ','; s += seq[i]; }
  return s;
}

// Accepts "4,4,6 7;8" etc. Returns false if anything is not a channel 1-8.
static bool textToSequence(const char* txt, std::vector<uint8_t>& out) {
  out.clear();
  if (!txt) return false;
  int cur = -1;
  for (const char* p = txt;; p++) {
    char c = *p;
    if (c >= '0' && c <= '9') {
      cur = (cur < 0 ? 0 : cur * 10) + (c - '0');
      if (cur > 255) return false;
    } else {
      if (cur >= 0) { if (cur < 1 || cur > NUM_CHANNELS) return false; out.push_back((uint8_t)cur); cur = -1; }
      if (c == 0) break;
      if (c != ',' && c != ' ' && c != ';' && c != '\n' && c != '\r' && c != '\t') return false;
    }
  }
  return true;
}

static void seqToJson(const SeqConfig& c, JsonDocument& d) {
  JsonArray a = d["sequence"].to<JsonArray>();
  for (uint8_t ch : c.sequence) a.add(ch);
  d["mode"] = lightModeName(c.mode);
  d["on_ms"] = c.on_ms;
  d["gap_ms"] = c.gap_ms;
  d["cycle_pause_ms"] = c.cycle_pause_ms;
  d["blink_off_ms"] = c.blink_off_ms;
  d["repeat"] = repeatModeName(c.repeat);
  d["repeat_count"] = c.repeat_count;
  d["autostart"] = c.autostart;
  JsonArray n = d["channel_names"].to<JsonArray>();
  for (uint8_t i = 0; i < NUM_CHANNELS; i++) n.add(c.channel_names[i]);
}

// Applies the fields present in `d` over `c` (override). Returns nullptr or the format error.
static const char* seqFromJson(JsonVariantConst d, SeqConfig& c) {
  if (d["sequence"].is<JsonArrayConst>()) {
    c.sequence.clear();
    for (JsonVariantConst v : d["sequence"].as<JsonArrayConst>()) {
      if (!v.is<int>()) return "Sequencia com valor nao numerico.";
      int ch = v.as<int>();
      if (ch < 1 || ch > NUM_CHANNELS) return "Cada passo deve ser um canal de 1 a 8.";
      c.sequence.push_back((uint8_t)ch);
    }
  } else if (d["sequence"].is<const char*>()) {
    if (!textToSequence(d["sequence"].as<const char*>(), c.sequence)) return "Sequencia invalida: use canais de 1 a 8 separados por virgula.";
  }
  if (d["mode"].is<const char*>() && !parseLightMode(d["mode"], c.mode)) return "Modo invalido.";
  if (d["repeat"].is<const char*>() && !parseRepeatMode(d["repeat"], c.repeat)) return "Repeticao invalida.";
  auto u32 = [&](const char* k, uint32_t& dst) -> bool {
    if (d[k].isNull()) return true;
    if (!d[k].is<long long>() && !d[k].is<double>()) return false;
    double v = d[k].as<double>();
    if (v < 0) return false;
    dst = (uint32_t)v;
    return true;
  };
  if (!u32("on_ms", c.on_ms) || !u32("gap_ms", c.gap_ms) || !u32("cycle_pause_ms", c.cycle_pause_ms) ||
      !u32("blink_off_ms", c.blink_off_ms) || !u32("repeat_count", c.repeat_count))
    return "Tempo ou contagem invalidos.";
  if (d["autostart"].is<bool>()) c.autostart = d["autostart"].as<bool>();
  if (d["channel_names"].is<JsonArrayConst>()) {
    uint8_t i = 0;
    for (JsonVariantConst v : d["channel_names"].as<JsonArrayConst>()) {
      if (i >= NUM_CHANNELS) break;
      if (v.is<const char*>()) c.channel_names[i] = v.as<const char*>();
      i++;
    }
  }
  return nullptr;
}

// ───────────────────────── Network ─────────────────────────

struct IfaceConfig {
  bool enabled = true;
  bool dhcp = true;
  String ip, gateway, netmask, dns;
};

struct NetConfig {
  String hostname = "quadros";
  String admin_pass_hash;            // empty = default password
  String ap_pass;                    // configuration-AP WiFi password; empty = DEFAULT_AP_PASS
  IfaceConfig eth;
  IfaceConfig wifi;
  String wifi_ssid, wifi_pass;
  String mqtt_broker, mqtt_user, mqtt_pass;
  uint16_t mqtt_port = 1883;
};

static void ifaceToJson(const IfaceConfig& i, JsonObject o) {
  o["enabled"] = i.enabled; o["dhcp"] = i.dhcp;
  o["ip"] = i.ip; o["gateway"] = i.gateway; o["netmask"] = i.netmask; o["dns"] = i.dns;
}
static void ifaceFromJson(JsonVariantConst o, IfaceConfig& i) {
  if (o.isNull()) return;
  if (o["enabled"].is<bool>()) i.enabled = o["enabled"];
  if (o["dhcp"].is<bool>()) i.dhcp = o["dhcp"];
  if (o["ip"].is<const char*>()) i.ip = o["ip"].as<const char*>();
  if (o["gateway"].is<const char*>()) i.gateway = o["gateway"].as<const char*>();
  if (o["netmask"].is<const char*>()) i.netmask = o["netmask"].as<const char*>();
  if (o["dns"].is<const char*>()) i.dns = o["dns"].as<const char*>();
}

// `withSecrets=false` is what goes to the page: passwords never leave the board.
static void netToJson(const NetConfig& n, JsonDocument& d, bool withSecrets) {
  d["hostname"] = n.hostname;
  if (withSecrets) d["admin_pass_hash"] = n.admin_pass_hash;
  JsonObject a = d["ap"].to<JsonObject>();
  if (withSecrets) a["pass"] = n.ap_pass; else a["pass_set"] = n.ap_pass.length() > 0;
  ifaceToJson(n.eth, d["eth"].to<JsonObject>());
  JsonObject w = d["wifi"].to<JsonObject>();
  ifaceToJson(n.wifi, w);
  w["ssid"] = n.wifi_ssid;
  if (withSecrets) w["pass"] = n.wifi_pass; else w["pass_set"] = n.wifi_pass.length() > 0;
  JsonObject m = d["mqtt"].to<JsonObject>();
  m["broker"] = n.mqtt_broker; m["port"] = n.mqtt_port; m["user"] = n.mqtt_user;
  if (withSecrets) m["pass"] = n.mqtt_pass; else m["pass_set"] = n.mqtt_pass.length() > 0;
}

// Passwords absent (or null) in the JSON coming from the page keep the current value.
static void netFromJson(JsonVariantConst d, NetConfig& n, bool allowHash) {
  if (d["hostname"].is<const char*>() && strlen(d["hostname"]) > 0) n.hostname = d["hostname"].as<const char*>();
  if (allowHash && d["admin_pass_hash"].is<const char*>()) n.admin_pass_hash = d["admin_pass_hash"].as<const char*>();
  if (d["ap"]["pass"].is<const char*>()) n.ap_pass = d["ap"]["pass"].as<const char*>();
  ifaceFromJson(d["eth"], n.eth);
  ifaceFromJson(d["wifi"], n.wifi);
  if (d["wifi"]["ssid"].is<const char*>()) n.wifi_ssid = d["wifi"]["ssid"].as<const char*>();
  if (d["wifi"]["pass"].is<const char*>()) n.wifi_pass = d["wifi"]["pass"].as<const char*>();
  JsonVariantConst m = d["mqtt"];
  if (m["broker"].is<const char*>()) n.mqtt_broker = m["broker"].as<const char*>();
  if (m["port"].is<int>()) n.mqtt_port = m["port"].as<int>();
  if (m["user"].is<const char*>()) n.mqtt_user = m["user"].as<const char*>();
  if (m["pass"].is<const char*>()) n.mqtt_pass = m["pass"].as<const char*>();
}

static const char* validateNetConfig(const NetConfig& n) {
  for (char c : n.hostname)
    if (!isalnum((unsigned char)c) && c != '-') return "O hostname aceita so letras, numeros e hifen.";
  // WPA2 passphrase: 8-63 characters (empty keeps the default). Message is shown on the page, in Portuguese.
  if (n.ap_pass.length() && (n.ap_pass.length() < 8 || n.ap_pass.length() > 63)) return "A senha do AP deve ter de 8 a 63 caracteres.";
  IPAddress tmp;
  for (const IfaceConfig* i : {&n.eth, &n.wifi}) {
    if (!i->enabled || i->dhcp) continue;
    if (!tmp.fromString(i->ip) || !tmp.fromString(i->gateway) || !tmp.fromString(i->netmask))
      return "IP fixo exige IP, gateway e mascara validos.";
    if (i->dns.length() && !tmp.fromString(i->dns)) return "DNS invalido.";
  }
  return nullptr;
}

// ───────────────────────── Page password ─────────────────────────

static String sha256Hex(const String& s) {
  uint8_t out[32];
  mbedtls_sha256((const unsigned char*)s.c_str(), s.length(), out, 0);
  char hex[65];
  for (int i = 0; i < 32; i++) sprintf(hex + i * 2, "%02x", out[i]);
  return String(hex);
}

static String makePassHash(const String& pass) {
  char salt[17];
  for (int i = 0; i < 8; i++) sprintf(salt + i * 2, "%02x", (uint8_t)esp_random());
  return String(salt) + "$" + sha256Hex(String(salt) + pass);
}

static bool checkPass(const NetConfig& n, const String& pass) {
  if (n.admin_pass_hash.isEmpty()) return pass == DEFAULT_ADMIN_PASS;
  int sep = n.admin_pass_hash.indexOf('$');
  if (sep <= 0) return pass == DEFAULT_ADMIN_PASS;
  String salt = n.admin_pass_hash.substring(0, sep);
  return sha256Hex(salt + pass) == n.admin_pass_hash.substring(sep + 1);
}

// ───────────────────────── Persistence ─────────────────────────

class ConfigStore {
public:
  SeqConfig seq;
  NetConfig net;

  void begin() {
    if (!LittleFS.begin(true)) Serial.println("[CFG] LittleFS failed to mount; using defaults");
    loadSeq();
    loadNet();
  }

  void loadSeq() {
    seq = SeqConfig();
    JsonDocument d;
    if (!readJson("/config.json", d)) { Serial.println("[CFG] /config.json missing: using defaults"); return; }
    SeqConfig c;
    const char* err = seqFromJson(d.as<JsonVariantConst>(), c);
    if (!err) err = validateSeqConfig(c);
    if (err) { Serial.printf("[CFG] /config.json rejected (%s): using defaults\n", err); return; }
    seq = c;
    Serial.printf("[CFG] sequence: %s (%u steps)\n", sequenceToText(seq.sequence).c_str(), (unsigned)seq.sequence.size());
  }

  void loadNet() {
    net = NetConfig();
    JsonDocument d;
    if (!readJson("/network.json", d)) { Serial.println("[CFG] /network.json missing: using defaults"); return; }
    NetConfig n;
    netFromJson(d.as<JsonVariantConst>(), n, true);
    const char* err = validateNetConfig(n);
    if (err) { Serial.printf("[CFG] /network.json rejected (%s): using defaults\n", err); return; }
    net = n;
  }

  bool saveSeq() { JsonDocument d; seqToJson(seq, d); return writeJson("/config.json", d); }
  bool saveNet() { JsonDocument d; netToJson(net, d, true); return writeJson("/network.json", d); }

private:
  bool readJson(const char* path, JsonDocument& d) {
    File f = LittleFS.open(path, "r");
    if (!f) return false;
    DeserializationError e = deserializeJson(d, f);
    f.close();
    if (e) { Serial.printf("[CFG] %s: invalid JSON (%s)\n", path, e.c_str()); return false; }
    return true;
  }
  bool writeJson(const char* path, const JsonDocument& d) {
    String tmp = String(path) + ".tmp";
    File f = LittleFS.open(tmp, "w");
    if (!f) return false;
    bool ok = serializeJson(d, f) > 0;
    f.close();
    if (!ok) return false;
    LittleFS.remove(path);
    return LittleFS.rename(tmp, path);
  }
};
