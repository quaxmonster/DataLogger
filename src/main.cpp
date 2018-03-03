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
const unsigned int dbInterval = 1000 * 15; //1 minute
const char ap_ssid[] = "AdaloggerTest";
const char ap_password[] = "Adafruit";
// if you don't want to use DNS (and reduce your sketch size)
// use the numeric IP instead of the name for the server:
//IPAddress server(10, 0, 42, 42);  // numeric IP for server (no DNS)
char server[] = "ofweb.srs.is.keysight.com";    // name address for server (using DNS)


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
    char content[menuWidth+1];   //There is space on screen for `menuWidth` chars, plus null termination
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
      {IP, "IP: Connecting\0", 4},
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
      {FILE_INT, "Update: \0", 8}
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
  display.drawFastVLine(10, 13, 3, WHITE);
  display.drawFastVLine(13, 12, 5, WHITE);
  display.drawFastVLine(16, 11, 7, WHITE);
  display.drawChar(0, 23, 'C', WHITE, BLACK, 1);
  display.drawChar(11, 23, 'i', WHITE, BLACK, 1);
  display.drawCircle(13, 26, 5, WHITE);
  display.drawFastVLine(21, 0, 32, WHITE);

  char tempArray[16];
  sprintf(tempArray,"%u.%u.%u.%u", server[0], server[1], server[2], server[3]);
  menuData.updateValue(Menu::DB, tempArray);

  sprintf(tempArray, "%-dms", dbInterval);
  menuData.updateValue(Menu::DB_INT, tempArray);

  sprintf(tempArray, "%-dms", cardInterval);
  menuData.updateValue(Menu::FILE_INT, tempArray);

  display.display();
}


// Init buttons and actions
Atm_button toggleLogging;
Atm_button toggleWiFi;
Atm_button infoBtn;

Atm_led led;



void setup() {
  led.begin(LED_BUILTIN);


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



  logger.begin(currentLoopPin, pumpRelayPin, cardInterval, dbInterval / cardInterval, server)

    .onStart([](int idx, int v, int up){
      display.fillRect(11, 2, 5, 5, WHITE);   //Draw "stop" rectangle icon
      menuData.updateValue(Menu::FILE, logger.getFilename());

      display.display();

      led.on();
    })

    .onStop([](int idx, int v, int up){
      display.fillRect(11, 2, 5, 5, BLACK);   //Erase "stop" rectangle
      display.fillTriangle(11, 2, 11, 6, 15, 2+(6-2)/2, WHITE); //Draw "play" triangle
      led.off();

      menuData.updateValue(Menu::FILE, "(no file)");

      display.display();
    })

    .onUpdate([](int idx, int v, int up){
      // Can't get consistent results using sprintf with floats, so using this
      // hacky workaround. Returns a float with two decimal points of precision.
      // char[8] will fit up to 4 digits to the left of the decimal.

      // >>>Raw analog reading<<<
      // sprintf(result, "%d.%02d", (int)logger.lastAnalogValue, (int)(logger.lastAnalogValue * 100) % 100);
      // menuData.updateValue(Menu::COND, result);

      char result[8] = "";
      sprintf(result, "%d.%03d", (int)logger.lastCondValue, (unsigned int)(logger.lastCondValue * 1000) % 1000);
      menuData.updateValue(Menu::COND, result);

      sprintf(result, "%d.%02d", (int)logger.lastRD15Value, (unsigned int)(logger.lastRD15Value * 100) % 100);
      menuData.updateValue(Menu::RD15, result);

      menuData.updateValue(Menu::RELAY, logger.lastDigitalValue ? "Closed" : "Open");

      display.display();

      // Serial.print("Analog Value: ");
      // Serial.println(logger.lastAnalogValue);
      // Serial.print("Digital Value: ");
      // Serial.println(logger.lastDigitalValue);
      // Serial.print("Count: ");
      // Serial.println(v);
    })

    .onRecord([](int idx, int v, int up){
      led.blink(0, 10, 1)
        .onFinish( led, led.EVT_ON )
        .trigger(led.EVT_BLINK);
    });


    //TODO Add a timer and some code into the wifi machine to periodically update
    //wifi stats like RSSI.
    wifi.begin( ap_ssid, ap_password )

      .onConnect([] ( int idx, int v, int up ) {
        menuData.updateValue(Menu::SSID, wifi.getSSID());
        Serial.println("Connected to SSID ");
        Serial.println(WiFi.SSID());

        char ipAddress[16];
        IPAddress ip = wifi.ip();
        sprintf(ipAddress,"%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
        menuData.updateValue(Menu::IP, ipAddress);

        char rssiChar[4];
        int rssi = wifi.rssi();
        sprintf(rssiChar, "%-d", rssi);
        menuData.updateValue(Menu::RSSI, rssiChar);

        display.display();
      })

      .onDisconnect([] ( int idx, int v, int up ) {
        menuData.updateValue(Menu::IP, "");
        menuData.updateValue(Menu::SSID, "");
        menuData.updateValue(Menu::RSSI, "");

        display.display();
      })

      .onEnable([] ( int idx, int v, int up ) {
        display.fillRect(9, 11, 9, 7, BLACK);
        display.drawFastVLine(10, 13, 3, WHITE);
        display.drawFastVLine(13, 12, 5, WHITE);
        display.drawFastVLine(16, 11, 7, WHITE);
        menuData.updateValue(Menu::IP, "Connecting");
        display.display();
      })

      .onDisable([] ( int idx, int v, int up ) {
        display.drawLine(9, 17, 17, 11, WHITE);
        display.display();
      })

      .toggle();


  toggleLogging.begin(9)
    .onPress(logger, logger.EVT_TOGGLE);
  toggleWiFi.begin(6)
    .onPress(wifi, wifi.EVT_TOGGLE);
  infoBtn.begin(5)
    .onPress(menu, menu.EVT_STEP);

}

void loop() {
  automaton.run();
}
