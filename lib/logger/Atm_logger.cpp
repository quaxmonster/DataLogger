#include "Atm_logger.h"

RTC_PCF8523 rtc;
WiFiClient client;

/* Add optional parameters for the state machine to begin()
 * Add extra initialization code
 */

 Atm_logger& Atm_logger::begin(
   int AnalogPin, int DigitalPin,
   unsigned int CardInterval, unsigned int DBCount, const char* Server) {
  //TODO Add counter or something to know when to log to web.
  // clang-format off
  const static state_t state_table[] PROGMEM = {
    /*                   ON_ENTER     ON_LOOP  ON_EXIT  EVT_TOGGLE  EVT_UPDATE_TIMER  EVT_START  EVT_STOP     ELSE */
    /*  STOPPING */  ENT_STOPPING,         -1,      -1,         -1,              -1,        -1,       -1, STOPPED,
    /*   STOPPED */            -1,    LP_READ,      -1,   STARTING,          UPDATE,  STARTING,       -1,      -1,
    /*    UPDATE */    ENT_UPDATE,         -1,      -1,         -1,              -1,        -1,       -1, STOPPED,
    /*  STARTING */  ENT_STARTING,         -1,      -1,         -1,              -1,        -1,       -1, STARTED,
    /*   STARTED */            -1,    LP_READ,      -1,   STOPPING,       SD_RECORD,        -1, STOPPING,      -1,
    /* SD_RECORD */ ENT_SD_RECORD,         -1,      -1,         -1,              -1,        -1,       -1, STARTED,
  };
  // clang-format on
  Machine::begin( state_table, ELSE );
  this->_analogPin = AnalogPin;
  this->_digitalPin = DigitalPin;
  this->_cardInterval = CardInterval;
  this->_dbCount = DBCount;
  this->_server = Server;
  pinMode(_digitalPin, INPUT_PULLUP);
  analogReadResolution(12);
  _update_timer.set(CardInterval);
  return *this;
}

/* Add C++ code for each internally handled event (input)
 * The code must return 1 to trigger the event
 */

int Atm_logger::event( int id ) {
  switch ( id ) {
    case EVT_UPDATE_TIMER:
      return _update_timer.expired(this);
  }
  return 0;
}

/* Add C++ code for each action
 * This generates the 'output' for the state machine
 *
 * Available connectors:
 *   push( connectors, ON_RECORD, 0, <v>, <up> );
 *   push( connectors, ON_START, 0, <v>, <up> );
 *   push( connectors, ON_STOP, 0, <v>, <up> );
 *   push( connectors, ON_UPDATE, 0, <v>, <up> );
 */

void Atm_logger::action( int id ) {
  switch ( id ) {
    case ENT_STOPPING: {
      SD.end();
      push( connectors, ON_STOP, 0, 0, 0 );
      return;
    }

    case ENT_UPDATE: {
      lastAnalogValue = _analogValue.average();
      lastCondValue = (((lastAnalogValue / COUNT_PER_VOLT) / 165.) - 0.04) * 687.5;
      lastRD15Value = lastCondValue * fitSlope + fitOffset;
      lastDigitalValue = !digitalRead(_digitalPin);
      push( connectors, ON_UPDATE, 0, _analogValue._count, 0 );
      _analogValue.reset();
      return;
    }

    case ENT_STARTING: {
      Serial.println("Atm_logger::Getting ready to start, getting next available log file.");
      getNextLogFile();   //Find next available serial number for file to record dataXXXX.csv
      Serial.print("Atm_logger::Log file is ");
      Serial.println(_filename);
      _analogValue.reset();
      _db_counter.set(_dbCount);

      if (WiFi.status() == WL_CONNECTED) {

        DateTime oldTime = rtc.now();
        DateTime serverTime = DateTime(WiFi.getTime());
        serverTime = serverTime + TimeSpan(0, GMT_OFFSET, 0, 0);  //Subtract time zone offset
        rtc.adjust(serverTime);

        char result[18];

        Serial.println("Atm_logger::Updating system time before starting.");
        Serial.print("Atm_logger::Old system time was ");
        sprintf(result, "%u/%u/%u %02u:%02u:%02u",
                        oldTime.month(),
                        oldTime.day(),
                        oldTime.year(),
                        oldTime.hour(),
                        oldTime.minute(),
                        oldTime.second());
        Serial.println(result);

        Serial.print("Atm_logger::Server time is ");
        sprintf(result, "%u/%u/%u %02u:%02u:%02u",
                        serverTime.month(),
                        serverTime.day(),
                        serverTime.year(),
                        serverTime.hour(),
                        serverTime.minute(),
                        serverTime.second());
        Serial.println(result);
      } else {
        Serial.println("Atm_logger::Couldn't update system time because WiFi is disconnected.");
      }

      startedTime = rtc.now();
      push( connectors, ON_START, 0, 0, 0 );
      return;
    }

    case LP_READ: {
      _analogValue.addSample(analogRead(_analogPin));
      return;
    }

    case ENT_SD_RECORD: {
      lastTime = rtc.now();
      lastAnalogValue = _analogValue.average();

      // COUNT_PER_VOLT converts analog reading to voltage.
      // 165 is the value of the resistor, in ohms, used to convert voltage drop to current
      // 0.04 is the offset in amps of the current loop (4 mA is 'zero')
      // 687.5 converts from 4-20 mA to 0-110 milliSiemens
      lastCondValue = (((lastAnalogValue / COUNT_PER_VOLT) / 165.) - 0.04) * 687.5;
      lastRD15Value = lastCondValue * fitSlope + fitOffset;
      lastDigitalValue = !digitalRead(_digitalPin);

      _logFile = SD.open(_filename, FILE_WRITE);

      // If the file available, write to it. Note that this can't be trusted to
      // indicate if the file is actually present. If the file was present during
      // `getNextLogFile()`, it will be in cache and will think it's still present.
      // even if card is removed. Unfortunately on the Feather Datalogger wing
      // the pin that indicates whether a card is inserted isn't connected.
      if (_logFile) {
        _logFile.print(lastTime.month());
        _logFile.print('/');
        _logFile.print(lastTime.day());
        _logFile.print('/');
        _logFile.print(lastTime.year());
        _logFile.print(' ');
        _logFile.print(lastTime.hour());
        _logFile.print(':');
        _logFile.print(lastTime.minute());
        _logFile.print(':');
        _logFile.print(lastTime.second());
        _logFile.print(',');
        _logFile.print(lastAnalogValue);
        _logFile.print(',');
        _logFile.print(lastCondValue);
        _logFile.print(',');
        _logFile.print(lastRD15Value);
        _logFile.print(',');
        _logFile.println(lastDigitalValue);
        _logFile.close();
      }

      _db_counter.decrement();

      if (_db_counter.expired()) {
        if (WiFi.status() == WL_CONNECTED) {

          //TODO Find better way to send data than with GET
          //TODO Incorporate non-blocking client.connect
          if (client.connect(_server, 80)) {
            // Make an HTTP request:
            client.print("GET /org/pmtc/etchrTrackr/dataLogger.php?cond=");
            client.print(lastCondValue);
            client.print("&conc=");
            client.print(lastRD15Value);
            client.print("&pumpState=");
            client.print(lastDigitalValue ? '1' : '0');
            client.println(" HTTP/1.1");
            client.print("Host: ");
            client.println(_server);
            client.println("Connection: close");
            client.println();

            //If you want the response from the server, you can uncomment this.
            // Serial.println("Atm_logger::Client response was: ");
            // while (client.connected()) {
            //   char c;
            //   if (client.available()) {
            //     c = client.read();
            //     Serial.print(c);
            //   }
            // }

            client.stop();
          } else {
            Serial.println("Atm_logger::Couldn't connect to client.");
          }
        } else {
          Serial.println("Atm_logger::Couldn't write to database because WiFi was disconnected.");
        }

        _db_counter.set(_dbCount);
      }

      push( connectors, ON_UPDATE, 0, _analogValue._count, 0 );
      push( connectors, ON_RECORD, 0, 0, 0 );

      //Move this before the push connectors once _count benchmarking isn't needed anymore.
      _analogValue.reset();
      return;
    }
  }
}

/* Optionally override the default trigger() method
 * Control how your machine processes triggers
 */

Atm_logger& Atm_logger::trigger( int event ) {
  Machine::trigger( event );
  return *this;
}

/* Optionally override the default state() method
 * Control what the machine returns when another process requests its state
 */

int Atm_logger::state( void ) {
  return Machine::state();
}

char* Atm_logger::getFilename() {
  return _filename;
}

void Atm_logger::getNextLogFile() {
  if (SD.begin(10)) {   // `10` is chip select pin on Feather M0
    int i = 0;
    char temp[13] = "data";   //Max filesize length 8+3+period+null
    char numStr[9] = "";      //Size of max number (9999) + extension ".csv" + null
    while(i < 10000) {
      i++;

      sprintf(numStr, "%d.csv", i);     //Put the number and extension into numStr
      strcpy(temp + 4, numStr);         //Concatenate that with temp

      if (!SD.exists(temp)) {
        strcpy(_filename, temp);
        break;
      }
    }

    _logFile = SD.open(_filename, FILE_WRITE);
    if (_logFile) {
      _logFile.println("Time,Value,Conductivity,RD15,Pump State");
      _logFile.close();
    }
  } else {
    Serial.println("Atm_logger::Couldn't initialize SD card.");
  }
}

/* Nothing customizable below this line
 ************************************************************************************************
*/

/* Public event methods
 *
 */

Atm_logger& Atm_logger::toggle() {
  trigger( EVT_TOGGLE );
  return *this;
}

Atm_logger& Atm_logger::update_timer() {
  trigger( EVT_UPDATE_TIMER );
  return *this;
}

Atm_logger& Atm_logger::start() {
  trigger( EVT_START );
  return *this;
}

Atm_logger& Atm_logger::stop() {
  trigger( EVT_STOP );
  return *this;
}

/*
 * onRecord() push connector variants ( slots 1, autostore 0, broadcast 0 )
 */

Atm_logger& Atm_logger::onRecord( Machine& machine, int event ) {
  onPush( connectors, ON_RECORD, 0, 1, 1, machine, event );
  return *this;
}

Atm_logger& Atm_logger::onRecord( atm_cb_push_t callback, int idx ) {
  onPush( connectors, ON_RECORD, 0, 1, 1, callback, idx );
  return *this;
}

/*
 * onStart() push connector variants ( slots 1, autostore 0, broadcast 0 )
 */

Atm_logger& Atm_logger::onStart( Machine& machine, int event ) {
  onPush( connectors, ON_START, 0, 1, 1, machine, event );
  return *this;
}

Atm_logger& Atm_logger::onStart( atm_cb_push_t callback, int idx ) {
  onPush( connectors, ON_START, 0, 1, 1, callback, idx );
  return *this;
}

/*
 * onStop() push connector variants ( slots 1, autostore 0, broadcast 0 )
 */

Atm_logger& Atm_logger::onStop( Machine& machine, int event ) {
  onPush( connectors, ON_STOP, 0, 1, 1, machine, event );
  return *this;
}

Atm_logger& Atm_logger::onStop( atm_cb_push_t callback, int idx ) {
  onPush( connectors, ON_STOP, 0, 1, 1, callback, idx );
  return *this;
}

/*
 * onUpdate() push connector variants ( slots 1, autostore 0, broadcast 0 )
 */

Atm_logger& Atm_logger::onUpdate( Machine& machine, int event ) {
  onPush( connectors, ON_UPDATE, 0, 1, 1, machine, event );
  return *this;
}

Atm_logger& Atm_logger::onUpdate( atm_cb_push_t callback, int idx ) {
  onPush( connectors, ON_UPDATE, 0, 1, 1, callback, idx );
  return *this;
}

/* State trace method
 * Sets the symbol table and the default logging method for serial monitoring
 */

Atm_logger& Atm_logger::trace( Stream & stream ) {
  Machine::setTrace( &stream, atm_serial_debug::trace,
    "LOGGER\0EVT_TOGGLE\0EVT_UPDATE_TIMER\0EVT_START\0EVT_STOP\0ELSE\0STOPPING\0STOPPED\0UPDATE\0STARTING\0STARTED\0SD_RECORD" );
  return *this;
}
