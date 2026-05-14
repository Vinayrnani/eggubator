#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <ESP8266WiFi.h>
#include <Arduino.h>

#define WIFI_SSID "Sweet Home"
#define WIFI_PASSWORD "dishoom1234"

void connectWiFi() {
  
  // 1. Initial DHCP connection to discover network info
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  
  IPAddress localIP = WiFi.localIP();
  IPAddress gatewayIP = WiFi.gatewayIP();
  IPAddress subnetIP = WiFi.subnetMask();
  
  // 2. Configure Static IP (.72)
  localIP[3] = 72;
  WiFi.disconnect(true); // Disconnect fully
  WiFi.config(localIP, gatewayIP, subnetIP);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  // 3. Wait for static connection
  delay(3000);
  
  // 4. Fallback if static IP failed
  if (WiFi.status() != WL_CONNECTED || WiFi.localIP() != localIP) {
    WiFi.disconnect(true);
    WiFi.config(0, 0, 0); // Revert to DHCP
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
    }
  }
  
}

bool isWiFiConnected() {
  return WiFi.status() == WL_CONNECTED;
}

void reconnectWiFi() {
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
      delay(500);
      attempts++;
    }
    if (WiFi.status() == WL_CONNECTED) {
    }
  }
}

#endif
