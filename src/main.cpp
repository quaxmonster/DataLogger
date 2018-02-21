#include <Arduino.h>
#include <Wire.h>
#include <WiFi101.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Automaton.h>
#include "Atm_logger.h"

const byte currentLoopPin = 1;
const byte pumpRelayPin = 12;
const unsigned int cardInterval = 500;


// Init logger
Atm_logger logger;


// Init buttons and actions
Atm_button startBtn;
Atm_button stopBtn;
Atm_button infoBtn;

void startLogging( int idx, int v, int up ) {
  Serial.println("Start button pressed.");
}
void stopLogging( int idx, int v, int up ) {
  Serial.println("Stop button pressed.");
}
void cycleInfo( int idx, int v, int up ) {
  Serial.println("Info button pressed.");
}




// Init display and menus
Adafruit_SSD1306 display = Adafruit_SSD1306();
Atm_step menu;

void cycleMenu( int idx, int v, int up ) {
  switch(v) {
    case 0:
      Serial.println("WiFi info screen selected.");

      display.fillRect(44, 0, 84, 32, BLACK);
      display.setCursor(45, 0);
      display.println("IP: ");
      display.setCursor(45, 8);
      display.println("SSID: ");
      display.setCursor(45, 16);
      display.println("RSSI: ");
      display.display();
      break;

    case 1:
      Serial.println("Data info screen selected.");

      display.fillRect(44, 0, 84, 32, BLACK);
      display.setCursor(45, 0);
      display.println("Cond: ");
      display.setCursor(45, 8);
      display.println("RD15\%: ");
      display.setCursor(45, 16);
      display.println("Relay: ");
      display.display();
      break;

    case 2:
      Serial.println("Logging info screen selected.");

      display.fillRect(44, 0, 84, 32, BLACK);
      display.setCursor(45, 0);
      display.println("DB: ");
      display.setCursor(45, 8);
      display.println("DB Int: ");
      display.setCursor(45, 16);
      display.println("SD: ");
      display.setCursor(45, 24);
      display.println("SD Int: ");
      display.display();
      break;
  }
}

void initDisplay() {
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextWrap(false);
  display.setTextSize(1);
  display.setTextColor(WHITE, BLACK);
  display.setCursor(0,0);

  display.println("A:Start");
  display.println("B:Stop");
  display.println("C:Info");
  display.println("Stopped");
  display.drawFastVLine(43, 0, 32, WHITE);
  display.display();
}



void setup() {
  analogReadResolution(12);

  logger.begin(currentLoopPin, pumpRelayPin, cardInterval);

  startBtn.begin(9)
    .onPress(startLogging);
  stopBtn.begin(6)
    .onPress(stopLogging);
  infoBtn.begin(5)
    .onPress(menu, menu.EVT_STEP);

  initDisplay();
  menu.begin();
  menu.onStep(0, cycleMenu);
  menu.onStep(1, cycleMenu);
  menu.onStep(2, cycleMenu);
  menu.trigger( menu.EVT_STEP );
}

void loop() {
  automaton.run();
}
