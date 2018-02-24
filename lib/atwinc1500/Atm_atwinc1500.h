#pragma once

#include <Automaton.h>
#include <WiFi101.h>

class Atm_atwinc1500: public Machine {

 public:
  enum { IDLE, START, WAIT, CHECK, ACTIVE, DISCONN }; // STATES
  enum { EVT_START, EVT_STOP, EVT_TIMER, EVT_CONNECT, EVT_DISCONNECT, ELSE }; // EVENTS
  Atm_atwinc1500( void ) : Machine() {};
  Atm_atwinc1500& begin(const char ssid[], const char password[]);
  Atm_atwinc1500& trace( Stream & stream );
  Atm_atwinc1500& trigger( int event );
  int state( void );
  Atm_atwinc1500& onChange( Machine& machine, int event = 0 );
  Atm_atwinc1500& onChange( atm_cb_push_t callback, int idx = 0 );
  Atm_atwinc1500& onChange( int sub, Machine& machine, int event = 0 );
  Atm_atwinc1500& onChange( int sub, atm_cb_push_t callback, int idx = 0 );
  Atm_atwinc1500& start( void );
  Atm_atwinc1500& stop( void );

  IPAddress ip( void );
  char* ssid( void );

 private:
  enum { ENT_START, ENT_ACTIVE, ENT_DISCONN }; // ACTIONS
  enum { ON_CHANGE, CONN_MAX = 2 }; // CONNECTORS
  atm_connector connectors[CONN_MAX];
  atm_timer_millis timer;
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
        <EVT_START>START</EVT_START>
      </IDLE>
      <START index="1" on_enter="ENT_START">
        <ELSE>WAIT</ELSE>
      </START>
      <WAIT index="2">
        <EVT_TIMER>CHECK</EVT_TIMER>
      </WAIT>
      <CHECK index="3">
        <EVT_CONNECT>ACTIVE</EVT_CONNECT>
        <ELSE>WAIT</ELSE>
      </CHECK>
      <ACTIVE index="4" on_enter="ENT_ACTIVE">
        <EVT_DISCONNECT>DISCONN</EVT_DISCONNECT>
      </ACTIVE>
      <DISCONN index="5" on_enter="ENT_DISCONN">
        <ELSE>WAIT</ELSE>
      </DISCONN>
    </states>
    <events>
      <EVT_START index="0" access="PUBLIC"/>
      <EVT_STOP index="1" access="PUBLIC"/>
      <EVT_TIMER index="2" access="PRIVATE"/>
      <EVT_CONNECT index="3" access="PRIVATE"/>
      <EVT_DISCONNECT index="4" access="PRIVATE"/>
    </events>
    <connectors>
      <CHANGE autostore="0" broadcast="0" dir="PUSH" slots="2"/>
    </connectors>
    <methods>
    </methods>
  </machine>
</machines>

Automaton::ATML::end
*/
