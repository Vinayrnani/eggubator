#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <ESP8266WiFi.h>
#include <Arduino.h>

#define WIFI_SSID "Sweet Home"
#define WIFI_PASSWORD "dishoom1234"

void connectWiFi() {
  Serial.println("\nConnecting to " + String(WIFI_SSID));
  
  // 1. Initial DHCP connection to discover network info
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  IPAddress localIP = WiFi.localIP();
  IPAddress gatewayIP = WiFi.gatewayIP();
  IPAddress subnetIP = WiFi.subnetMask();
  Serial.println("\nGot IP (DHCP): " + localIP.toString());
  
  // 2. Configure Static IP (.10)
  localIP[3] = 10;
  WiFi.disconnect(true); // Disconnect fully
  WiFi.config(localIP, gatewayIP, subnetIP);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  // 3. Wait for static connection
  delay(3000);
  
  // 4. Fallback if static IP failed
  if (WiFi.status() != WL_CONNECTED || WiFi.localIP() != localIP) {
    Serial.println("\nStatic IP failed, reverting to DHCP...");
    WiFi.disconnect(true);
    WiFi.config(0, 0, 0); // Revert to DHCP
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
  }
  
  Serial.println("\nFinal IP: " + WiFi.localIP().toString());
}

bool isWiFiConnected() {
  return WiFi.status() == WL_CONNECTED;
}

void reconnectWiFi() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected, reconnecting...");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
      delay(500);
      attempts++;
    }
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("Reconnected!");
    }
  }
}

#endif
