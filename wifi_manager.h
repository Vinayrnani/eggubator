#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <ESP8266WiFi.h>
#include <Arduino.h>

#define WIFI_SSID "Sweet Home"
#define WIFI_PASSWORD "dishoom1234"

void connectWiFi() {

  // 1. Initial DHCP connection to discover network info
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  // 2. Wait for connection
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    delay(500);
  }

  // 3. Configure Static IP
  IPAddress localIP = WiFi.localIP();
  IPAddress gatewayIP = WiFi.gatewayIP();
  IPAddress subnetIP = WiFi.subnetMask();
  localIP[3] = 72;
  WiFi.config(localIP, gatewayIP, subnetIP);
  
  // 4. Let SDK handle reconnection automatically
  WiFi.setAutoReconnect(true);
}

bool isWiFiConnected() {
  return WiFi.status() == WL_CONNECTED;
}

void reconnectWiFi() {
  // SDK handles auto-reconnect - no manual intervention needed
}

#endif
