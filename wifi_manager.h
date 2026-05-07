#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <ESP8266WiFi.h>
#include <Arduino.h>

#define WIFI_SSID "Sweet Home"
#define WIFI_PASSWORD "dishoom1234"

void connectWiFi() {
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);
  
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  // Get network info
  IPAddress localIP = WiFi.localIP();
  IPAddress gatewayIP = WiFi.gatewayIP();
  IPAddress subnetIP = WiFi.subnetMask();
  Serial.print("Got IP: ");
  Serial.println(localIP);
  
  // Set static IP ending in .10
  localIP[3] = 10;
  WiFi.config(localIP, gatewayIP, subnetIP);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  // Wait to verify connection
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 10) {
    delay(500);
    attempts++;
  }
  
  // If static IP failed, fall back to DHCP
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\nStatic IP failed, falling back to DHCP...");
    WiFi.config(0, 0, 0); // Revert to DHCP
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
  }
  
  Serial.print("\nFinal IP: ");
  Serial.println(WiFi.localIP());
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
