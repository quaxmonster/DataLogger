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
const unsigned int cardInterval = 1000;
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
  // void updateValue(const ItemName item, String& value);
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

// void Menu::updateValue(const ItemName item, String& value) {
//   updateValue(item, value.c_str());
// }

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
  //display.drawChar(0, 11, 'B', WHITE, BLACK, 1);
  //display.fillRect(11, 12, 5, 5, WHITE);
  display.drawChar(0, 23, 'C', WHITE, BLACK, 1);
  display.drawChar(11, 23, 'i', WHITE, BLACK, 1);
  display.drawCircle(13, 26, 5, WHITE);
  display.drawFastVLine(21, 0, 32, WHITE);
  display.display();
}


// Init buttons and actions
Atm_button toggleBtn;
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

      char ipAddress[16];
      IPAddress ip = wifi.ip();
      sprintf(ipAddress,"%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
      menuData.updateValue(Menu::IP, ipAddress);

      char rssiChar[4];
      int rssi = wifi.rssi();
      sprintf(rssiChar, "%-d", rssi);
      menuData.updateValue(Menu::RSSI, rssiChar);

      Serial.print( "Connected to Wifi, @ " );
      Serial.println( wifi.ip() );
    })
    .onChange( false, [] ( int idx, int v, int up ) {
      menuData.updateValue(Menu::IP, "");
      menuData.updateValue(Menu::SSID, "");
      menuData.updateValue(Menu::RSSI, "");
      Serial.println( "Lost WIFI connection" );
    })
    .start();

  logger.begin(currentLoopPin, pumpRelayPin, cardInterval)
    .onStart([](int idx, int v, int up){
      display.fillRect(11, 2, 5, 5, WHITE);
      display.display();
      digitalWrite(LED_BUILTIN, HIGH);

      char cardIntStr[13];
      sprintf(cardIntStr, "%-dms", cardInterval);
      menuData.updateValue(Menu::FILE_INT, cardIntStr);
      menuData.updateValue(Menu::FILE, logger.getFilename());

      Serial.println("Started");
    })
    .onStop([](int idx, int v, int up){
      display.fillRect(11, 2, 5, 5, BLACK);
      display.fillTriangle(11, 2, 11, 6, 15, 2+(6-2)/2, WHITE);
      digitalWrite(LED_BUILTIN, LOW);

      menuData.updateValue(Menu::FILE, "(no file)");
      menuData.updateValue(Menu::COND, "");
      menuData.updateValue(Menu::RELAY, "");

      Serial.println("Stopped");
    })
    .onRecord([](int idx, int v, int up){
      //This used to work and for some reason stopped working.
      //sprintf now returns a huge number and outputs an empty char buffer.
      char result[8];
      sprintf(result, "%-.2f", logger.lastAnalogValue);

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

  toggleBtn.begin(9)
    .onPress(logger, logger.EVT_TOGGLE);
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
