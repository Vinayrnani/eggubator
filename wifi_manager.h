#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <ESP8266WiFi.h>
#include <Arduino.h>

#define WIFI_SSID "Sweet Home"
#define WIFI_PASSWORD "dishoom1234"

#define AP_SSID "EGGubator"

void connectWiFi() {

  // Dual mode: AP always on, STA connects to hardcoded WiFi
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(AP_SSID);

  // Try STA connection (non-blocking with timeout)
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    delay(500);
  }

  // Configure static IP only if connected
  if (WiFi.status() == WL_CONNECTED) {
    IPAddress localIP = WiFi.localIP();
    IPAddress gatewayIP = WiFi.gatewayIP();
    IPAddress subnetIP = WiFi.subnetMask();
    localIP[3] = 72;
    WiFi.config(localIP, gatewayIP, subnetIP);
  }

  // SDK handles STA auto-reconnect in background
  WiFi.setAutoReconnect(true);
}

bool isWiFiConnected() {
  return WiFi.status() == WL_CONNECTED;
}

#endif
