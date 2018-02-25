#include "Atm_atwinc1500.h"

/* Add optional parameters for the state machine to begin()
 * Add extra initialization code
 */

Atm_atwinc1500& Atm_atwinc1500::begin(const char ssid[], const char password[]) {
  // clang-format off
  const static state_t state_table[] PROGMEM = {
    /*               ON_ENTER  ON_LOOP  ON_EXIT  EVT_START  EVT_STOP  EVT_TIMER  EVT_CONNECT  EVT_DISCONNECT  ELSE */
    /*    IDLE */          -1,      -1,      -1,     START,       -1,        -1,          -1,             -1,   -1,
    /*   START */   ENT_START,      -1,      -1,        -1,       -1,        -1,          -1,             -1, WAIT,
    /*    WAIT */          -1,      -1,      -1,        -1,       -1,     CHECK,          -1,             -1,   -1,
    /*   CHECK */          -1,      -1,      -1,        -1,       -1,        -1,      ACTIVE,             -1, WAIT,
    /*  ACTIVE */  ENT_ACTIVE,      -1,      -1,        -1,       -1,        -1,          -1,        DISCONN,   -1,
    /* DISCONN */ ENT_DISCONN,      -1,      -1,        -1,       -1,        -1,          -1,             -1, WAIT,
  };
  // clang-format on
  Machine::begin( state_table, ELSE );
  WiFi.setPins(8,7,4,2);    //Configure pins for Feather M0
  WiFi.begin( ssid, password );
  timer.set(500);
  return *this;
}

/* Add C++ code for each internally handled event (input)
 * The code must return 1 to trigger the event
 */

int Atm_atwinc1500::event( int id ) {
  switch ( id ) {
    case EVT_TIMER:
      return timer.expired( this );
      return 0;
    case EVT_CONNECT:
      return WiFi.status() == WL_CONNECTED;
      return 0;
    case EVT_DISCONNECT:
      return WiFi.status() != WL_CONNECTED;
      return 0;
  }
  return 0;
}

/* Add C++ code for each action
 * This generates the 'output' for the state machine
 *
 * Available connectors:
 *   push( connectors, ON_CHANGE, <sub>, <v>, <up> );
 */

void Atm_atwinc1500::action( int id ) {
  switch ( id ) {
    case ENT_START:
      return;
    case ENT_ACTIVE:
      push( connectors, ON_CHANGE, true, 1, 0 );
      return;
    case ENT_DISCONN:
      push( connectors, ON_CHANGE, false, 0, 0 );
      return;
  }
}

IPAddress Atm_atwinc1500::ip( void ) {
  return WiFi.localIP();
}

char* Atm_atwinc1500::ssid( void ) {
  return WiFi.SSID();
}

int Atm_atwinc1500::rssi( void ) {
  return WiFi.RSSI();
}

/* Optionally override the default trigger() method
 * Control how your machine processes triggers
 */

Atm_atwinc1500& Atm_atwinc1500::trigger( int event ) {
  Machine::trigger( event );
  return *this;
}

/* Optionally override the default state() method
 * Control what the machine returns when another process requests its state
 */

int Atm_atwinc1500::state( void ) {
  return current == ACTIVE ? 1 : 0;
}

/* Nothing customizable below this line
 ************************************************************************************************
*/

/* Public event methods
 *
 */

Atm_atwinc1500& Atm_atwinc1500::start() {
  trigger( EVT_START );
  return *this;
}

Atm_atwinc1500& Atm_atwinc1500::stop() {
  trigger( EVT_STOP );
  return *this;
}

/*
 * onChange() push connector variants ( slots 2, autostore 0, broadcast 0 )
 */

Atm_atwinc1500& Atm_atwinc1500::onChange( Machine& machine, int event ) {
  onPush( connectors, ON_CHANGE, 0, 2, 1, machine, event );
  return *this;
}

Atm_atwinc1500& Atm_atwinc1500::onChange( atm_cb_push_t callback, int idx ) {
  onPush( connectors, ON_CHANGE, 0, 2, 1, callback, idx );
  return *this;
}

Atm_atwinc1500& Atm_atwinc1500::onChange( int sub, Machine& machine, int event ) {
  onPush( connectors, ON_CHANGE, sub, 2, 0, machine, event );
  return *this;
}

Atm_atwinc1500& Atm_atwinc1500::onChange( int sub, atm_cb_push_t callback, int idx ) {
  onPush( connectors, ON_CHANGE, sub, 2, 0, callback, idx );
  return *this;
}

/* State trace method
 * Sets the symbol table and the default logging method for serial monitoring
 */

Atm_atwinc1500& Atm_atwinc1500::trace( Stream & stream ) {
  Machine::setTrace( &stream, atm_serial_debug::trace,
    "ATWINC1500\0EVT_START\0EVT_STOP\0EVT_TIMER\0EVT_CONNECT\0EVT_DISCONNECT\0ELSE\0IDLE\0START\0WAIT\0CHECK\0ACTIVE\0DISCONN" );
  return *this;
}
