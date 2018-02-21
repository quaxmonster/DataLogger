#include "Atm_logger.h"

/* Add optional parameters for the state machine to begin()
 * Add extra initialization code
 */

Atm_logger& Atm_logger::begin(int AnalogPin, int DigitalPin, unsigned int CardInterval) {
  // clang-format off
  const static state_t state_table[] PROGMEM = {
    /*               ON_ENTER     ON_LOOP  ON_EXIT  EVT_START  EVT_STOP  EVT_TIMER_LOG  ELSE */
    /* STOPPED */ ENT_STOPPED,         -1,      -1,   STARTED,       -1,            -1,   -1,
    /* STARTED */ ENT_STARTED, LP_STARTED,      -1,        -1,  STOPPED,            -1,   -1,
  };
  // clang-format on
  Machine::begin( state_table, ELSE );
  this->_analogPin = AnalogPin;
  this->_digitalPin = DigitalPin;
  this->_cardInterval = CardInterval;
  pinMode(_digitalPin, INPUT_PULLUP);
  analogReadResolution(12);
  _timer_log.set(-1);
  return *this;
}

/* Add C++ code for each internally handled event (input)
 * The code must return 1 to trigger the event
 */

int Atm_logger::event( int id ) {
  switch ( id ) {
    case EVT_TIMER_LOG:
      return _timer_log.expired(this);
  }
  return 0;
}

/* Add C++ code for each action
 * This generates the 'output' for the state machine
 */

void Atm_logger::action( int id ) {
  switch ( id ) {
    case ENT_STOPPED:
      display.setCursor(0, 24);
      display.print("Stopped");
      return;
    case ENT_STARTED:
      getNextLogFile();
      _analogValue.reset();
      display.setCursor(0, 24);
      display.print("Started");
      _timer_log.set(_cardInterval);
      return;
    case LP_STARTED:
      _analogValue.addSample(analogRead(_analogPin));
      return;
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

void Atm_logger::getNextLogFile() {
  if (SD.begin(10)) {   // `10` is chip select pin on Feather M0
    int i = 0;
    while(1) {
      i++;
      if (!SD.exists("data" + (String)i + ".csv")) {
        _filename = "data" + (String)i + ".csv";
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

Atm_logger& Atm_logger::start() {
  trigger( EVT_START );
  return *this;
}

Atm_logger& Atm_logger::stop() {
  trigger( EVT_STOP );
  return *this;
}

/* State trace method
 * Sets the symbol table and the default logging method for serial monitoring
 */

Atm_logger& Atm_logger::trace( Stream & stream ) {
  Machine::setTrace( &stream, atm_serial_debug::trace,
    "LOGGER\0EVT_START\0EVT_STOP\0EVT_TIMER_LOG\0ELSE\0STOPPED\0STARTED" );
  return *this;
}
