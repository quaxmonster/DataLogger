#include "Atm_logger.h"

RTC_PCF8523 rtc;

/* Add optional parameters for the state machine to begin()
 * Add extra initialization code
 */

 Atm_logger& Atm_logger::begin(
   int AnalogPin, int DigitalPin,
   unsigned int CardInterval, unsigned int DBInterval) {
  //TODO Add counter or something to know when to log to web.
  // clang-format off
  const static state_t state_table[] PROGMEM = {
    /*                   ON_ENTER     ON_LOOP  ON_EXIT  EVT_TOGGLE  EVT_DB_TIMER  EVT_SD_TIMER  EVT_START  EVT_STOP     ELSE */
    /*   STOPPED */   ENT_STOPPED,    LP_READ,      -1,   STARTING,           -1,           -1,  STARTING,       -1,      -1,
    /*    UPDATE */    ENT_UPDATE,         -1,      -1,         -1,           -1,           -1,        -1,       -1, STOPPED,
    /*  STARTING */  ENT_STARTING,         -1,      -1,         -1,           -1,           -1,        -1,       -1, STARTED,
    /*   STARTED */            -1,    LP_READ,      -1,    STOPPED,    DB_RECORD,    SD_RECORD,        -1,  STOPPED,      -1,
    /* SD_RECORD */ ENT_SD_RECORD,         -1,      -1,         -1,           -1,           -1,        -1,       -1, STARTED,
    /* DB_RECORD */ ENT_DB_RECORD,         -1,      -1,         -1,           -1,           -1,        -1,       -1, STARTED,
  };
  // clang-format on
  Machine::begin( state_table, ELSE );
  this->_analogPin = AnalogPin;
  this->_digitalPin = DigitalPin;
  this->_cardInterval = CardInterval;
  this->_dbInterval = DBInterval;
  pinMode(_digitalPin, INPUT_PULLUP);
  analogReadResolution(12);
  _timer_sd.set(CardInterval);
  _timer_db.set(ATM_TIMER_OFF);
  return *this;
}

/* Add C++ code for each internally handled event (input)
 * The code must return 1 to trigger the event
 */

int Atm_logger::event( int id ) {
  switch ( id ) {
    case EVT_DB_TIMER:
      return _timer_db.expired(this);
    case EVT_SD_TIMER:
      return _timer_sd.expired(this);
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
    case ENT_STOPPED: {
      _timer_db.set(ATM_TIMER_OFF);
      SD.end();
      push( connectors, ON_STOP, 0, 0, 0 );
      return;
    }

    case ENT_UPDATE: {
      push( connectors, ON_UPDATE, 0, 0, 0 );
      return;
    }

    case ENT_STARTING: {
      getNextLogFile();
      _analogValue.reset();
      push( connectors, ON_START, 0, 0, 0 );
      _timer_sd.set(_cardInterval);
      _timer_sd.set(_dbInterval);
      return;
    }

    case LP_READ: {
      _analogValue.addSample(analogRead(_analogPin));
      return;
    }

    case ENT_SD_RECORD: {
      lastTime = rtc.now();
      lastAnalogValue = _analogValue.average();
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
        _logFile.print(lastDigitalValue);
        _logFile.close();
      }

      push( connectors, ON_UPDATE, 0, 0, 0 );
      push( connectors, ON_RECORD, 0, 0, 0 );  //Passing zero as middle param will indicate SD record
      _analogValue.reset();
      return;
    }

    case ENT_DB_RECORD: {
      push( connectors, ON_RECORD, 0, 1, 0 );  //Passing 1 as middle param will indicate DB record
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
      _logFile.println("Time,Cond.,Pump");
      _logFile.close();
    }
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
    "LOGGER\0EVT_TOGGLE\0EVT_DB_TIMER\0EVT_SD_TIMER\0EVT_START\0EVT_STOP\0ELSE\0STOPPED\0UPDATE\0STARTING\0STARTED\0SD_RECORD\0DB_RECORD" );
  return *this;
}
