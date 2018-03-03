#pragma once

#include <Automaton.h>
#include <WiFi101.h>

class Atm_atwinc1500: public Machine {

 public:
  enum { IDLE, DISCONNECTED, STARTING, CONNECTING, ACTIVE }; // STATES
  enum { EVT_TOGGLE, EVT_TIMER, EVT_CONNECT, EVT_DISCONNECT, ELSE }; // EVENTS
  Atm_atwinc1500( void ) : Machine() {};
  Atm_atwinc1500& begin(const char* SSID, const char* Password);
  Atm_atwinc1500& trace( Stream & stream );
  Atm_atwinc1500& trigger( int event );
  int state( void );
  Atm_atwinc1500& onConnect( Machine& machine, int event = 0 );
  Atm_atwinc1500& onConnect( atm_cb_push_t callback, int idx = 0 );
  Atm_atwinc1500& onDisable( Machine& machine, int event = 0 );
  Atm_atwinc1500& onDisable( atm_cb_push_t callback, int idx = 0 );
  Atm_atwinc1500& onDisconnect( Machine& machine, int event = 0 );
  Atm_atwinc1500& onDisconnect( atm_cb_push_t callback, int idx = 0 );
  Atm_atwinc1500& onEnable( Machine& machine, int event = 0 );
  Atm_atwinc1500& onEnable( atm_cb_push_t callback, int idx = 0 );
  Atm_atwinc1500& toggle( void );

  IPAddress ip( void );
  char* getSSID( void );
  int rssi( void );

 private:
  enum { ENT_DISCONNECTED, EXT_DISCONNECTED, ENT_STARTING, ENT_ACTIVE, EXT_ACTIVE }; // ACTIONS
  enum { ON_CONNECT, ON_DISABLE, ON_DISCONNECT, ON_ENABLE, CONN_MAX }; // CONNECTORS
  atm_connector connectors[CONN_MAX];
  atm_timer_millis _timer;
  int event( int id );
  void action( int id );

};

/*
Automaton::ATML::begin - Automaton Markup Language

<?xml version="1.0" encoding="UTF-8"?>
<machines>
  <machine name="Atm_atwinc1500">
    <states>
      <IDLE index="0">
      </IDLE>
      <DISCONNECTED index="1" on_enter="ENT_DISCONNECTED" on_exit="EXT_DISCONNECTED">
        <EVT_TOGGLE>STARTING</EVT_TOGGLE>
      </DISCONNECTED>
      <STARTING index="2" on_enter="ENT_STARTING">
        <ELSE>CONNECTING</ELSE>
      </STARTING>
      <CONNECTING index="3">
        <EVT_TOGGLE>DISCONNECTED</EVT_TOGGLE>
        <EVT_TIMER>STARTING</EVT_TIMER>
        <EVT_CONNECT>ACTIVE</EVT_CONNECT>
      </CONNECTING>
      <ACTIVE index="4" on_enter="ENT_ACTIVE" on_exit="EXT_ACTIVE">
        <EVT_TOGGLE>DISCONNECTED</EVT_TOGGLE>
        <EVT_DISCONNECT>STARTING</EVT_DISCONNECT>
      </ACTIVE>
    </states>
    <events>
      <EVT_TOGGLE index="0" access="PUBLIC"/>
      <EVT_TIMER index="1" access="PRIVATE"/>
      <EVT_CONNECT index="2" access="PRIVATE"/>
      <EVT_DISCONNECT index="3" access="PRIVATE"/>
    </events>
    <connectors>
      <CONNECT autostore="0" broadcast="0" dir="PUSH" slots="1"/>
      <DISABLE autostore="0" broadcast="0" dir="PUSH" slots="1"/>
      <DISCONNECT autostore="0" broadcast="0" dir="PUSH" slots="1"/>
      <ENABLE autostore="0" broadcast="0" dir="PUSH" slots="1"/>
    </connectors>
    <methods>
    </methods>
  </machine>
</machines>

Automaton::ATML::end
*/
