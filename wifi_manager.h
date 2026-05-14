#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <Arduino.h>

#define WIFI_SSID "Sweet Home"
#define WIFI_PASSWORD "dishoom1234"
#define AP_SSID "EGGubator"

DNSServer dnsServer;

void connectWiFi() {
  WiFi.hostname("EGGubator");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    delay(500);
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    WiFi.setAutoReconnect(true);
  } else {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID);
    dnsServer.start(53, "*", WiFi.softAPIP());
  }
}

void processDNS() {
  if (WiFi.getMode() == WIFI_AP) {
    dnsServer.processNextRequest();
  }
}

#endif

