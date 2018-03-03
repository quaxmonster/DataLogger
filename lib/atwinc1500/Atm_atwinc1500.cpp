#include "Atm_atwinc1500.h"

/* Add optional parameters for the state machine to begin()
 * Add extra initialization code
 */

Atm_atwinc1500& Atm_atwinc1500::begin(const char* ssid, const char* password) {
  // clang-format off
  const static state_t state_table[] PROGMEM = {
    /*                         ON_ENTER  ON_LOOP           ON_EXIT    EVT_TOGGLE  EVT_TIMER  EVT_CONNECT  EVT_DISCONNECT        ELSE */
    /*         IDLE */               -1,      -1,               -1,     STARTING,        -1,          -1,             -1,         -1,
    /* DISCONNECTED */ ENT_DISCONNECTED,      -1, EXT_DISCONNECTED,     STARTING,        -1,          -1,             -1,         -1,
    /*     STARTING */     ENT_STARTING,      -1,               -1,           -1,        -1,          -1,             -1, CONNECTING,
    /*   CONNECTING */               -1,      -1,               -1, DISCONNECTED,  STARTING,      ACTIVE,             -1,         -1,
    /*       ACTIVE */       ENT_ACTIVE,      -1,       EXT_ACTIVE, DISCONNECTED,        -1,          -1,       STARTING,         -1,
  };
  // clang-format on
  Machine::begin( state_table, ELSE );
  WiFi.setPins(8,7,4,2);    //Configure pins for Feather M0
  Serial.println("Atm_atwinc1500::Connecting to network...");
  Serial.print("Atm_atwinc1500::Connection returned ");
  Serial.println(WiFi.begin( ssid, password ));
  _timer.set(5000);         //How long to wait before reattempting WiFi.begin()
  return *this;
}

/* Add C++ code for each internally handled event (input)
 * The code must return 1 to trigger the event
 */

int Atm_atwinc1500::event( int id ) {
  switch ( id ) {
    case EVT_TIMER:
      return _timer.expired( this );
    case EVT_CONNECT:
      return WiFi.status() == WL_CONNECTED;
    case EVT_DISCONNECT:
      return WiFi.status() != WL_CONNECTED;
  }
  return 0;
}

/* Add C++ code for each action
 * This generates the 'output' for the state machine
 *
 * Available connectors:
 *   push( connectors, ON_CONNECT, 0, <v>, <up> );
 *   push( connectors, ON_DISABLE, 0, <v>, <up> );
 *   push( connectors, ON_DISCONNECT, 0, <v>, <up> );
 *   push( connectors, ON_ENABLE, 0, <v>, <up> );
 */

void Atm_atwinc1500::action( int id ) {
  switch ( id ) {
    case ENT_DISCONNECTED: {
      push( connectors, ON_DISABLE, 0, 1, 0 );
      return;
    }

    case EXT_DISCONNECTED: {
      push( connectors, ON_ENABLE, 0, 1, 0);
      return;
    }

    case ENT_STARTING: {
      if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Atm_atwinc1500::Reconnecting to WiFi...");
        Serial.print("Atm_atwinc1500::Connection returned ");
        Serial.println(WiFi.begin());
      }
      return;
    }

    case ENT_ACTIVE: {
      push( connectors, ON_CONNECT, 0, 1, 0 );
      return;
    }

    case EXT_ACTIVE: {
      WiFi.end();
      push( connectors, ON_DISCONNECT, 0, 1, 0);
      return;
    }
  }
}

IPAddress Atm_atwinc1500::ip( void ) {
  return WiFi.localIP();
}

char* Atm_atwinc1500::getSSID( void ) {
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

Atm_atwinc1500& Atm_atwinc1500::toggle() {
  trigger( EVT_TOGGLE );
  return *this;
}

/*
 * onConnect() push connector variants ( slots 1, autostore 0, broadcast 0 )
 */

Atm_atwinc1500& Atm_atwinc1500::onConnect( Machine& machine, int event ) {
  onPush( connectors, ON_CONNECT, 0, 1, 1, machine, event );
  return *this;
}

Atm_atwinc1500& Atm_atwinc1500::onConnect( atm_cb_push_t callback, int idx ) {
  onPush( connectors, ON_CONNECT, 0, 1, 1, callback, idx );
  return *this;
}

/*
 * onDisable() push connector variants ( slots 1, autostore 0, broadcast 0 )
 */

Atm_atwinc1500& Atm_atwinc1500::onDisable( Machine& machine, int event ) {
  onPush( connectors, ON_DISABLE, 0, 1, 1, machine, event );
  return *this;
}

Atm_atwinc1500& Atm_atwinc1500::onDisable( atm_cb_push_t callback, int idx ) {
  onPush( connectors, ON_DISABLE, 0, 1, 1, callback, idx );
  return *this;
}

/*
 * onDisconnect() push connector variants ( slots 1, autostore 0, broadcast 0 )
 */

Atm_atwinc1500& Atm_atwinc1500::onDisconnect( Machine& machine, int event ) {
  onPush( connectors, ON_DISCONNECT, 0, 1, 1, machine, event );
  return *this;
}

Atm_atwinc1500& Atm_atwinc1500::onDisconnect( atm_cb_push_t callback, int idx ) {
  onPush( connectors, ON_DISCONNECT, 0, 1, 1, callback, idx );
  return *this;
}

/*
 * onEnable() push connector variants ( slots 1, autostore 0, broadcast 0 )
 */

Atm_atwinc1500& Atm_atwinc1500::onEnable( Machine& machine, int event ) {
  onPush( connectors, ON_ENABLE, 0, 1, 1, machine, event );
  return *this;
}

Atm_atwinc1500& Atm_atwinc1500::onEnable( atm_cb_push_t callback, int idx ) {
  onPush( connectors, ON_ENABLE, 0, 1, 1, callback, idx );
  return *this;
}

/* State trace method
 * Sets the symbol table and the default logging method for serial monitoring
 */

Atm_atwinc1500& Atm_atwinc1500::trace( Stream & stream ) {
  Machine::setTrace( &stream, atm_serial_debug::trace,
    "IDLE\0ATWINC1500\0EVT_TOGGLE\0EVT_TIMER\0EVT_CONNECT\0EVT_DISCONNECT\0ELSE\0DISCONNECTED\0STARTING\0CONNECTING\0ACTIVE" );
  return *this;
}
