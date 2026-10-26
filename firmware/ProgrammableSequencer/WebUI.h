#pragma once
// Configuration web server (port 80, all interfaces): login, API and page (docs/ARCHITECTURE.md §3 and §5).

#include <WebServer.h>
#include "Config.h"
#include "SeqRunner.h"
#include "Net.h"
#include "web_index.h"
#include "logo_svg.h"

struct AppCtx {
  ConfigStore& store;
  SeqRunner&   seq;
  Net&         net;
  void (*onSeqConfigChanged)();   // tells HA to republish
  bool (*mqttConnected)();
};
uint32_t (*ha_connects)() = nullptr;

class WebUI {
public:
  explicit WebUI(AppCtx& ctx) : ctx_(ctx), server_(80) {}

  void begin() {
    static const char* hdrs[] = {"Cookie", "Host"};
    server_.collectHeaders(hdrs, 2);

    server_.on("/", HTTP_GET, [this] { handleRoot(); });
    server_.on("/login", HTTP_POST, [this] { handleLogin(); });
    server_.on("/logout", HTTP_POST, [this] { handleLogout(); });
    server_.on("/logo.svg", HTTP_GET, [this] { handleLogo(); });

    server_.on("/api/state", HTTP_GET, [this] { if (auth()) handleState(); });
    server_.on("/api/config", HTTP_GET, [this] { if (auth()) handleGetConfig(); });
    server_.on("/api/config", HTTP_POST, [this] { if (auth()) handlePostConfig(); });
    server_.on("/api/start", HTTP_POST, [this] { if (auth()) { Serial.println("[CMD] start (web)"); ctx_.seq.requestStart(); sendOk(); } });
    server_.on("/api/stop", HTTP_POST, [this] { if (auth()) { Serial.println("[CMD] stop (web)"); ctx_.seq.requestStop(); sendOk(); } });
    server_.on("/api/manual", HTTP_POST, [this] { if (auth()) handleManual(); });
    server_.on("/api/network", HTTP_GET, [this] { if (auth()) handleGetNetwork(); });
    server_.on("/api/network", HTTP_POST, [this] { if (auth()) handlePostNetwork(); });
    server_.on("/api/password", HTTP_POST, [this] { if (auth()) handlePassword(); });
    server_.on("/api/wifi/scan", HTTP_GET, [this] { if (auth()) handleWifiScan(); });
    server_.onNotFound([this] { handleNotFound(); });

    server_.begin();
    Serial.println("[WEB] server on port 80");
  }

  void loop() {
    server_.handleClient();
    if (restartAt_ && millis() - restartAt_ > 1500) { Serial.println("[WEB] restarting to apply the network settings"); ESP.restart(); }
  }

private:
  AppCtx& ctx_;
  WebServer server_;
  static constexpr int MAX_SESSIONS = 8;
  String sessions_[MAX_SESSIONS];
  int nextSession_ = 0;
  uint32_t restartAt_ = 0;

  // ── Sessions ──
  String cookieToken() {
    String c = server_.header("Cookie");
    int i = c.indexOf("seq_sess=");
    if (i < 0) return "";
    int e = c.indexOf(';', i);
    return c.substring(i + 8, e < 0 ? c.length() : e);
  }
  bool hasSession() {
    String t = cookieToken();
    if (t.length() != 32) return false;
    for (auto& s : sessions_) if (s == t) return true;
    return false;
  }
  bool auth() {
    if (hasSession()) return true;
    server_.send(401, "application/json", "{\"error\":\"Sessao expirada. Entre de novo.\"}");
    return false;
  }
  String newSession() {
    char t[33];
    for (int i = 0; i < 16; i++) sprintf(t + i * 2, "%02x", (uint8_t)esp_random());
    sessions_[nextSession_] = t;
    nextSession_ = (nextSession_ + 1) % MAX_SESSIONS;
    return String(t);
  }

  // ── Responses ──
  void sendOk() { server_.send(200, "application/json", "{\"ok\":true}"); }
  void sendErr(int code, const char* msg) {
    JsonDocument d; d["error"] = msg;
    String out; serializeJson(d, out);
    server_.send(code, "application/json", out);
  }
  void sendJson(const JsonDocument& d) {
    String out; serializeJson(d, out);
    server_.sendHeader("Cache-Control", "no-store");
    server_.send(200, "application/json", out);
  }
  bool parseBody(JsonDocument& d) {
    if (deserializeJson(d, server_.arg("plain"))) { sendErr(400, "JSON invalido."); return false; }
    return true;
  }

  // ── Public routes ──
  void handleRoot() {
    server_.sendHeader("Cache-Control", "no-store");
    if (hasSession()) server_.send_P(200, "text/html; charset=utf-8", INDEX_HTML);
    else server_.send_P(200, "text/html; charset=utf-8", LOGIN_HTML);
  }

  void handleLogin() {
    if (checkPass(ctx_.store.net, server_.arg("pass"))) {
      server_.sendHeader("Set-Cookie", "seq_sess=" + newSession() + "; Path=/; HttpOnly; SameSite=Strict");
      server_.sendHeader("Location", "/");
      server_.send(303);
      Serial.println("[WEB] login ok");
    } else {
      server_.sendHeader("Location", "/?erro=1");
      server_.send(303);
      Serial.println("[WEB] login refused");
    }
  }

  void handleLogout() {
    String t = cookieToken();
    for (auto& s : sessions_) if (s == t) s = "";
    server_.sendHeader("Set-Cookie", "seq_sess=; Path=/; Max-Age=0; HttpOnly; SameSite=Strict");
    server_.sendHeader("Location", "/");
    server_.send(303);
  }

  void handleLogo() {
    server_.sendHeader("Content-Encoding", "gzip");
    server_.sendHeader("Cache-Control", "public, max-age=31536000, immutable");
    server_.send_P(200, "image/svg+xml", (const char*)LOGO_SVG_GZ, LOGO_SVG_GZ_LEN);
  }

  // Captive portal: on the AP, any unknown host goes to the board's page.
  // On any interface, an unknown page path (e.g. /syscfg from the browser's autocomplete,
  // OS captive-portal probes) also goes to "/". Only the API answers 404.
  void handleNotFound() {
    String uri = server_.uri();
    if (ctx_.net.apOn()) {
      String host = server_.header("Host");
      String apIp = ctx_.net.apIp();
      if (host != apIp) {
        server_.sendHeader("Location", "http://" + apIp + "/");
        server_.send(302);
        return;
      }
    }
    if (server_.method() == HTTP_GET && !uri.startsWith("/api/")) {
      server_.sendHeader("Location", "/");
      server_.send(302);
      return;
    }
    server_.send(404, "text/plain", "Nao encontrado");
  }

  // ── API ──
  void handleState() {
    const SeqConfig& c = ctx_.store.seq;
    SeqRunner::Snap s = ctx_.seq.snap();
    JsonDocument d;
    d["state"] = seqStateName(s.state);
    bool running = s.state != SeqState::PARADO;
    d["step"] = running ? (int)s.idx + 1 : 0;
    d["total"] = (int)c.sequence.size();
    d["channel"] = running && s.idx < c.sequence.size() ? c.sequence[s.idx] : 0;
    d["cycle"] = s.cycle;
    d["cycles"] = c.repeat == RepeatMode::LOOP ? 0 : (c.repeat == RepeatMode::ONCE ? 1 : c.repeat_count);
    d["relays"] = s.relays;
    d["relays_ok"] = s.relaysOk;
    d["seq_max_late_us"] = s.maxLateUs;
    d["manual"] = s.manual;
    d["version"] = FW_VERSION;
    d["uptime_s"] = millis() / 1000;
    d["reset_reason"] = (int)esp_reset_reason();
    d["free_heap"] = ESP.getFreeHeap();
    d["ha_connects"] = ha_connects ? ha_connects() : 0;
    JsonObject n = d["net"].to<JsonObject>();
    n["eth_link"] = ctx_.net.ethLink();
    n["eth_ip"] = ctx_.net.ethIp();
    n["wifi_ip"] = ctx_.net.wifiIp();
    n["ap"] = ctx_.net.apOn();
    n["ap_ssid"] = ctx_.net.apSsid();
    n["mqtt"] = ctx_.mqttConnected ? ctx_.mqttConnected() : false;
    n["hostname"] = ctx_.store.net.hostname;
    sendJson(d);
  }

  void handleGetConfig() {
    JsonDocument d;
    seqToJson(ctx_.store.seq, d);
    sendJson(d);
  }

  void handlePostConfig() {
    JsonDocument d;
    if (!parseBody(d)) return;
    SeqConfig c = ctx_.store.seq;
    const char* err = seqFromJson(d.as<JsonVariantConst>(), c);
    if (!err) err = validateSeqConfig(c);
    if (err) { sendErr(400, err); return; }
    ctx_.store.seq = c;
    ctx_.seq.setConfig(c);    // new config: the sequencer adopts it and goes to PARADO (stopped), before the flash write
    if (!ctx_.store.saveSeq()) { sendErr(500, "Falha ao gravar a configuracao."); return; }
    if (ctx_.onSeqConfigChanged) ctx_.onSeqConfigChanged();
    Serial.printf("[WEB] config saved: %s\n", sequenceToText(c.sequence).c_str());
    sendOk();
  }

  void handleManual() {
    JsonDocument d;
    if (!parseBody(d)) return;
    uint8_t mask = d["mask"] | 0;
    if (!ctx_.seq.requestManual(mask)) { sendErr(409, "Teste manual so com a sequencia parada."); return; }
    sendOk();
  }

  void handleGetNetwork() {
    JsonDocument d;
    netToJson(ctx_.store.net, d, false);
    sendJson(d);
  }

  void handlePostNetwork() {
    JsonDocument d;
    if (!parseBody(d)) return;
    NetConfig n = ctx_.store.net;
    netFromJson(d.as<JsonVariantConst>(), n, false);
    const char* err = validateNetConfig(n);
    if (err) { sendErr(400, err); return; }
    ctx_.store.net = n;
    if (!ctx_.store.saveNet()) { sendErr(500, "Falha ao gravar a rede."); return; }
    sendOk();
    restartAt_ = millis();
  }

  // Asynchronous scan: the first call starts it, the next ones return {"scanning":true} until
  // the result is ready. Repeated networks (several APs with the same SSID) keep the strongest signal.
  void handleWifiScan() {
    int n = WiFi.scanComplete();
    if (n == WIFI_SCAN_RUNNING) { server_.send(200, "application/json", "{\"scanning\":true}"); return; }
    if (n < 0) {
      if (WiFi.getMode() == WIFI_OFF) WiFi.mode(WIFI_STA);   // WiFi off: turn the radio on just to scan
      WiFi.scanNetworks(true);
      server_.send(200, "application/json", "{\"scanning\":true}");
      return;
    }
    JsonDocument d;
    JsonArray a = d["networks"].to<JsonArray>();
    for (int i = 0; i < n; i++) {
      String ssid = WiFi.SSID(i);
      if (ssid.isEmpty()) continue;
      int rssi = WiFi.RSSI(i);
      bool dup = false;
      for (JsonObject o : a) {
        if (o["ssid"] == ssid) { dup = true; if (rssi > (int)o["rssi"]) o["rssi"] = rssi; break; }
      }
      if (dup) continue;
      JsonObject o = a.add<JsonObject>();
      o["ssid"] = ssid;
      o["rssi"] = rssi;
      o["secure"] = WiFi.encryptionType(i) != WIFI_AUTH_OPEN;
    }
    WiFi.scanDelete();
    sendJson(d);
  }

  void handlePassword() {
    JsonDocument d;
    if (!parseBody(d)) return;
    String cur = d["current"] | "", nw = d["new"] | "", conf = d["confirm"] | "";
    if (!checkPass(ctx_.store.net, cur)) { sendErr(403, "Senha atual incorreta."); return; }
    if (nw.isEmpty()) { sendErr(400, "A nova senha nao pode ser vazia."); return; }
    if (nw != conf) { sendErr(400, "A confirmacao nao confere com a nova senha."); return; }
    ctx_.store.net.admin_pass_hash = makePassHash(nw);
    if (!ctx_.store.saveNet()) { sendErr(500, "Falha ao gravar a senha."); return; }
    Serial.println("[WEB] page password changed");
    sendOk();
  }
};
