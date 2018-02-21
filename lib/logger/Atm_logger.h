#pragma once

#include <Automaton.h>
#include <SPI.h>
#include <SD.h>
#include <RTClib.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

extern Adafruit_SSD1306 display;

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

    double average() {
      return (float)_total / (float)_count;
    }

    void reset() {
      _total = 0;
      _count = 0;
    }
  private:
    unsigned int _total, _count;
};

class Atm_logger: public Machine {

 public:
  enum { STOPPED, STARTED }; // STATES
  enum { EVT_START, EVT_STOP, EVT_TIMER_LOG, ELSE }; // EVENTS
  Atm_logger( void ) : Machine() {};
  Atm_logger& begin(int AnalogPin, int DigitalPin, unsigned int CardInterval);
  Atm_logger& trace( Stream & stream );
  Atm_logger& trigger( int event );
  int state( void );
  Atm_logger& start( void );
  Atm_logger& stop( void );

 private:
  int _analogPin, _digitalPin;
  atm_timer_millis _timer_log;
  RunningAverage _analogValue;
  unsigned int _cardInterval;
  String _filename;
  File _logFile;
  void getNextLogFile();

  enum { ENT_STOPPED, ENT_STARTED, LP_STARTED }; // ACTIONS
  int event( int id );
  void action( int id );

};

/*
Automaton::ATML::begin - Automaton Markup Language

<?xml version="1.0" encoding="UTF-8"?>
<machines>
  <machine name="Atm_logger">
    <states>
      <STOPPED index="0" on_enter="ENT_STOPPED">
        <EVT_START>STARTED</EVT_START>
      </STOPPED>
      <STARTED index="1" on_enter="ENT_STARTED" on_loop="LP_STARTED">
        <EVT_STOP>STOPPED</EVT_STOP>
      </STARTED>
    </states>
    <events>
      <EVT_START index="0" access="PUBLIC"/>
      <EVT_STOP index="1" access="PUBLIC"/>
      <EVT_TIMER_LOG index="2" access="PRIVATE"/>
    </events>
    <connectors>
    </connectors>
    <methods>
    </methods>
  </machine>
</machines>

Automaton::ATML::end
*/
