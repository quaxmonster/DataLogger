#pragma once

#include <Automaton.h>
#include <SPI.h>
#include <SD.h>
#include <RTClib.h>
#include <WiFi101.h>

const int GMT_OFFSET = -8;
const float COUNT_PER_VOLT = 4096. / 3.3;
const float fitSlope = 5.9605;
const float fitOffset = 6.;

class RunningAverage {
  public:
    RunningAverage() {
      this->reset();
    }

    unsigned int addSample (int Sample) {
      _total += Sample;
      return _count++;
    }

    float average() {
      return (float)_total / (float)_count;
    }

    void reset() {
      _total = 0;
      _count = 0;
    }

    //Maybe return this to private eventually, but useful for benchmarking.
    //Tells how many samples were taken and averaged per recorded value.
    //Without Automaton framework it was ~1100, with framework it's ~1010.
    unsigned int _count;

  private:
    unsigned int _total;
};

class Atm_logger: public Machine {

 public:
  enum { STOPPING, STOPPED, UPDATE, STARTING, STARTED, SD_RECORD }; // STATES
  enum { EVT_TOGGLE, EVT_UPDATE_TIMER, EVT_START, EVT_STOP, ELSE }; // EVENTS
  Atm_logger( void ) : Machine() {};
  Atm_logger& begin(int AnalogPin, int DigitalPin, unsigned int CardInterval, unsigned int DBInterval, const char* Server);
  Atm_logger& trace( Stream & stream );
  Atm_logger& trigger( int event );
  int state( void );
  Atm_logger& onRecord( Machine& machine, int event = 0 );
  Atm_logger& onRecord( atm_cb_push_t callback, int idx = 0 );
  Atm_logger& onStart( Machine& machine, int event = 0 );
  Atm_logger& onStart( atm_cb_push_t callback, int idx = 0 );
  Atm_logger& onStop( Machine& machine, int event = 0 );
  Atm_logger& onStop( atm_cb_push_t callback, int idx = 0 );
  Atm_logger& onUpdate( Machine& machine, int event = 0 );
  Atm_logger& onUpdate( atm_cb_push_t callback, int idx = 0 );
  Atm_logger& toggle( void );
  Atm_logger& db_counter( void );
  Atm_logger& update_timer( void );
  Atm_logger& start( void );
  Atm_logger& stop( void );

  float lastAnalogValue;
  float lastCondValue;
  float lastRD15Value;
  bool lastDigitalValue;
  DateTime lastTime;
  DateTime startedTime;

  char* getFilename();

 private:
  enum { ENT_UPDATE, ENT_STARTING, LP_READ, ENT_SD_RECORD, ENT_STOPPING }; // ACTIONS
  enum { ON_RECORD, ON_START, ON_STOP, ON_UPDATE, CONN_MAX }; // CONNECTORS
  atm_connector connectors[CONN_MAX];
  int event( int id );
  void action( int id );

  int _analogPin, _digitalPin;
  atm_counter _db_counter;
  atm_timer_millis _update_timer;
  RunningAverage _analogValue;
  unsigned int _cardInterval;
  unsigned int _dbCount;
  const char* _server;
  char _filename[13];
  File _logFile;
  void getNextLogFile();

};

/*
Automaton::ATML::begin - Automaton Markup Language

<?xml version="1.0" encoding="UTF-8"?>
<machines>
  <machine name="Atm_logger">
    <states>
      <STOPPING index="0" on_enter="ENT_STOPPING">
        <ELSE>STOPPED</ELSE>
      </STOPPING>
      <STOPPED index="1" on_loop="LP_READ">
        <EVT_TOGGLE>STARTING</EVT_TOGGLE>
        <EVT_UPDATE_TIMER>UPDATE</EVT_UPDATE_TIMER>
        <EVT_START>STARTING</EVT_START>
      </STOPPED>
      <UPDATE index="2" on_enter="ENT_UPDATE">
        <ELSE>STOPPED</ELSE>
      </UPDATE>
      <STARTING index="3" on_enter="ENT_STARTING">
        <ELSE>STARTED</ELSE>
      </STARTING>
      <STARTED index="4" on_loop="LP_READ">
        <EVT_TOGGLE>STOPPING</EVT_TOGGLE>
        <EVT_UPDATE_TIMER>SD_RECORD</EVT_UPDATE_TIMER>
        <EVT_STOP>STOPPING</EVT_STOP>
      </STARTED>
      <SD_RECORD index="5" on_enter="ENT_SD_RECORD">
        <ELSE>STARTED</ELSE>
      </SD_RECORD>
    </states>
    <events>
      <EVT_TOGGLE index="0" access="PUBLIC"/>
      <EVT_UPDATE_TIMER index="2" access="MIXED"/>
      <EVT_START index="3" access="PUBLIC"/>
      <EVT_STOP index="4" access="PUBLIC"/>
    </events>
    <connectors>
      <RECORD autostore="0" broadcast="0" dir="PUSH" slots="1"/>
      <START autostore="0" broadcast="0" dir="PUSH" slots="1"/>
      <STOP autostore="0" broadcast="0" dir="PUSH" slots="1"/>
      <UPDATE autostore="0" broadcast="0" dir="PUSH" slots="1"/>
    </connectors>
    <methods>
    </methods>
  </machine>
</machines>

Automaton::ATML::end
*/
