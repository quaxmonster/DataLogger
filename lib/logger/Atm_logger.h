#pragma once

#include <Automaton.h>
#include <SPI.h>
#include <SD.h>
#include <RTClib.h>

//RTC_PCF8523 rtc;

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
  enum { STOPPED, STARTING, STARTED, RECORDING }; // STATES
  enum { EVT_TOGGLE, EVT_START, EVT_STOP, EVT_TIMER_LOG, ELSE }; // EVENTS
  Atm_logger( void ) : Machine() {};
  Atm_logger& begin(int AnalogPin, int DigitalPin, unsigned int CardInterval);
  Atm_logger& trace( Stream & stream );
  Atm_logger& trigger( int event );
  int state( void );
  Atm_logger& onRecord( Machine& machine, int event = 0 );
  Atm_logger& onRecord( atm_cb_push_t callback, int idx = 0 );
  Atm_logger& onStart( Machine& machine, int event = 0 );
  Atm_logger& onStart( atm_cb_push_t callback, int idx = 0 );
  Atm_logger& onStop( Machine& machine, int event = 0 );
  Atm_logger& onStop( atm_cb_push_t callback, int idx = 0 );
  Atm_logger& toggle( void );
  Atm_logger& start( void );
  Atm_logger& stop( void );

  float lastAnalogValue;
  bool lastDigitalValue;

  char* getFilename();

 private:
  enum { ENT_STOPPED, ENT_STARTING, LP_STARTED, ENT_RECORDING }; // ACTIONS
  enum { ON_RECORD, ON_START, ON_STOP, CONN_MAX }; // CONNECTORS
  atm_connector connectors[CONN_MAX];
  int event( int id );
  void action( int id );

  int _analogPin, _digitalPin;
  atm_timer_millis _timer_log;
  RunningAverage _analogValue;
  unsigned int _cardInterval;
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
      <STOPPED index="0" on_enter="ENT_STOPPED">
        <EVT_TOGGLE>STARTING</EVT_TOGGLE>
        <EVT_START>STARTING</EVT_START>
      </STOPPED>
      <STARTING index="1" on_enter="ENT_STARTING">
        <ELSE>STARTED</ELSE>
      </STARTING>
      <STARTED index="2" on_loop="LP_STARTED">
        <EVT_TOGGLE>STOPPED</EVT_TOGGLE>
        <EVT_STOP>STOPPED</EVT_STOP>
        <EVT_TIMER_LOG>RECORDING</EVT_TIMER_LOG>
      </STARTED>
      <RECORDING index="3" on_enter="ENT_RECORDING">
        <ELSE>STARTED</ELSE>
      </RECORDING>
    </states>
    <events>
      <EVT_TOGGLE index="0" access="PUBLIC"/>
      <EVT_START index="1" access="PUBLIC"/>
      <EVT_STOP index="2" access="PUBLIC"/>
      <EVT_TIMER_LOG index="3" access="PRIVATE"/>
    </events>
    <connectors>
      <RECORD autostore="0" broadcast="0" dir="PUSH" slots="1"/>
      <START autostore="0" broadcast="0" dir="PUSH" slots="1"/>
      <STOP autostore="0" broadcast="0" dir="PUSH" slots="1"/>
    </connectors>
    <methods>
    </methods>
  </machine>
</machines>

Automaton::ATML::end
*/

