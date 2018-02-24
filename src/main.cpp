#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Automaton.h>
#include "Atm_logger.h"
#include "Atm_atwinc1500.h"

const byte currentLoopPin = 1;
const byte pumpRelayPin = 12;
const byte menuPages = 3;
const byte menuRows = 4;
const byte menuWidth = 17;
const unsigned int cardInterval = 500;
char ap_ssid[] = "whiteface1";
char ap_password[] = "secondnaturestudios";

Atm_logger logger;
Atm_atwinc1500 wifi;

// Init display and menus
Adafruit_SSD1306 display = Adafruit_SSD1306();
Atm_step menu;

class Menu {
public:
  enum ItemName {IP, SSID, RSSI, COND, RD15, RELAY, DB, DB_INT, FILE, FILE_INT};
  Menu();
private:
  struct MenuItem {
    ItemName id;
    char content[menuWidth+1];   //There is space on screen for 14 chars, plus null termination
    byte nameLength;             //How much of the content is the name?
  };
  MenuItem menuItems[menuPages][menuRows];

public:
  void draw(int page);
  void updateValue(const ItemName item, const char* value);
};

Menu menuData;

Menu::Menu() :
  menuItems
  {
    {
      {IP, "IP: \0", 4},
      {SSID, "SSID: \0", 6},
      {RSSI, "RSSI: \0", 6}
    },
    {
      {COND, "Cond: \0", 6},
      {RD15, "RD15\%: \0", 7},
      {RELAY, "Relay: \0", 7}
    },
    {
      {DB, "DB: \0", 4},
      {DB_INT, "DB Int: \0", 8},
      {FILE, "\0", 0},
      {FILE_INT, "File Int: \0", 10}
    }
  }
{}

void Menu::draw(int page) {
  display.fillRect(24, 0, 102, 32, BLACK);

  for(int row = 0; row < menuRows; row++) {
    display.setCursor(24, row * 8);
    display.println(menuItems[page][row].content);
  }

  display.display();
}

void Menu::updateValue(const ItemName item, const char* value) {

  for(int page = 0; page < menuPages; page++) {
    for(int row = 0; row < menuRows; row++) {
      if(menuItems[page][row].id == item) {

        strncpy(menuItems[page][row].content + menuItems[page][row].nameLength,
          value, menuWidth - menuItems[page][row].nameLength);

        if(menu.state() == page) {
          display.fillRect(24, row * 8, 102, 8, BLACK);
          display.setCursor(24, row * 8);
          display.println(menuItems[page][row].content);
          display.display();
        }

        return;
      }
    }
  }
}


void drawMenu(int idx, int v, int up) {
  menuData.draw(v);
}

void initDisplay() {
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextWrap(false);
  display.setTextSize(1);
  display.setTextColor(WHITE, BLACK);

  display.drawChar(0, 1, 'A', WHITE, BLACK, 1);
  display.fillTriangle(11, 2, 11, 6, 15, 2+(6-2)/2, WHITE);
  display.drawChar(0, 11, 'B', WHITE, BLACK, 1);
  display.fillRect(11, 12, 5, 5, WHITE);
  display.drawChar(0, 23, 'C', WHITE, BLACK, 1);
  display.drawChar(11, 23, 'i', WHITE, BLACK, 1);
  display.drawCircle(13, 26, 5, WHITE);
  //display.println("Stopped");
  display.drawFastVLine(21, 0, 32, WHITE);
  display.display();
}


// Init buttons and actions
Atm_button startBtn;
Atm_button stopBtn;
Atm_button infoBtn;


void setup() {
  pinMode(LED_BUILTIN, OUTPUT);

  //TODO Add a timer and some code into the wifi machine to periodically update
  //wifi stats like RSSI. Also output IP and SSID to menu once there's better
  //string handling for `menuData.updateValue`.
  //TODO Figure out whether logger or wifi machine should handle logging to DB.
  wifi.begin( ap_ssid, ap_password )
    .onChange( true, [] ( int idx, int v, int up ) {
      menuData.updateValue(Menu::SSID, wifi.ssid());
      Serial.print( "Connected to Wifi, browse to http://" );
      Serial.println( wifi.ip() );
    })
    .onChange( false, [] ( int idx, int v, int up ) {
      Serial.println( "Lost WIFI connection" );
    })
    .start()
    .trace(Serial);

  logger.begin(currentLoopPin, pumpRelayPin, cardInterval)
    .onStart([](int idx, int v, int up){

      Serial.println("Started");
      digitalWrite(LED_BUILTIN, HIGH);
      //TODO find a way to pass contents of `cardInterval` to the updateValue handler.
      //Make better string handling? Shouldn't be hard to modify updateValue to
      //accept strings instead of char[] and then strcopy.

      //menuData.updateValue(Menu::FILE_INT, cardInterval)
      menuData.updateValue(Menu::FILE, logger.getFilename());
    })
    .onStop([](int idx, int v, int up){
      digitalWrite(LED_BUILTIN, LOW);
      Serial.println("Stopped");

      menuData.updateValue(Menu::FILE, "(no file)");
      menuData.updateValue(Menu::COND, "");
      menuData.updateValue(Menu::RELAY, "");
    })
    .onRecord([](int idx, int v, int up){
      char result[8];
      sprintf(result, "%.2f", logger.lastAnalogValue);
      menuData.updateValue(Menu::COND, result);
      menuData.updateValue(Menu::RELAY, logger.lastDigitalValue ? "Closed" : "Open");

      //TODO Add LED blink when value is written
      Serial.print("Analog Value: ");
      Serial.println(logger.lastAnalogValue);
      Serial.print("Digital Value: ");
      Serial.println(logger.lastDigitalValue);
      Serial.print("Count: ");
      Serial.println(v);
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
  //
  //EVEN BETTER!! Incorporate menuData into this step machine.
  menu.onStep(0, drawMenu);
  menu.onStep(1, drawMenu);
  menu.onStep(2, drawMenu);
  menu.trigger( menu.EVT_STEP );
}

void loop() {
  automaton.run();
}
