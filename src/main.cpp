/*
 * insomniaTV - ESP32 Smart IR Sleep Controller
 *
 * Blink test firmware for ESP32-C3 hardware validation
 * Copyright 2026 insomniaTV Contributors
 */

#include <Arduino.h>
#include <Ticker.h>

// Onboard LED (active-low) for nologo C3 super mini
#define LED_PIN 8

Ticker blinker;
Ticker toggler;
Ticker changer;
float blinkerPace = 0.1;       // seconds
const float togglePeriod = 5;  // seconds


void blink() {
  digitalWrite(LED_PIN, !digitalRead(LED_PIN));
  // Optional: verbose output (can spam serial at high blink rates)
  // Serial.println("[BLINK] LED toggled");
}

void change() {
  Serial.println("[CHANGE] blinkerPace updated to 0.5s");
  blinkerPace = 0.5;
  // If blinker is active, re-attach with new pace
  if (blinker.active()) {
    blinker.detach();
    blinker.attach(blinkerPace, blink);
    Serial.println("[CHANGE] blinker re-attached with new pace");
  }
}

void toggle() {
  static bool isBlinking = false;
  if (isBlinking) {
    blinker.detach();
    isBlinking = false;
    Serial.println("[TOGGLE] Blinking STOPPED");
  } else {
    blinker.attach(blinkerPace, blink);
    isBlinking = true;
    Serial.printf("[TOGGLE] Blinking STARTED (pace: %.2fs)\n", blinkerPace);
  }
  digitalWrite(LED_PIN, LOW);  // Active-low LED: LOW = ON
}

void setup() {
  Serial.begin(115200);
  while (!Serial) { ; }  // wait for serial port (useful for native USB boards)

  Serial.println("\n=== System Starting ===");
  Serial.printf("LED_PIN: %d\n", LED_PIN);
  Serial.printf("Initial blinkerPace: %.2fs\n", blinkerPace);
  Serial.printf("Toggle period: %.1fs\n", togglePeriod);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);  // start with LED OFF (assuming active-low)

  toggler.attach(togglePeriod, toggle);
  Serial.println("[SETUP] toggler attached");

  changer.once(30, change);
  Serial.println("[SETUP] changer scheduled for 30s");

  Serial.println("=== Setup Complete ===\n");
}

void loop() {
  // Nothing here - Tickers handle everything asynchronously
  // Add delay(10) if you want to reduce CPU load on some platforms
}
