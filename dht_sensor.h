#ifndef DHT_SENSOR_H
#define DHT_SENSOR_H

#include <Arduino.h>

#define DHTPIN D4

bool useMockSensor = false;
float mockTemp = 25.0;
float mockHum = 50.0;

void setMockSensor(bool enable) {
  useMockSensor = enable;
}

void setMockValues(float temp, float hum) {
  mockTemp = temp;
  mockHum = hum;
}

float readDHT22() {
  if (useMockSensor) return mockTemp;
  int data[5] = {0, 0, 0, 0, 0};
  
  pinMode(DHTPIN, OUTPUT);
  digitalWrite(DHTPIN, LOW);
  delay(18);
  digitalWrite(DHTPIN, HIGH);
  delayMicroseconds(30);
  pinMode(DHTPIN, INPUT);
  
  unsigned long timeout = micros();
  while (digitalRead(DHTPIN) == LOW) {
    if (micros() - timeout > 100) return -1;
  }
  timeout = micros();
  while (digitalRead(DHTPIN) == HIGH) {
    if (micros() - timeout > 100) return -1;
  }
  
  for (int i = 0; i < 40; i++) {
    unsigned long bitStart = micros();
    while (digitalRead(DHTPIN) == LOW) {
      if (micros() - bitStart > 50) break;
    }
    unsigned long bitEnd = micros();
    if (bitEnd - bitStart > 40) data[i / 8] |= (1 << (7 - i % 8));
  }
  
  if (data[4] == ((data[0] + data[1] + data[2] + data[3]) & 0xFF)) {
    return (float)((data[0] << 8) + data[1]) / 10.0;
  }
  return -1;
}

float readHumidity() {
  if (useMockSensor) return mockHum;
  int data[5] = {0, 0, 0, 0, 0};
  
  pinMode(DHTPIN, OUTPUT);
  digitalWrite(DHTPIN, LOW);
  delay(18);
  digitalWrite(DHTPIN, HIGH);
  delayMicroseconds(30);
  pinMode(DHTPIN, INPUT);
  
  unsigned long timeout = micros();
  while (digitalRead(DHTPIN) == LOW) {
    if (micros() - timeout > 100) return -1;
  }
  timeout = micros();
  while (digitalRead(DHTPIN) == HIGH) {
    if (micros() - timeout > 100) return -1;
  }
  
  for (int i = 0; i < 40; i++) {
    unsigned long bitStart = micros();
    while (digitalRead(DHTPIN) == LOW) {
      if (micros() - bitStart > 50) break;
    }
    unsigned long bitEnd = micros();
    if (bitEnd - bitStart > 40) data[i / 8] |= (1 << (7 - i % 8));
  }
  
  if (data[4] == ((data[0] + data[1] + data[2] + data[3]) & 0xFF)) {
    return (float)((data[2] << 8) + data[3]) / 10.0;
  }
  return -1;
}

#endif
