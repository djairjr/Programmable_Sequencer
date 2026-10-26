#pragma once
// Network: W5500 Ethernet + WiFi STA + configuration AP with captive portal + mDNS (docs/ARCHITECTURE.md §4).

#include <ETH.h>
#include <SPI.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <esp_mac.h>
#include "Config.h"
#include "Pins.h"

class Net {
public:
  void begin(const NetConfig& n) {
    cfg_ = &n;
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);   // works even with WiFi off
    char ssid[24];
    snprintf(ssid, sizeof(ssid), "Sequencer-%02X%02X", mac[4], mac[5]);
    apSsid_ = ssid;

    // Core 3.x enables modem sleep (WIFI_PS_MIN_MODEM) by default: the radio sleeps between beacons and
    // the AP/STA TCP handshake can be refused for seconds. Must come before any WiFi.mode().
    WiFi.setSleep(false);

    Network.setHostname(n.hostname.c_str());
    Network.onEvent(onEvent);

    if (n.eth.enabled) {
      SPI.begin(PIN_ETH_SCK, PIN_ETH_MISO, PIN_ETH_MOSI);
      if (ETH.begin(ETH_PHY_W5500, ETH_PHY_ADDR_W5500, PIN_ETH_CS, PIN_ETH_INT, PIN_ETH_RST, SPI)) {
        if (!n.eth.dhcp) applyStatic(ETH, n.eth);
        Serial.println("[NET] Ethernet W5500 started");
      } else {
        Serial.println("[NET] Ethernet W5500 failed to start");
      }
    }

    wifiWanted_ = n.wifi.enabled && n.wifi_ssid.length() > 0;
    if (wifiWanted_) {
      WiFi.mode(WIFI_STA);
      if (!n.wifi.dhcp) applyStatic(WiFi.STA, n.wifi);
      WiFi.setAutoReconnect(true);
      WiFi.begin(n.wifi_ssid.c_str(), n.wifi_pass.c_str());
      Serial.printf("[NET] WiFi connecting to %s\n", n.wifi_ssid.c_str());
    } else {
      WiFi.mode(WIFI_OFF);
    }

    if (MDNS.begin(n.hostname.c_str())) MDNS.addService("http", "tcp", 80);
    noNetSince_ = millis();
  }

  void loop() {
    uint32_t now = millis();
    bool up = ethUp() || wifiUp();

    // Default route: Ethernet when it has an IP, otherwise WiFi (MQTT follows the default route).
    if (ethUp() && !defaultIsEth_) { Network.setDefaultInterface(ETH); defaultIsEth_ = true; Serial.println("[NET] default route: Ethernet"); }
    if (!ethUp() && defaultIsEth_ && wifiUp()) { Network.setDefaultInterface(WiFi.STA); defaultIsEth_ = false; Serial.println("[NET] default route: WiFi"); }
    if (!ethUp()) defaultIsEth_ = false;

    if (up) {
      noNetSince_ = now;
      if (apOn_) stopAP();
    } else if (!apOn_ && now - noNetSince_ >= AP_GRACE_MS) {
      startAP();
    }
    if (apOn_) {
      dns_.processNextRequest();
      staRetryWhileAP(now);
    }
  }

  bool ethUp() const  { return ETH.linkUp() && ETH.hasIP(); }
  bool wifiUp() const { return wifiWanted_ && WiFi.status() == WL_CONNECTED && WiFi.STA.hasIP(); }
  bool online() const { return ethUp() || wifiUp(); }
  bool apOn() const   { return apOn_; }
  String ethIp() const  { return ethUp() ? ETH.localIP().toString() : String(""); }
  String wifiIp() const { return wifiUp() ? WiFi.localIP().toString() : String(""); }
  String apIp() const   { return apOn_ ? WiFi.softAPIP().toString() : String(""); }
  const String& apSsid() const { return apSsid_; }
  bool ethLink() const { return ETH.linkUp(); }

private:
  static constexpr uint32_t AP_GRACE_MS = 10000;        // no network at all for 10 s -> AP
  static constexpr uint32_t STA_RETRY_EVERY_MS = 60000; // with the AP up, try the saved network every 60 s
  static constexpr uint32_t STA_TRY_WINDOW_MS = 10000;  // ...for at most 10 s
  const NetConfig* cfg_ = nullptr;
  DNSServer dns_;
  String apSsid_;
  bool apOn_ = false;
  bool wifiWanted_ = false;
  bool defaultIsEth_ = false;
  uint32_t noNetSince_ = 0;
  uint32_t staTryAt_ = 0;
  bool staTrying_ = false;

  void staBegin() {
    if (!cfg_->wifi.dhcp) applyStatic(WiFi.STA, cfg_->wifi);
    WiFi.setAutoReconnect(true);
    WiFi.begin(cfg_->wifi_ssid.c_str(), cfg_->wifi_pass.c_str());
  }

  void staStop() {
    WiFi.setAutoReconnect(false);
    WiFi.disconnect(false);
  }

  // A STA searching for a missing network makes the radio hop channels and drops the AP. So,
  // with the AP up the STA stays stopped and only tries in short windows, never with a client on the AP.
  void staRetryWhileAP(uint32_t now) {
    if (!wifiWanted_) return;
    bool hasClient = WiFi.softAPgetStationNum() > 0;
    if (staTrying_) {
      if (hasClient || now - staTryAt_ >= STA_TRY_WINDOW_MS) {
        staStop();
        WiFi.mode(WIFI_AP);
        staTrying_ = false;
        staTryAt_ = now;
      }
    } else if (!hasClient && now - staTryAt_ >= STA_RETRY_EVERY_MS) {
      WiFi.mode(WIFI_AP_STA);
      staBegin();
      staTrying_ = true;
      staTryAt_ = now;
    }
  }

  template <typename IFC>
  static void applyStatic(IFC& ifc, const IfaceConfig& c) {
    IPAddress ip, gw, mask, dns;
    ip.fromString(c.ip); gw.fromString(c.gateway); mask.fromString(c.netmask);
    if (!dns.fromString(c.dns)) dns = gw;
    ifc.config(ip, gw, mask, dns);
  }

  void startAP() {
    if (wifiWanted_) staStop();
    WiFi.mode(WIFI_AP);
    String pass = cfg_->ap_pass.length() ? cfg_->ap_pass : String(DEFAULT_AP_PASS);
    WiFi.softAP(apSsid_.c_str(), pass.c_str());
    dns_.start(53, "*", WiFi.softAPIP());
    apOn_ = true;
    staTrying_ = false;
    staTryAt_ = millis();
    Serial.printf("[NET] no network: AP %s at http://%s\n", apSsid_.c_str(), WiFi.softAPIP().toString().c_str());
  }

  void stopAP() {
    dns_.stop();
    WiFi.softAPdisconnect(false);
    WiFi.mode(wifiWanted_ ? WIFI_STA : WIFI_OFF);
    if (wifiWanted_ && !wifiUp()) staBegin();   // the network that came back was Ethernet: WiFi resumes
    apOn_ = false;
    staTrying_ = false;
    Serial.println("[NET] network available: AP off");
  }

  static void onEvent(arduino_event_id_t ev, arduino_event_info_t) {
    switch (ev) {
      case ARDUINO_EVENT_ETH_CONNECTED:     Serial.println("[NET] Ethernet: link up"); break;
      case ARDUINO_EVENT_ETH_GOT_IP:        Serial.printf("[NET] Ethernet IP %s\n", ETH.localIP().toString().c_str()); break;
      case ARDUINO_EVENT_ETH_DISCONNECTED:  Serial.println("[NET] Ethernet: link down"); break;
      case ARDUINO_EVENT_WIFI_STA_GOT_IP:   Serial.printf("[NET] WiFi IP %s\n", WiFi.localIP().toString().c_str()); break;
      case ARDUINO_EVENT_WIFI_STA_DISCONNECTED: Serial.println("[NET] WiFi disconnected"); break;
      default: break;
    }
  }
};
