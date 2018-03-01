# DataLogger
Arduino-based, SAMD compatible custom datalogger. This project is based off of Adafruit's Feather platform, and is built for hardware that receives analog input from a 4-20 mA current loop, as well as digital input from a relay that is driving a pump.

Data is logged locally to an onboard SD card, and logged remotely to a SQL database.

The code uses the excellent Automaton code library to allow for event-based concurrent programming and managing multiple modules.
