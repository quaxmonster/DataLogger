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

// Init WiFi module
//TODO built an Automaton machine for the WiFi module.

// Init logger
Atm_logger logger;

// Init display and menus
Adafruit_SSD1306 display = Adafruit_SSD1306();
Atm_step menu;

//TODO replace this nasty big switch case with an intelligent menu class that
//keeps track of info items and which screen/location they're at, and setters
//for each item. Then any function can just update an item, and the class will
//decide if it's currently visible and needs to be updated or not, and where
//on the display to put it.
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


// Init buttons and actions
Atm_button startBtn;
Atm_button stopBtn;
Atm_button infoBtn;



void setup() {
  analogReadResolution(12);

  logger.begin(currentLoopPin, pumpRelayPin, cardInterval)
    .onStart([](int idx, int v, int up){
      display.setCursor(0, 24);
      display.print("Started");
      display.display();
      Serial.println("Started");
    })
    .onStop([](int idx, int v, int up){
      display.setCursor(0, 24);
      display.print("Stopped");
      display.display();
      Serial.println("Stopped");
    })
    .onRecord([](int idx, int v, int up){
      //TODO direct this output to the screen via intelligent menu class.
      Serial.println("Analog Value: " + (String)logger.lastAnalogValue);
      Serial.println("Digital Value: " + (String)logger.lastDigitalValue);
      Serial.println("Count: " + (String)v);
    });

  startBtn.begin(9)
    .onPress(logger, logger.EVT_START);
  stopBtn.begin(6)
    .onPress(logger, logger.EVT_STOP);
  infoBtn.begin(5)
    .onPress(menu, menu.EVT_STEP);

  initDisplay();
  menu.begin();
  //TODO If I get around to making an intelligent menu class, I can dynamically
  //build this series of `menu.onStep` instructions based on how many screens
  //the menu class contains using a for loop or something.
  menu.onStep(0, cycleMenu);
  menu.onStep(1, cycleMenu);
  menu.onStep(2, cycleMenu);
  menu.trigger( menu.EVT_STEP );
}

void loop() {
  automaton.run();
}
